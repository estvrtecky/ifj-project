/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       ast.c
   # Brief:      Abstract syntax tree implementation.
   # Authors:    Damian Volosiny (xvolosd00)
*/

/* Function comments in this file were drafted with AI assistance (Claude).
   Prompt: "Vies mi pomoct s tymi komentarmi konkretne" (comments for ast.c
   functions) */

#include "ast.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

/* Returns a newly allocated copy of 'source', or NULL if 'source' is NULL
   or the allocation fails. The caller owns the copy. */
static char *copy_string(const char *source) {
  if (source == NULL) {
    return NULL;
  }
  size_t source_len = strlen(source);
  char *source_al = malloc(source_len + 1);
  if (source_al == NULL) {
    return NULL;
  }
  memcpy(source_al, source, source_len + 1);
  return source_al;
}

/* Frees all nodes in 'list' (including their subtrees) and the array itself,
   then resets the list to empty. */
static void free_list(AST_node_list *list) {
  for (size_t i = 0; i < list->count; i++) {
    AST_free(list->items[i]);
  }
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

/* Returns the IFJ26 name of a data type ("Int", "Double", ...),
   or "?" for an unknown type. Used by the dump. */
static const char *type_name(Data_types type) {
  switch (type) {
  case TYPE_INT:
    return "Int";

  case TYPE_DOUBLE:
    return "Double";

  case TYPE_STRING:
    return "String";

  case TYPE_UNIT:
    return "Unit";

  case TYPE_BOOLEAN:
    return "Boolean";

  case TYPE_UNKNOWN:
    return "?";

  default:
    return "Unknown";
  }
}

/* Returns true if 'type' is an expression node kind
   (literal, identifier, operator, call or conversion). */
static bool is_expr_kind(AST_node_type type) {
  switch (type) {
  case AST_int_lit:
  case AST_double_lit:
  case AST_string_lit:
  case AST_unit_lit:
  case AST_bool_lit:
  case AST_id:
  case AST_binop:
  case AST_unop:
  case AST_call:
  case AST_conv_int_to_double:
  case AST_member:
    return true;

  default:
    return false;
  }
}

/* Allocates a new zero-initialized node of the given kind and sets common
   fields: data_type = TYPE_UNKNOWN, sym_id = -1, is_result = false. Returns
   NULL on allocation failure. */
AST_node *AST_new(AST_node_type kind, size_t line) {
  AST_node *node = calloc(1, sizeof *node);
  if (node == NULL) {
    return NULL;
  }
  node->kind = kind;
  node->line = line;
  node->data_type = TYPE_UNKNOWN;
  node->sym_id = -1;
  node->is_result = false;
  return node;
}

/* Node constructors. Each returns a new node of the corresponding kind or NULL
   on allocation failure. Strings are copied. Child nodes passed as arguments
   are owned by the new node; if construction fails, they are freed here. */
AST_node *AST_new_int_lit(int64_t value, size_t line) {
  AST_node *node = AST_new(AST_int_lit, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.int_lit.value = value;
  return node;
}

AST_node *AST_new_double_lit(double value, size_t line) {
  AST_node *node = AST_new(AST_double_lit, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.double_lit.value = value;
  return node;
}

AST_node *AST_new_string_lit(const char *value, size_t line) {
  AST_node *node = AST_new(AST_string_lit, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.string_lit.value = copy_string(value);
  if (node->as.string_lit.value == NULL) {
    free(node);
    return NULL;
  }
  return node;
}

AST_node *AST_new_bool_lit(bool value, size_t line) {
  AST_node *node = AST_new(AST_bool_lit, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.bool_lit.value = value;
  return node;
}

AST_node *AST_new_unit_lit(size_t line) { return AST_new(AST_unit_lit, line); }

AST_node *AST_new_id(const char *name, size_t line) {
  AST_node *node = AST_new(AST_id, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.id.name = copy_string(name);
  if (node->as.id.name == NULL) {
    free(node);
    return NULL;
  }
  return node;
}

AST_node *AST_new_binop(AST_op_type op, AST_node *left, AST_node *right,
                        size_t line) {
  AST_node *node = AST_new(AST_binop, line);
  if (node == NULL) {
    AST_free(left);
    AST_free(right);
    return NULL;
  }
  node->as.binop.op = op;
  node->as.binop.left = left;
  node->as.binop.right = right;
  return node;
}

AST_node *AST_new_unop(AST_op_type op, AST_node *operand, size_t line) {
  AST_node *node = AST_new(AST_unop, line);
  if (node == NULL) {
    AST_free(operand);
    return NULL;
  }
  node->as.unop.op = op;
  node->as.unop.operand = operand;
  return node;
}

// Creates a call node; arguments are added by the caller via AST_list_push.
AST_node *AST_new_call(const char *name, size_t line) {
  AST_node *node = AST_new(AST_call, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.call.name = copy_string(name);
  if (node->as.call.name == NULL) {
    free(node);
    return NULL;
  }
  node->as.call.has_parens = true;
  return node;
}

// Creates an implicit Int -> Double conversion; used only by semantic analysis.
AST_node *AST_new_conv(AST_node *operand, size_t line) {
  AST_node *node = AST_new(AST_conv_int_to_double, line);
  if (node == NULL) {
    AST_free(operand);
    return NULL;
  }
  node->as.conv.operand = operand;
  return node;
}

AST_node *AST_new_var_def(const char *name, bool is_mutable,
                          Data_types decl_type, AST_node *init, size_t line) {
  AST_node *node = AST_new(AST_var_def, line);
  if (node == NULL) {
    AST_free(init);
    return NULL;
  }
  node->as.var_def.name = copy_string(name);
  if (node->as.var_def.name == NULL) {
    free(node);
    AST_free(init);
    return NULL;
  }
  node->as.var_def.is_mutable = is_mutable;
  node->as.var_def.decl_type = decl_type;
  node->as.var_def.init = init;
  return node;
}

AST_node *AST_new_assign(const char *name, AST_node *value, size_t line) {
  AST_node *node = AST_new(AST_assign, line);
  if (node == NULL) {
    AST_free(value);
    return NULL;
  }
  node->as.assign.name = copy_string(name);
  if (node->as.assign.name == NULL) {
    free(node);
    AST_free(value);
    return NULL;
  }
  node->as.assign.value = value;
  return node;
}

// Creates an if node; else_block may be NULL (short if, BOOLTHEN).
AST_node *AST_new_if(AST_node *cond, AST_node *then_block, AST_node *else_block,
                     size_t line) {
  AST_node *node = AST_new(AST_if, line);
  if (node == NULL) {
    AST_free(cond);
    AST_free(then_block);
    AST_free(else_block);
    return NULL;
  }
  node->as.if_stmt.then_block = then_block;
  node->as.if_stmt.else_block = else_block;
  node->as.if_stmt.cond = cond;
  return node;
}

AST_node *AST_new_while(AST_node *cond, AST_node *body, size_t line) {
  AST_node *node = AST_new(AST_while, line);
  if (node == NULL) {
    AST_free(cond);
    AST_free(body);
    return NULL;
  }
  node->as.while_stmt.body = body;
  node->as.while_stmt.cond = cond;
  return node;
}

// Creates a return node; value may be NULL.
AST_node *AST_new_return(AST_node *value, size_t line) {
  AST_node *node = AST_new(AST_return, line);
  if (node == NULL) {
    AST_free(value);
    return NULL;
  }
  node->as.return_stmt.value = value;
  return node;
}

AST_node *AST_new_expr_stmt(AST_node *expr, size_t line) {
  AST_node *node = AST_new(AST_expr_stmt, line);
  if (node == NULL) {
    AST_free(expr);
    return NULL;
  }
  node->as.expr_stmt.expr = expr;
  return node;
}

// Creates a block node; statements are added by the caller via AST_list_push.
AST_node *AST_new_block(size_t line) { return AST_new(AST_block, line); }

// Creates a program node; function definitions are added via AST_list_push.
AST_node *AST_new_program(size_t line) { return AST_new(AST_program, line); }

// Creates a function definition with return_type = TYPE_UNKNOWN; parameters,
// body, return type and flags are filled in by the parser.
AST_node *AST_new_func_def(const char *name, size_t line) {
  AST_node *node = AST_new(AST_func_def, line);
  if (node == NULL) {
    return NULL;
  }
  node->as.func_def.name = copy_string(name);
  if (node->as.func_def.name == NULL) {
    free(node);
    return NULL;
  }
  node->as.func_def.has_param_list = true;
  node->as.func_def.return_type = TYPE_UNKNOWN;
  return node;
}

/* Appends 'node' to 'list', doubling the capacity when the array is full.
   Returns false on allocation failure; in that case 'node' still belongs
   to the caller. */
bool AST_list_push(AST_node_list *list, AST_node *node) {
  if (list == NULL) {
    return false;
  }
  if (list->count == list->capacity) {
    size_t new_capacity = (list->capacity == 0) ? 4 : list->capacity * 2;
    AST_node **temp = realloc(list->items, new_capacity * sizeof(AST_node *));
    if (temp == NULL) {
      return false;
    }
    list->items = temp;
    list->capacity = new_capacity;
  }
  list->items[list->count] = node;
  list->count++;
  return true;
}

/* Appends a parameter (copied name, type, sym_id = -1) to a function
   definition. Returns false if 'func' is not a function definition or
   allocation fails. */
bool AST_func_add_param(AST_node *func, const char *name, Data_types type) {
  if (func == NULL || func->kind != AST_func_def) {
    return false;
  }
  char *copy = copy_string(name);
  if (copy == NULL) {
    return false;
  }
  if (func->as.func_def.param_count == func->as.func_def.param_capacity) {
    size_t new_capacity = (func->as.func_def.param_capacity == 0)
                              ? 4
                              : func->as.func_def.param_capacity * 2;
    AST_param *temp =
        realloc(func->as.func_def.params, new_capacity * sizeof(AST_param));
    if (temp == NULL) {
      free(copy);
      return false;
    }
    func->as.func_def.params = temp;
    func->as.func_def.param_capacity = new_capacity;
  }
  func->as.func_def.params[func->as.func_def.param_count].name = copy;
  func->as.func_def.params[func->as.func_def.param_count].type = type;
  func->as.func_def.params[func->as.func_def.param_count].sym_id =
      -1; // Initialize sym_id to -1 for the new parameter
  func->as.func_def.param_count++;
  return true;
}

/* Recursively frees 'node', its whole subtree and all owned strings.
   Does nothing if 'node' is NULL. */
void AST_free(AST_node *node) {
  if (node == NULL) {
    return;
  }
  switch (node->kind) {
  case AST_program:
    free_list(&node->as.program.funcs);
    break;
  case AST_func_def:
    free(node->as.func_def.name);
    for (size_t i = 0; i < node->as.func_def.param_count; i++) {
      free(node->as.func_def.params[i].name);
    }
    free(node->as.func_def.params);
    AST_free(node->as.func_def.body);
    break;
  case AST_block:
    free_list(&node->as.block.stmts);
    break;
  case AST_var_def:
    free(node->as.var_def.name);
    AST_free(node->as.var_def.init);
    break;
  case AST_assign:
    free(node->as.assign.name);
    AST_free(node->as.assign.value);
    break;
  case AST_if:
    AST_free(node->as.if_stmt.cond);
    AST_free(node->as.if_stmt.else_block);
    AST_free(node->as.if_stmt.then_block);
    break;
  case AST_while:
    AST_free(node->as.while_stmt.cond);
    AST_free(node->as.while_stmt.body);
    break;
  case AST_return:
    AST_free(node->as.return_stmt.value);
    break;
  case AST_expr_stmt:
    AST_free(node->as.expr_stmt.expr);
    break;
  case AST_string_lit:
    free(node->as.string_lit.value);
    break;
  case AST_id:
    free(node->as.id.name);
    break;
  case AST_binop:
    AST_free(node->as.binop.left);
    AST_free(node->as.binop.right);
    break;
  case AST_unop:
    AST_free(node->as.unop.operand);
    break;
  case AST_conv_int_to_double:
    AST_free(node->as.conv.operand);
    break;
  case AST_call:
    free(node->as.call.name);
    free_list(&node->as.call.args);
    break;
  case AST_member:
    AST_free(node->as.member.receiver);
    free(node->as.member.name);
    free_list(&node->as.member.args);
    break;
  case AST_int_lit:
  case AST_double_lit:
  case AST_bool_lit:
  case AST_unit_lit:
  case AST_for:
  case AST_if_expr:
    break;
  default:
    break;
  }
  free(node);
}

/* Returns the name of a node kind as a static string (must not be freed). */
const char *AST_kind_name(AST_node_type kind) {
  switch (kind) {
  case AST_program:
    return "PROGRAM";
  case AST_func_def:
    return "FUNC_DEF";
  case AST_block:
    return "BLOCK";
  case AST_var_def:
    return "VAR_DEF";
  case AST_assign:
    return "ASSIGN";
  case AST_if:
    return "IF";
  case AST_while:
    return "WHILE";
  case AST_return:
    return "RETURN";
  case AST_expr_stmt:
    return "EXPR_STMT";
  case AST_int_lit:
    return "INT_LIT";
  case AST_double_lit:
    return "DOUBLE_LIT";
  case AST_string_lit:
    return "STRING_LIT";
  case AST_bool_lit:
    return "BOOL_LIT";
  case AST_id:
    return "ID";
  case AST_binop:
    return "BINOP";
  case AST_unop:
    return "UNOP";
  case AST_call:
    return "CALL";
  case AST_conv_int_to_double:
    return "CONV_INT_TO_DOUBLE";
  case AST_unit_lit:
    return "UNIT_LIT";
  case AST_member:
    return "MEMBER";
  default:
    return "?";
  }
}

/* Returns the source symbol of an operator ("+", "<=", "!", ...)
   as a static string (must not be freed). */
const char *AST_op_name(AST_op_type op) {
  switch (op) {
  case OP_add:
    return "+";
  case OP_sub:
    return "-";
  case OP_mul:
    return "*";
  case OP_div:
    return "/";
  case OP_lt:
    return "<";
  case OP_le:
    return "<=";
  case OP_gt:
    return ">";
  case OP_ge:
    return ">=";
  case OP_eq:
    return "==";
  case OP_ne:
    return "!=";
  case OP_and:
    return "&&";
  case OP_or:
    return "||";
  case OP_neg:
    return "-";
  case OP_not:
    return "!";
  case OP_pos:
    return "+";
  default:
    return "?";
  }
}

// Prints 2 spaces per indentation level.
static void dump_indent(FILE *out, int depth) {
  for (int i = 0; i < depth; i++) {
    fputs("  ", out);
  }
}

/* Prints a string literal in double quotes; newline, tab, carriage return,
   quote and backslash are printed as escape sequences so the output stays
   on one line. */
static void dump_string(FILE *out, const char *s) {
  fputc('"', out);
  while (*s != '\0') {
    switch (*s) {
    case '\n':
      fputs("\\n", out);
      break;
    case '\t':
      fputs("\\t", out);
      break;
    case '\r':
      fputs("\\r", out);
      break;
    case '"':
      fputs("\\\"", out);
      break;
    case '\\':
      fputs("\\\\", out);
      break;
    default:
      fputc(*s, out);
      break;
    }
    s++;
  }
  fputc('"', out);
}

/* Prints an expression on a single line. Binary and unary operations are
   fully parenthesized, e.g. (2 + (3 * 4)). */
static void dump_expr(const AST_node *node, FILE *out) {
  if (node == NULL) {
    fputs("<null>", out);
    return;
  }
  switch (node->kind) {
  case AST_int_lit:
    fprintf(out, "%" PRId64, node->as.int_lit.value);
    break;
  case AST_double_lit:
    fprintf(out, "%g", node->as.double_lit.value);
    break;
  case AST_string_lit:
    dump_string(out, node->as.string_lit.value);
    break;
  case AST_bool_lit:
    fputs(node->as.bool_lit.value ? "true" : "false", out);
    break;
  case AST_unit_lit:
    fputs("()", out);
    break;
  case AST_id:
    fputs(node->as.id.name, out);
    break;
  case AST_binop:
    fputs("(", out);
    dump_expr(node->as.binop.left, out);
    fputs(" ", out);
    fputs(AST_op_name(node->as.binop.op), out);
    fputs(" ", out);
    dump_expr(node->as.binop.right, out);
    fputs(")", out);
    break;
  case AST_unop:
    fputs("(", out);
    fputs(AST_op_name(node->as.unop.op), out);
    dump_expr(node->as.unop.operand, out);
    fputs(")", out);
    break;
  case AST_call:
    fputs(node->as.call.name, out);
    fputs("(", out);
    for (size_t i = 0; i < node->as.call.args.count; i++) {
      if (i > 0) {
        fputs(", ", out);
      }
      dump_expr(node->as.call.args.items[i], out);
    }
    fputs(")", out);
    break;
  case AST_conv_int_to_double:
    fputs("int2double(", out);
    dump_expr(node->as.conv.operand, out);
    fputs(")", out);
    break;
  case AST_member:
    fputc('(', out);
    dump_expr(node->as.member.receiver, out);
    fputs(").", out);
    fputs(node->as.member.name, out);
    if (node->as.member.has_parens) {
      fputc('(', out);
      for (size_t i = 0; i < node->as.member.args.count; i++) {
        if (i > 0) {
          fputs(", ", out);
        }
        dump_expr(node->as.member.args.items[i], out);
      }
      fputc(')', out);
    }
    break;
  default:
    fputs("<not an expression>", out);
    break;
  }
}

// Prints the parameter list of a function definition, e.g. (a: Int, b: String).
static void dump_params(const AST_node *func, FILE *out) {
  fputs("(", out);
  for (size_t i = 0; i < func->as.func_def.param_count; i++) {
    if (i > 0) {
      fputs(", ", out);
    }
    fputs(func->as.func_def.params[i].name, out);
    fputs(": ", out);
    fputs(type_name(func->as.func_def.params[i].type), out);
  }
  fputs(")", out);
}

/* Prints a statement-level node and its children, one node per line,
   indented by 'depth'. Expressions are printed via dump_expr. */
static void dump_node(const AST_node *node, FILE *out, int depth) {
  if (node == NULL) {
    dump_indent(out, depth);
    fputs("<null>\n", out);
    return;
  }
  if (is_expr_kind(node->kind)) {
    dump_indent(out, depth);
    dump_expr(node, out);
    fputs("\n", out);
    return;
  }
  dump_indent(out, depth);
  switch (node->kind) {
  case AST_program:
    fputs("PROGRAM\n", out);
    for (size_t i = 0; i < node->as.program.funcs.count; i++) {
      dump_node(node->as.program.funcs.items[i], out, depth + 1);
    }
    break;
  case AST_func_def:
    fputs("FUNC_DEF ", out);
    fputs(node->as.func_def.name, out);
    dump_params(node, out);
    fputs(": ", out);
    fputs(type_name(node->as.func_def.return_type), out);
    if (node->as.func_def.is_extension == true) {
      fputs(" [ext]", out);
    }

    if (node->as.func_def.is_main) {
      fputs(" @main", out);
    }
    fputs("\n", out);
    dump_node(node->as.func_def.body, out, depth + 1);
    break;
  case AST_block:
    fputs("BLOCK\n", out);
    for (size_t i = 0; i < node->as.block.stmts.count; i++) {
      dump_node(node->as.block.stmts.items[i], out, depth + 1);
    }
    break;
  case AST_var_def:
    fputs("VAR_DEF ", out);
    fputs((node->as.var_def.is_mutable ? "var " : "val "), out);
    fputs(node->as.var_def.name, out);
    fputs(": ", out);
    fputs(type_name(node->as.var_def.decl_type), out);
    fputs("\n", out);
    dump_node(node->as.var_def.init, out, depth + 1);
    break;
  case AST_assign:
    fputs("ASSIGN ", out);
    fputs(node->as.assign.name, out);
    fputs("\n", out);
    dump_node(node->as.assign.value, out, depth + 1);
    break;
  case AST_if:
    fputs("IF\n", out);
    dump_node(node->as.if_stmt.cond, out, depth + 1);
    dump_indent(out, depth);
    fputs("THEN\n", out);
    dump_node(node->as.if_stmt.then_block, out, depth + 1);
    if (node->as.if_stmt.else_block != NULL) {
      dump_indent(out, depth);
      fputs("ELSE\n", out);
      dump_node(node->as.if_stmt.else_block, out, depth + 1);
    }
    break;
  case AST_while:
    fputs("WHILE\n", out);
    dump_node(node->as.while_stmt.cond, out, depth + 1);
    dump_node(node->as.while_stmt.body, out, depth + 1);
    break;
  case AST_return:
    fputs("RETURN\n", out);
    if (node->as.return_stmt.value != NULL) {
      dump_node(node->as.return_stmt.value, out, depth + 1);
    }
    break;
  case AST_expr_stmt:
    fputs("EXPR_STMT\n", out);
    dump_node(node->as.expr_stmt.expr, out, depth + 1);
    break;
  default:
    fputs(AST_kind_name(node->kind), out);
    fputs("\n", out);
    break;
  }
}

/* Prints a debug dump of the subtree rooted at 'node' to 'out'.
   Used for testing the parser; the format is described in ast.h. */
void AST_dump(const AST_node *node, FILE *out) {
  if (out == NULL) {
    return;
  }
  dump_node(node, out, 0);
}

// Creates a method call node; takes ownership of 'receiver'. Arguments are
// added by the caller via AST_list_push.
AST_node *AST_new_member(AST_node *receiver, const char *name, bool has_parens,
                         size_t line) {
  AST_node *node = AST_new(AST_member, line);
  if (node == NULL) {
    AST_free(receiver);
    return NULL;
  }
  node->as.member.name = copy_string(name);
  if (node->as.member.name == NULL) {
    AST_free(receiver);
    free(node);
    return NULL;
  }
  node->as.member.receiver = receiver;
  node->as.member.has_parens = has_parens;
  return node;
}

// Returns a newly allocated mangled name, e.g. "Int$twice", or NULL. The caller
// frees it.
char *AST_ext_name(Data_types type, const char *method) {
  const char *type_text = type_name(type);
  if (type == TYPE_UNKNOWN || type == TYPE_ANY || method == NULL) {
    return NULL;
  }
  size_t length = strlen(type_text) + 1 + strlen(method) + 1;
  char *result = malloc(length);
  if (result == NULL) {
    return NULL;
  }
  snprintf(result, length, "%s$%s", type_text, method);
  return result;
}
