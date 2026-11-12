/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       ast.h
   # Brief:      Abstract syntax tree definitions
   # Authors:    Damian Volosiny (xvolosd00)
*/
#ifndef AST_H
#define AST_H
#include "types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type of node, according to which the union member is selected
typedef enum {
  AST_program,
  AST_func_def,
  AST_block,
  AST_var_def,
  AST_assign,
  AST_if,
  AST_while,
  AST_return,
  AST_expr_stmt,
  AST_int_lit,
  AST_double_lit,
  AST_string_lit,
  AST_unit_lit,
  AST_bool_lit,
  AST_id,
  AST_binop,
  AST_unop,
  AST_call,
  AST_conv_int_to_double,
  // Reserved for extensions, not produced yet
  AST_for,
  AST_if_expr,
  AST_member,
} AST_node_type;

// Operators independent of token types
typedef enum {
  // Arithmetic operators
  OP_add, // +
  OP_sub, // -
  OP_mul, // *
  OP_div, // /
          // Relational operators
  OP_lt,  // <
  OP_le,  // <=
  OP_gt,  // >
  OP_ge,  // >=
  OP_eq,  // ==
  OP_ne,  // !=
          // Logical Operators
  OP_and, // &&
  OP_or,  // ||
          // Unary Operators
  OP_neg, // -x
  OP_not, // !
  OP_pos, // +x
} AST_op_type;

// Allows structures to refer to a node before it is defined
typedef struct AST_node AST_node;

// A dynamic array of pointers to nodes
typedef struct {
  AST_node **items;
  size_t count;
  size_t capacity;
} AST_node_list;

// One function parameter.
typedef struct {
  char *name;      // Name is a copy
  Data_types type; // Type is the declared type
  int sym_id; // -1 means unassigned. The semantics assign a unique number to
              // the symbol, which the generator uses to distinguish shadowed
              // variables
} AST_param;

typedef struct {
  AST_op_type op;
  AST_node *left;
  AST_node *right;
} AST_binop_data; // Left and right are never NULL

typedef struct {
  char *name;
  AST_param *params; // params can be  NULL when there are 0 parameters
  size_t param_count;
  size_t param_capacity;
  Data_types return_type; // return_type is UNKNOWN if not specified
  bool return_explicit;   // False if the type is not specified.
  bool is_main;           // is_main corresponds to the @main annotation
  AST_node *body;         // body is a BLOCK node
} AST_func_def_data;

typedef struct {
  AST_node_list stmts;
} AST_block_data; // Commands in the order they appear in the source code. Each
                  // block constitutes a separate scope

typedef struct {
  char *name;
  bool is_mutable;      // is_mutable is true for var, false for val.
  Data_types decl_type; // decl_type is UNKNOWN if the type is not specified
  AST_node *init;       // init is never NULL
} AST_var_def_data;

typedef struct {
  char *name;
  AST_node *value;
} AST_assign_data; // Name is the target variable. Value is never NULL

typedef struct {
  AST_node *cond;
  AST_node *then_block;
  AST_node *else_block;
} AST_if_data; // Both then_block and else_block are BLOCK nodes. Else_block is
               // NULL only in the case of a short-form if(BOOLTHEN).

typedef struct {
  AST_node *cond;
  AST_node *body;
} AST_while_data; // Cond is a condition, and body is a BLOCK node

typedef struct {
  AST_node *value;
} AST_return_data; // Value can be NULL

typedef struct {
  AST_node *expr;
} AST_expr_stmt_data; // An expression or call as a standalone statement. The
                      // value is discarded, unless it is the result of a
                      // function

typedef struct {
  int64_t value;
} AST_int_lit_data; // Always a non-negative value. The minus sign is a separate
                    // unary operator.

typedef struct {
  double value;
} AST_double_lit_data;

typedef struct {
  char *value;
} AST_string_lit_data; // Escape sequences are already processed by the lexer.
                       // The string is a copy

typedef struct {
  bool value;
} AST_bool_lit_data; // For BOOLTHEN only

typedef struct {
  char *name;
} AST_id_data; // Variable name (Copy)

typedef struct {
  AST_op_type op;
  AST_node *operand;
} AST_unop_data; // Operator is only neg, not or pos

typedef struct {
  char *name;
  AST_node_list args;
} AST_call_data; // Function name and arguments in order from left to right.
                 // Without FUNEXP arguments are only literals or identifiers.

typedef struct {
  AST_node *operand;
} AST_conv_int_to_double_data; // Implicit conversion from Int to Double. It is
                               // inserted only by semantic analysis around an
                               // expression of type Int

typedef struct {
  AST_node_list funcs;
} AST_program_data; // Functions in the order they appear in the source code

struct AST_node {
  AST_node_type kind; // Determines which member of the as is valid
  size_t line;        // Source code line (starting from 1), for error messages
  Data_types data_type; // Parser sets UNKNOWN, semantics completes the type of
                        // expression or symbol
  int sym_id; // -1 means unassigned. The semantics assign a unique number to
              // the symbol, which the generator uses to distinguish shadowed
              // variables
  bool is_result; // Parser sets it to false. The semantics identifies the
                  // expression whose value is the function's return value

  union {
    AST_program_data program;
    AST_func_def_data func_def;
    AST_block_data block;
    AST_var_def_data var_def;
    AST_assign_data assign;
    AST_if_data if_stmt;
    AST_while_data while_stmt;
    AST_return_data return_stmt;
    AST_expr_stmt_data expr_stmt;
    AST_int_lit_data int_lit;
    AST_double_lit_data double_lit;
    AST_string_lit_data string_lit;
    AST_bool_lit_data bool_lit;
    AST_id_data id;
    AST_binop_data binop;
    AST_unop_data unop;
    AST_call_data call;
    AST_conv_int_to_double_data conv;
  } as; // Payload depends on the kind. AST_unit_lit has no payload;
};

/*Constructors :    Return new node or NULL if allocation fails
                    Set common fields to UNKNOWN / -1 / false
                    Copy strings (name, value)
                    Take ownership of child nodes and free them in the event of
a failure

For AST_new_call, AST_new_block, AST_new_program : Arguments, commands,
functions are added by the caller via AST_list_push For AST_new_func_def parser
adds parameters via AST_func_add_param, as well as the return type and flags
*/

AST_node *AST_new(AST_node_type kind, size_t line);
AST_node *AST_new_int_lit(int64_t value, size_t line);
AST_node *AST_new_double_lit(double value, size_t line);
AST_node *AST_new_string_lit(const char *value, size_t line);
AST_node *AST_new_bool_lit(bool value, size_t line);
AST_node *AST_new_unit_lit(size_t line);
AST_node *AST_new_id(const char *name, size_t line);
AST_node *AST_new_binop(AST_op_type op, AST_node *left, AST_node *right,
                        size_t line);
AST_node *AST_new_unop(AST_op_type op, AST_node *operand, size_t line);
AST_node *AST_new_call(const char *name, size_t line);
AST_node *AST_new_conv(AST_node *operand, size_t line);
AST_node *AST_new_var_def(const char *name, bool is_mutable,
                          Data_types decl_type, AST_node *init, size_t line);
AST_node *AST_new_assign(const char *name, AST_node *value, size_t line);
AST_node *AST_new_if(AST_node *cond, AST_node *then_block, AST_node *else_block,
                     size_t line); // else_block can be NULL
AST_node *AST_new_while(AST_node *cond, AST_node *body, size_t line);
AST_node *AST_new_return(AST_node *value, size_t line); // value can be NULL
AST_node *AST_new_expr_stmt(AST_node *expr, size_t line);
AST_node *AST_new_block(size_t line);
AST_node *AST_new_program(size_t line);
AST_node *AST_new_func_def(const char *name, size_t line);
bool AST_list_push(
    AST_node_list *list,
    AST_node *node); // True on success, false on allocation failure. In case of
                     // failure, the node still belongs to the caller, who must
                     // release it.
bool AST_func_add_param(
    AST_node *func, const char *name,
    Data_types type);          // Copies the name, true/false, just like push.
void AST_free(AST_node *node); // Recursively frees the node, the entire
                               // subtree, and all strings; NULL is allowed.
void AST_dump(
    const AST_node *node,
    FILE *out); // Debugging output of the subtree to 'out'. Expressions are
                // fully parenthesized, e.g. (2 + (3 * 4)).
// Return the name as a string, e.g. for dumps and error messages. The returned
// string must not be freed.
const char *AST_kind_name(AST_node_type kind);
const char *AST_op_name(AST_op_type op);
#ifdef __cplusplus
}
#endif

#endif
