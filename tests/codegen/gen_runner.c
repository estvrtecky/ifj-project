#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include "codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Tento kod bol 15.11. vygenerovany Claude AI verziou Sonnet 5.5, na ziadost aby vygeneroval script na otestovanie codegen

static AST_node *mk(AST_node_type kind){
  AST_node *n = calloc(1, sizeof *n);
  n->kind = kind; n->sym_id = -1; n->line = 1;
  return n;
}
static void push(AST_node_list *l, AST_node *n){
  if(l->count == l->capacity){
    l->capacity = l->capacity ? l->capacity * 2 : 4;
    l->items = realloc(l->items, l->capacity * sizeof *l->items);
  }
  l->items[l->count++] = n;
}
static AST_node *i_(int64_t v){ AST_node *n = mk(AST_int_lit); n->as.int_lit.value = v; n->data_type = TYPE_INT; return n; }
static AST_node *d_(double v){ AST_node *n = mk(AST_double_lit); n->as.double_lit.value = v; n->data_type = TYPE_DOUBLE; return n; }
static AST_node *s_(const char *s){ AST_node *n = mk(AST_string_lit); n->as.string_lit.value = strdup(s); n->data_type = TYPE_STRING; return n; }
static AST_node *b_(bool v){ AST_node *n = mk(AST_bool_lit); n->as.bool_lit.value = v; n->data_type = TYPE_BOOLEAN; return n; }
static AST_node *u_(void){ AST_node *n = mk(AST_unit_lit); n->data_type = TYPE_UNIT; return n; }
static AST_node *id_(const char *name, int sym){ AST_node *n = mk(AST_id); n->as.id.name = strdup(name); n->sym_id = sym; n->data_type = TYPE_INT; return n; }
static AST_node *typed_(AST_node *n, Data_types t){ n->data_type = t; return n; }
static AST_node *i2d_(AST_node *x){ AST_node *n = mk(AST_conv_int_to_double); n->as.conv.operand = x; n->data_type = TYPE_DOUBLE; return n; }
static AST_node *bin_(AST_op_type op, AST_node *l, AST_node *r){
  AST_node *n = mk(AST_binop);
  n->as.binop.op = op; n->as.binop.left = l; n->as.binop.right = r;
  n->data_type = TYPE_INT;
  return n;
}
static AST_node *un_(AST_op_type op, AST_node *x){ AST_node *n = mk(AST_unop); n->as.unop.op = op; n->as.unop.operand = x; n->data_type = TYPE_INT; return n; }
static AST_node *call_(const char *name, AST_node *a, AST_node *b){
  AST_node *n = mk(AST_call);
  n->as.call.name = strdup(name);
  if(a) push(&n->as.call.args, a);
  if(b) push(&n->as.call.args, b);
  return n;
}
static AST_node *call3_(const char *name, AST_node *a, AST_node *b, AST_node *c){ AST_node *n = call_(name, a, b); if(c) push(&n->as.call.args, c); return n; }
static AST_node *stmt_(AST_node *e){ AST_node *n = mk(AST_expr_stmt); n->as.expr_stmt.expr = e; return n; }
static AST_node *ret_(AST_node *v){ AST_node *n = mk(AST_return); n->as.return_stmt.value = v; return n; }
static AST_node *print_(AST_node *e){ return stmt_(call_("print", e, NULL)); }
static AST_node *val_(const char *name, int sym, AST_node *init){
  AST_node *n = mk(AST_var_def);
  n->as.var_def.name = strdup(name); n->as.var_def.is_mutable = true;
  n->as.var_def.init = init; n->sym_id = sym;
  return n;
}
static AST_node *assign_(const char *name, int sym, AST_node *v){
  AST_node *n = mk(AST_assign);
  n->as.assign.name = strdup(name); n->as.assign.value = v; n->sym_id = sym;
  return n;
}
static AST_node *if_(AST_node *c, AST_node *t, AST_node *e){
  AST_node *n = mk(AST_if);
  n->as.if_stmt.cond = c; n->as.if_stmt.then_block = t; n->as.if_stmt.else_block = e;
  return n;
}
static AST_node *while_(AST_node *c, AST_node *body){
  AST_node *n = mk(AST_while);
  n->as.while_stmt.cond = c; n->as.while_stmt.body = body;
  return n;
}
static AST_node *block_(AST_node *first, ...){
  AST_node *b = mk(AST_block);
  va_list ap; va_start(ap, first);
  for(AST_node *s = first; s; s = va_arg(ap, AST_node *)) push(&b->as.block.stmts, s);
  va_end(ap);
  return b;
}
static void param_(AST_node *fn, const char *name, int sym, Data_types t){
  AST_func_def_data *f = &fn->as.func_def;
  f->params = realloc(f->params, (f->param_count + 1) * sizeof *f->params);
  f->params[f->param_count++] = (AST_param){ .name = strdup(name), .sym_id = sym, .type = t };
}
static AST_node *func_(const char *name, bool is_main, AST_node *body){
  AST_node *f = mk(AST_func_def);
  f->as.func_def.name = strdup(name);
  f->as.func_def.is_main = is_main;
  f->as.func_def.body = body;
  return f;
}
static AST_node *prog_(AST_node *first, ...){
  AST_node *p = mk(AST_program);
  va_list ap; va_start(ap, first);
  for(AST_node *f = first; f; f = va_arg(ap, AST_node *)) push(&p->as.program.funcs, f);
  va_end(ap);
  return p;
}
static AST_node *main_(AST_node *body){ return prog_(func_("main", true, body), NULL); }

/* funkcia, ktora vracia jeden vyraz (implicitny return) */
static AST_node *fn_(const char *name, Data_types rt, AST_node *result_expr){
  result_expr->is_result = true;
  AST_node *f = func_(name, false, block_(stmt_(result_expr), NULL));
  f->as.func_def.return_type = rt;
  f->as.func_def.return_explicit = true;
  return f;
}

static AST_node *build(const char *t){
  /* ---------- zaklad ---------- */
  if(!strcmp(t,"print_int"))  return main_(block_(print_(i_(42)), NULL));
  if(!strcmp(t,"arith"))      return main_(block_(print_(bin_(OP_add, i_(1), bin_(OP_mul, i_(2), i_(3)))), NULL));
  if(!strcmp(t,"print_str"))  return main_(block_(print_(s_("a b#c\nd")), NULL));
  if(!strcmp(t,"neg"))        return main_(block_(print_(un_(OP_neg, i_(5))), NULL));
  if(!strcmp(t,"cmp"))        return main_(block_(print_(bin_(OP_le, i_(1), i_(2))), print_(bin_(OP_ne, i_(1), i_(2))), print_(bin_(OP_gt, i_(1), i_(2))), NULL));
  if(!strcmp(t,"var"))        return main_(block_(val_("x", 1, i_(5)), print_(id_("x", 1)), NULL));
  if(!strcmp(t,"if_else"))    return main_(block_(if_(b_(true), block_(print_(i_(1)), NULL), block_(print_(i_(2)), NULL)), NULL));
  if(!strcmp(t,"if_else2"))   return main_(block_(if_(b_(false), block_(print_(i_(1)), NULL), block_(print_(i_(2)), NULL)), NULL));
  if(!strcmp(t,"while"))      return main_(block_(val_("i", 1, i_(0)),
                                 while_(bin_(OP_lt, id_("i",1), i_(3)),
                                        block_(print_(id_("i",1)), assign_("i",1, bin_(OP_add, id_("i",1), i_(1))), NULL)), NULL));
  if(!strcmp(t,"while_def"))  return main_(block_(val_("i", 1, i_(0)),
                                 while_(bin_(OP_lt, id_("i",1), i_(3)),
                                        block_(val_("y",2, bin_(OP_mul, id_("i",1), i_(2))), print_(id_("y",2)),
                                               assign_("i",1, bin_(OP_add, id_("i",1), i_(1))), NULL)), NULL));
  if(!strcmp(t,"readint"))    return main_(block_(val_("n", 1, call_("readInt", NULL, NULL)), print_(bin_(OP_add, id_("n",1), i_(1))), NULL));
  if(!strcmp(t,"readint_bad"))return main_(block_(val_("n", 1, call_("readInt", NULL, NULL)), print_(id_("n",1)), NULL));
  if(!strcmp(t,"strlen"))     return main_(block_(print_(call_("strLength", s_("abc"), NULL)), NULL));
  if(!strcmp(t,"ord"))        return main_(block_(print_(call_("ord", s_("A"), i_(0))), print_(s_(" ")), print_(call_("ord", s_("A"), i_(5))), NULL));
  if(!strcmp(t,"call")){
    AST_node *foo = fn_("foo", TYPE_INT, i_(7));
    return prog_(func_("main", true, block_(print_(call_("foo", NULL, NULL)), NULL)), foo, NULL);
  }
  if(!strcmp(t,"chr"))        return main_(block_(print_(call_("chr", i_(65), NULL)), NULL));
  if(!strcmp(t,"unit"))       return main_(block_(print_(u_()), NULL));
  if(!strcmp(t,"tostr"))      return main_(block_(print_(call_("toStr", i_(42), NULL)), print_(s_(" ")), print_(call_("toStr", d_(2.5), NULL)), print_(s_(" ")), print_(call_("toStr", u_(), NULL)), NULL));
  if(!strcmp(t,"double"))     return main_(block_(print_(typed_(bin_(OP_div, i2d_(i_(5)), d_(2.0)), TYPE_DOUBLE)), print_(s_(" ")), print_(call_("toInt", d_(3.9), NULL)), NULL));
  if(!strcmp(t,"strcat"))     return main_(block_(print_(typed_(bin_(OP_add, s_("IFJ"), s_("26")), TYPE_STRING)), print_(s_(" ")), print_(bin_(OP_lt, s_("abc"), s_("abd"))), NULL));
  if(!strcmp(t,"readline"))   return main_(block_(val_("s", 1, call_("readLine", NULL, NULL)), print_(typed_(id_("s",1), TYPE_STRING)), NULL));
  if(!strcmp(t,"readdouble")) return main_(block_(val_("d", 1, call_("readDouble", NULL, NULL)), print_(call_("toStr", typed_(id_("d",1), TYPE_DOUBLE), NULL)), NULL));
  if(!strcmp(t,"shadow"))     return main_(block_(val_("x", 1, i_(10)),
                                 if_(b_(true), block_(val_("x", 2, i_(20)), print_(id_("x",2)), print_(s_(" ")), NULL), block_(NULL)),
                                 print_(id_("x",1)), NULL));
  if(!strcmp(t,"params")){
    AST_node *sub = fn_("sub", TYPE_INT, bin_(OP_sub, id_("a",1), id_("b",2)));
    param_(sub, "a", 1, TYPE_INT); param_(sub, "b", 2, TYPE_INT);
    return prog_(func_("main", true, block_(print_(call_("sub", i_(10), i_(3))), NULL)), sub, NULL);
  }
  if(!strcmp(t,"fact")){
    AST_node *fact = func_("fact", false, block_(
      if_(bin_(OP_le, id_("n",1), i_(1)), block_(ret_(i_(1)), NULL),
          block_(ret_(bin_(OP_mul, id_("n",1), call_("fact", bin_(OP_sub, id_("n",1), i_(1)), NULL))), NULL)), NULL));
    param_(fact, "n", 1, TYPE_INT);
    fact->as.func_def.return_type = TYPE_INT; fact->as.func_def.return_explicit = true;
    return prog_(func_("main", true, block_(print_(call_("fact", i_(5), NULL)), NULL)), fact, NULL);
  }

  /* ---------- subStr ---------- */
  if(!strcmp(t,"substr"))     return main_(block_(print_(call3_("subStr", s_("abcdef"), i_(1), i_(4))), print_(s_(" ")), print_(call3_("subStr", s_("abc"), i_(2), i_(2))), NULL));
  if(!strcmp(t,"substr_neg")) return main_(block_(print_(call3_("subStr", s_("abc"), i_(-1), i_(2))), NULL));
  if(!strcmp(t,"substr_ord")) return main_(block_(print_(call3_("subStr", s_("abc"), i_(2), i_(1))), NULL));
  if(!strcmp(t,"substr_oob")) return main_(block_(print_(call3_("subStr", s_("abc"), i_(0), i_(4))), NULL));

  /* ---------- BOOLTHEN ---------- */
  if(!strcmp(t,"b_not"))       return main_(block_(print_(un_(OP_not, b_(true))), NULL));
  if(!strcmp(t,"b_and"))       return main_(block_(print_(bin_(OP_and, b_(true), b_(false))), NULL));
  if(!strcmp(t,"b_or"))        return main_(block_(print_(bin_(OP_or, b_(false), b_(true))), NULL));
  /* 1/0 by dalo chybu 57, ak by sa prava strana vyhodnotila */
  if(!strcmp(t,"b_and_short")) return main_(block_(print_(bin_(OP_and, b_(false),
                                   bin_(OP_eq, bin_(OP_div, i_(1), i_(0)), i_(0)))), NULL));
  if(!strcmp(t,"b_or_short"))  return main_(block_(print_(bin_(OP_or, b_(true),
                                   bin_(OP_eq, bin_(OP_div, i_(1), i_(0)), i_(0)))), NULL));
  if(!strcmp(t,"b_var"))       return main_(block_(val_("f", 1, b_(false)), print_(id_("f",1)), NULL));
  if(!strcmp(t,"if_noelse"))   return main_(block_(if_(b_(true), block_(print_(i_(1)), NULL), NULL), print_(i_(2)), NULL));
  if(!strcmp(t,"if_noelse2"))  return main_(block_(if_(b_(false), block_(print_(i_(1)), NULL), NULL), print_(i_(2)), NULL));
  if(!strcmp(t,"else_if"))     return main_(block_(
                                   if_(b_(false), block_(print_(i_(1)), NULL),
                                       block_(if_(b_(true), block_(print_(i_(2)), NULL),
                                                  block_(print_(i_(3)), NULL)), NULL)), NULL));
  if(!strcmp(t,"b_return")){
    AST_node *yes = fn_("yes", TYPE_BOOLEAN, b_(true));
    return prog_(func_("main", true, block_(print_(call_("yes", NULL, NULL)), NULL)), yes, NULL);
  }

  /* ---------- FUNEXP ---------- */
  if(!strcmp(t,"nested_call")){ /* f1(3) = f2(3+1) + 1 = 4*2 + 1 = 9 */
    AST_node *f2 = fn_("f2", TYPE_INT, bin_(OP_mul, id_("y",1), i_(2)));
    param_(f2, "y", 1, TYPE_INT);
    AST_node *f1 = fn_("f1", TYPE_INT, bin_(OP_add, call_("f2", bin_(OP_add, id_("x",1), i_(1)), NULL), i_(1)));
    param_(f1, "x", 1, TYPE_INT);
    return prog_(func_("main", true, block_(print_(call_("f1", i_(3), NULL)), NULL)), f1, f2, NULL);
  }
  if(!strcmp(t,"nested_arg")){  /* f1(f2(5)) = (5+1)*2 = 12 */
    AST_node *f2 = fn_("f2", TYPE_INT, bin_(OP_add, id_("y",1), i_(1)));
    param_(f2, "y", 1, TYPE_INT);
    AST_node *f1 = fn_("f1", TYPE_INT, bin_(OP_mul, id_("x",1), i_(2)));
    param_(f1, "x", 1, TYPE_INT);
    return prog_(func_("main", true, block_(print_(call_("f1", call_("f2", i_(5), NULL), NULL)), NULL)), f1, f2, NULL);
  }
  if(!strcmp(t,"mutual_rec")){  /* is_even(4) == true */
    AST_node *ev = func_("is_even", false, block_(
      if_(bin_(OP_eq, id_("n",1), i_(0)), block_(ret_(b_(true)), NULL),
          block_(ret_(call_("is_odd", bin_(OP_sub, id_("n",1), i_(1)), NULL)), NULL)), NULL));
    param_(ev, "n", 1, TYPE_INT); ev->as.func_def.return_type = TYPE_BOOLEAN; ev->as.func_def.return_explicit = true;
    AST_node *od = func_("is_odd", false, block_(
      if_(bin_(OP_eq, id_("n",1), i_(0)), block_(ret_(b_(false)), NULL),
          block_(ret_(call_("is_even", bin_(OP_sub, id_("n",1), i_(1)), NULL)), NULL)), NULL));
    param_(od, "n", 1, TYPE_INT); od->as.func_def.return_type = TYPE_BOOLEAN; od->as.func_def.return_explicit = true;
    return prog_(func_("main", true, block_(print_(call_("is_even", i_(4), NULL)), NULL)), ev, od, NULL);
  }
  if(!strcmp(t,"fe_sum")){      /* one() + one() = 2 */
    AST_node *one = fn_("one", TYPE_INT, i_(1));
    return prog_(func_("main", true, block_(print_(bin_(OP_add, call_("one",NULL,NULL), call_("one",NULL,NULL))), NULL)), one, NULL);
  }
  if(!strcmp(t,"fe_two")){      /* add(add(1,2), add(3,4)) = 10 */
    AST_node *add = fn_("add", TYPE_INT, bin_(OP_add, id_("a",1), id_("b",2)));
    param_(add, "a", 1, TYPE_INT); param_(add, "b", 2, TYPE_INT);
    return prog_(func_("main", true, block_(print_(call_("add",
                   call_("add", i_(1), i_(2)), call_("add", i_(3), i_(4)))), NULL)), add, NULL);
  }
  if(!strcmp(t,"fe_expr")){     /* 2 * add1(3) + 1 = 9 */
    AST_node *add1 = fn_("add1", TYPE_INT, bin_(OP_add, id_("n",1), i_(1)));
    param_(add1, "n", 1, TYPE_INT);
    return prog_(func_("main", true, block_(print_(bin_(OP_add, bin_(OP_mul, i_(2), call_("add1", i_(3), NULL)), i_(1))), NULL)), add1, NULL);
  }
  if(!strcmp(t,"fe_deep")){     /* add1(add1(add1(0))) = 3 */
    AST_node *add1 = fn_("add1", TYPE_INT, bin_(OP_add, id_("n",1), i_(1)));
    param_(add1, "n", 1, TYPE_INT);
    return prog_(func_("main", true, block_(print_(call_("add1", call_("add1", call_("add1", i_(0), NULL), NULL), NULL)), NULL)), add1, NULL);
  }
  return NULL;
}

int main(int argc, char **argv){
  if(argc < 2) return 2;
  AST_node *p = build(argv[1]);
  if(!p){ fprintf(stderr, "unknown test %s\n", argv[1]); return 2; }
  return generate_code(p, stdout);
}
