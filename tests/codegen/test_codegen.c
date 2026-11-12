/*
 * Testy generátora IFJcode26 (TDD).
 *
 * Cely obsah tohto suboru bol vygenerovany Claude AI verzia 5.5 Sonent dna 12.11 na ziadost o vytvorenie testovacej sady na otestovanie codegen
 * Každý test si ručne postaví kúsok AST, zavolá generate_code a výstup
 * (text IFJcode26) porovná s tým, čo čakáme.
 *
 * Dva druhy kontrol:
 *   - check_exact:    výstup sa musí zhodovať presne (hlavička, prázdny main)
 *   - check_in_order: očakávané riadky sa musia v výstupe objaviť v tomto
 *                     poradí (iné riadky medzi nimi nevadia a riadok stačí,
 *                     ak ZAČÍNA očakávaným textom). Test teda nezlyhá, ak si
 *                     vyberieš trochu iné, ale správne inštrukcie
 *                     (napr. JUMPIFEQS vs JUMPIFNEQS).
 *
 * Preklad:  gcc -Wall -Wextra -Isrc tests/test_codegen.c src/codegen.c ...
 * (viď tests/run.sh)
 */
#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ADAPTER: názvy z types.h. Ak sa u vás volajú inak, prepíš. */
#define TT_INT TYPE_INT

/* ===================== pomocné funkcie na stavbu AST ===================== */

static AST_node *mk(AST_node_type kind) {
  AST_node *n = calloc(1, sizeof *n);
  n->kind = kind;
  n->sym_id = -1;
  n->line = 1;
  return n;
}

static void push(AST_node_list *l, AST_node *n) {
  if (l->count == l->capacity) {
    l->capacity = l->capacity ? l->capacity * 2 : 4;
    l->items = realloc(l->items, l->capacity * sizeof *l->items);
  }
  l->items[l->count++] = n;
}

static AST_node *mk_int(int64_t v) {
  AST_node *n = mk(AST_int_lit);
  n->as.int_lit.value = v;
  n->data_type = TT_INT;
  return n;
}
static AST_node *mk_dbl(double v) {
  AST_node *n = mk(AST_double_lit);
  n->as.double_lit.value = v;
  return n;
}
static AST_node *mk_str(const char *s) {
  AST_node *n = mk(AST_string_lit);
  n->as.string_lit.value = strdup(s);
  return n;
}
static AST_node *mk_bool(bool b) {
  AST_node *n = mk(AST_bool_lit);
  n->as.bool_lit.value = b;
  return n;
}
static AST_node *mk_id(const char *name, int sym) {
  AST_node *n = mk(AST_id);
  n->as.id.name = strdup(name);
  n->sym_id = sym;
  n->data_type = TT_INT;
  return n;
}
static AST_node *mk_binop(AST_op_type op, AST_node *l, AST_node *r) {
  AST_node *n = mk(AST_binop);
  n->as.binop.op = op;
  n->as.binop.left = l;
  n->as.binop.right = r;
  n->data_type = TT_INT;
  return n;
}
static AST_node *mk_unop(AST_op_type op, AST_node *x) {
  AST_node *n = mk(AST_unop);
  n->as.unop.op = op;
  n->as.unop.operand = x;
  return n;
}
static AST_node *mk_var_def(const char *name, int sym, AST_node *init) {
  AST_node *n = mk(AST_var_def);
  n->as.var_def.name = strdup(name);
  n->as.var_def.is_mutable = true;
  n->as.var_def.init = init;
  n->sym_id = sym;
  return n;
}
static AST_node *mk_expr_stmt(AST_node *e) {
  AST_node *n = mk(AST_expr_stmt);
  n->as.expr_stmt.expr = e;
  return n;
}
static AST_node *mk_print(AST_node *arg) {
  AST_node *c = mk(AST_call);
  c->as.call.name = strdup("print");
  push(&c->as.call.args, arg);
  return mk_expr_stmt(c);
}
static AST_node *mk_while(AST_node *cond, AST_node *body) {
  AST_node *n = mk(AST_while);
  n->as.while_stmt.cond = cond;
  n->as.while_stmt.body = body;
  return n;
}
static AST_node *mk_if(AST_node *cond, AST_node *then_b, AST_node *else_b) {
  AST_node *n = mk(AST_if);
  n->as.if_stmt.cond = cond;
  n->as.if_stmt.then_block = then_b;
  n->as.if_stmt.else_block = else_b;
  return n;
}

/* Blok z príkazov; zoznam končí NULL:  mk_block(a, b, c, NULL) */
static AST_node *mk_block(AST_node *first, ...) {
  AST_node *b = mk(AST_block);
  va_list ap;
  va_start(ap, first);
  for (AST_node *s = first; s; s = va_arg(ap, AST_node *))
    push(&b->as.block.stmts, s);
  va_end(ap);
  return b;
}

static AST_node *mk_func(const char *name, bool is_main, AST_node *body) {
  AST_node *f = mk(AST_func_def);
  f->as.func_def.name = strdup(name);
  f->as.func_def.is_main = is_main;
  f->as.func_def.body = body;
  return f;
}

/* Program z funkcií; zoznam končí NULL */
static AST_node *mk_prog(AST_node *first, ...) {
  AST_node *p = mk(AST_program);
  va_list ap;
  va_start(ap, first);
  for (AST_node *f = first; f; f = va_arg(ap, AST_node *))
    push(&p->as.program.funcs, f);
  va_end(ap);
  return p;
}

/* Skratka: program s jediným main, ktorého telo sú dané príkazy */
static AST_node *prog_main(AST_node *body_block) {
  return mk_prog(mk_func("main", true, body_block), NULL);
}

/* ============================ spúšťanie testov =========================== */

static int n_pass, n_fail;

static char *run_gen(AST_node *prog, int *rc) {
  char *buf = NULL;
  size_t size = 0;
  FILE *f = open_memstream(&buf, &size);
  *rc = generate_code(prog, f);
  fclose(f);
  return buf;
}

static void verdict(const char *name, int ok, const char *why, const char *got) {
  if (ok) {
    n_pass++;
    printf("  PASS  %s\n", name);
  } else {
    n_fail++;
    printf("  FAIL  %s\n        %s\n", name, why);
    printf("        ---- vygenerovaný výstup ----\n%s        -----------------------------\n",
           got ? got : "(nič)\n");
  }
}

static void check_exact(const char *name, AST_node *prog, const char *expected) {
  int rc;
  char *got = run_gen(prog, &rc);
  char why[128] = "výstup sa nezhoduje s očakávaným textom";
  int ok = (rc == 0) && strcmp(got, expected) == 0;
  if (rc != 0)
    snprintf(why, sizeof why, "generate_code vrátila %d, čakalo sa 0", rc);
  if (!ok && rc == 0)
    printf("  [očakávané]\n%s", expected);
  verdict(name, ok, why, got);
  free(got);
}

/* expected[] = riadky (alebo ich začiatky), ktoré musia byť v tomto poradí */
static void check_in_order(const char *name, AST_node *prog, const char **expected, int n) {
  int rc;
  char *got = run_gen(prog, &rc);
  char why[200];
  int k = 0;
  if (rc == 0) {
    char *copy = strdup(got);
    for (char *line = strtok(copy, "\n"); line && k < n; line = strtok(NULL, "\n"))
      if (strncmp(line, expected[k], strlen(expected[k])) == 0)
        k++;
    free(copy);
  }
  int ok = (rc == 0) && k == n;
  if (rc != 0)
    snprintf(why, sizeof why, "generate_code vrátila %d, čakalo sa 0", rc);
  else
    snprintf(why, sizeof why, "nenašiel sa riadok začínajúci: \"%s\" (alebo je v zlom poradí)",
             k < n ? expected[k] : "");
  verdict(name, ok, why, got);
  free(got);
}

static void check_count(const char *name, AST_node *prog, const char *line_start, int want) {
  int rc;
  char *got = run_gen(prog, &rc);
  int cnt = 0;
  char *copy = strdup(got);
  for (char *line = strtok(copy, "\n"); line; line = strtok(NULL, "\n"))
    if (strncmp(line, line_start, strlen(line_start)) == 0)
      cnt++;
  free(copy);
  char why[160];
  snprintf(why, sizeof why, "riadok \"%s\" sa vyskytol %dx, čakalo sa %dx", line_start, cnt, want);
  verdict(name, rc == 0 && cnt == want, why, got);
  free(got);
}

/* Všetky návestia typu "LABEL _L..." musia byť navzájom rôzne */
static void check_unique_labels(const char *name, AST_node *prog) {
  int rc;
  char *got = run_gen(prog, &rc);
  char *labels[256];
  int n = 0, dup = 0;
  char *copy = strdup(got);
  for (char *line = strtok(copy, "\n"); line && n < 256; line = strtok(NULL, "\n"))
    if (strncmp(line, "LABEL ", 6) == 0 && line[6] != '$') {
      for (int i = 0; i < n; i++)
        if (strcmp(labels[i], line) == 0)
          dup = 1;
      labels[n++] = line;
    }
  char why[120];
  snprintf(why, sizeof why, "našlo sa %d návestí a niektoré sa opakujú (alebo žiadne)", n);
  verdict(name, rc == 0 && !dup && n >= 4, why, got);
  free(copy);
  free(got);
}

/* ================================= testy ================================= */

int main(void) {
  printf("== KROK 1: kostra programu ==\n");

  check_exact("prázdny main",
              prog_main(mk_block(NULL)),
              ".IFJcode26\n"
              "DEFVAR GF@_t0\nDEFVAR GF@_t1\nDEFVAR GF@_t2\nDEFVAR GF@_t3\n"
              "CALL $main\n"
              "EXIT int@0\n"
              "\n"
              "LABEL $main\n"
              "CREATEFRAME\nPUSHFRAME\n"
              "PUSHS nil@nil\n"
              "POPFRAME\nRETURN\n");

  {
    AST_node *p = mk_prog(mk_func("foo", false, mk_block(NULL)), NULL);
    int rc;
    char *got = run_gen(p, &rc);
    verdict("bez @main -> nenulový návratový kód", rc != 0,
            "generate_code mala vrátiť nenulovú hodnotu, lebo chýba main", got);
    free(got);
  }

  {
    const char *e[] = {".IFJcode26", "CALL $main", "EXIT int@0", "LABEL $main", "LABEL $foo"};
    check_in_order("dve funkcie: main aj foo majú návestie",
                   mk_prog(mk_func("main", true, mk_block(NULL)),
                           mk_func("foo", false, mk_block(NULL)), NULL),
                   e, 5);
  }

  {
    int rc;
    char *got = run_gen(prog_main(mk_block(NULL)), &rc);
    verdict("prvý riadok je .IFJcode26", strncmp(got, ".IFJcode26\n", 11) == 0,
            "výstup musí začínať riadkom .IFJcode26", got);
    free(got);
  }

  printf("== KROK 2: literály a print ==\n");

  {
    const char *e[] = {"PUSHS int@42", "POPS GF@_t1", "WRITE GF@_t1"};
    check_in_order("print(42)", prog_main(mk_block(mk_print(mk_int(42)), NULL)), e, 3);
  }
  {
    const char *e[] = {"PUSHS float@0x1.8p+0", "WRITE"};
    check_in_order("print(1.5) - double ako %a", prog_main(mk_block(mk_print(mk_dbl(1.5)), NULL)), e, 2);
  }
  {
    const char *e[] = {"PUSHS bool@true", "WRITE"};
    check_in_order("print(true)", prog_main(mk_block(mk_print(mk_bool(true)), NULL)), e, 2);
  }
  {
    const char *e[] = {"PUSHS string@a\\032b\\035c\\092d", "WRITE"};
    check_in_order("reťazec: medzera, # a \\ sa escapujú ako \\xyz",
                   prog_main(mk_block(mk_print(mk_str("a b#c\\d")), NULL)), e, 2);
  }

  printf("== KROK 3: výrazy ==\n");

  {
    const char *e[] = {"PUSHS int@1", "PUSHS int@2", "ADDS", "WRITE"};
    check_in_order("print(1 + 2) - najprv operandy, potom ADDS",
                   prog_main(mk_block(mk_print(mk_binop(OP_add, mk_int(1), mk_int(2))), NULL)), e, 4);
  }
  {
    const char *e[] = {"PUSHS int@1", "PUSHS int@2", "PUSHS int@3", "MULS", "ADDS"};
    check_in_order("1 + (2 * 3) - vnorený výraz",
                   prog_main(mk_block(mk_print(mk_binop(OP_add, mk_int(1),
                                                        mk_binop(OP_mul, mk_int(2), mk_int(3)))), NULL)),
                   e, 5);
  }
  {
    const char *e[] = {"PUSHS int@0", "PUSHS int@5", "SUBS"};
    check_in_order("-5 sa skladá ako 0 - 5",
                   prog_main(mk_block(mk_print(mk_unop(OP_neg, mk_int(5))), NULL)), e, 3);
  }
  {
    const char *e[] = {"PUSHS int@1", "PUSHS int@2", "GTS", "NOTS"};
    check_in_order("1 <= 2 je !(1 > 2)",
                   prog_main(mk_block(mk_print(mk_binop(OP_le, mk_int(1), mk_int(2))), NULL)), e, 4);
  }
  {
    const char *e[] = {"PUSHS int@1", "PUSHS int@2", "EQS", "NOTS"};
    check_in_order("1 != 2 je !(1 == 2)",
                   prog_main(mk_block(mk_print(mk_binop(OP_ne, mk_int(1), mk_int(2))), NULL)), e, 4);
  }

  printf("== KROK 4: premenné ==\n");

  {
    const char *e[] = {"CREATEFRAME", "PUSHFRAME", "DEFVAR LF@x$1", "PUSHS int@5",
                       "POPS LF@x$1", "PUSHS LF@x$1", "WRITE"};
    check_in_order("val x = 5; print(x) - DEFVAR na začiatku funkcie",
                   prog_main(mk_block(mk_var_def("x", 1, mk_int(5)), mk_print(mk_id("x", 1)), NULL)),
                   e, 7);
  }
  {
    const char *e[] = {"DEFVAR LF@x$1", "DEFVAR LF@x$2"};
    check_in_order("tieňovanie: x$1 a x$2 sú rôzne premenné",
                   prog_main(mk_block(mk_var_def("x", 1, mk_int(1)),
                                      mk_if(mk_bool(true), mk_block(mk_var_def("x", 2, mk_int(2)), NULL), NULL),
                                      NULL)),
                   e, 2);
  }

  printf("== KROK 5: if a while ==\n");

  {
    const char *e[] = {"PUSHS bool@true", "JUMPIF", "PUSHS int@1", "JUMP ",
                       "LABEL _L", "PUSHS int@2", "LABEL _L"};
    check_in_order("if/else: skok na else, skok za else, dve návestia",
                   prog_main(mk_block(mk_if(mk_bool(true),
                                            mk_block(mk_print(mk_int(1)), NULL),
                                            mk_block(mk_print(mk_int(2)), NULL)),
                                      NULL)),
                   e, 7);
  }
  {
    const char *e[] = {"LABEL _L", "PUSHS bool@true", "JUMPIF", "PUSHS int@1", "JUMP _L", "LABEL _L"};
    check_in_order("while: návestie, podmienka, výstupný skok, telo, skok späť",
                   prog_main(mk_block(mk_while(mk_bool(true), mk_block(mk_print(mk_int(1)), NULL)), NULL)),
                   e, 6);
  }
  check_count("DEFVAR premennej v tele while sa vypíše presne raz (nie v cykle!)",
              prog_main(mk_block(mk_while(mk_bool(true),
                                          mk_block(mk_var_def("y", 7, mk_int(1)), NULL)), NULL)),
              "DEFVAR LF@y$7", 1);
  check_unique_labels("viac cyklov a if-ov = všetky návestia unikátne",
                      prog_main(mk_block(
                          mk_while(mk_bool(true), mk_block(NULL)),
                          mk_while(mk_bool(true), mk_block(NULL)),
                          mk_if(mk_bool(true), mk_block(NULL), mk_block(NULL)), NULL)));

  printf("\nVýsledok: %d prešlo, %d zlyhalo\n", n_pass, n_fail);
  return n_fail ? 1 : 0;
}
