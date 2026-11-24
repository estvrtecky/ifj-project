/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       expr.c
   # Brief:      Expression implementation.
   # Authors:    Damian Volosiny (xvolosd00)
*/

/* Comments in this file were drafted with AI assistance (Claude).
   Prompt: "Vies mi pomoct s komentarmi pre expr.c" */

#include "expr.h"
#include <stdlib.h>
#include <string.h>

/* Terminals of the precedence table. The order must match the rows and
   columns of Action_precedence_table. Unary + and - are separate terminals,
   'i' stands for any term (literal incl. true/false, identifier, function call)
 */
typedef enum
{
  T_Unary_plus,               // U+
  T_Unary_minus,              // U-
  T_Not,                      // !
  T_Addition,                 // +
  T_Subtraction,              // -
  T_Multiplication,           // *
  T_Division,                 // /
  T_Greater_than,             // >
  T_Less_than,                // <
  T_Greater_than_or_equal_to, // >=
  T_Less_than_or_equal_to,    // <=
  T_Equal_to,                 // ==
  T_Not_equal_to,             // !=
  T_And,                      // &&
  T_Or,                       // ||
  T_Left_parenthesis,         // (
  T_Right_parenthesis,        // )
  T_Letter_i,                 // i (term: literal, identifier, call)
  T_Dollar_sign,              // $ (end of input)
} Terminal;

/* Actions of the precedence table:
   A_Shift_with_sign    '<'  push a handle mark and shift the input terminal
   A_Shift_without_sign '='  shift the input terminal without a mark
   A_Reduction          '>'  reduce the handle on top of the stack
   A_Error                   syntax error */
typedef enum
{
  A_Shift_with_sign,
  A_Shift_without_sign,
  A_Reduction,
  A_Error,
} Action;

// Kind of a stack item: terminal, nonterminal E, or handle mark '<'.
typedef enum
{
  ITEM_TERM,
  ITEM_NONTERM,
  ITEM_MARK
} Item_type;

/* One item of the precedence stack.
   term - valid only for ITEM_TERM.
   node - for terminal 'i' the AST leaf created when it was shifted,
          for E the AST subtree, otherwise NULL. Owned by the stack. */
typedef struct
{
  Item_type kind;
  Terminal term;
  AST_node *node;
} Stack_item;

// Dynamic array used as the precedence analysis stack.
typedef struct
{
  Stack_item *items;
  size_t count;
  size_t capacity;
} Stack;

/* Precedence table: row = topmost terminal on the stack,
   column = input terminal. Both are indexed by Terminal. */
static const Action Action_precedence_table[19][19] = {
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction}, // U+
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction}, // U-
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction}, // !
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction,
     A_Shift_with_sign, A_Reduction}, // +
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction,
     A_Shift_with_sign, A_Reduction}, // -
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction}, // *
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction}, // /
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction}, // >
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction}, // <
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction}, // >=
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction}, // <=
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction,
     A_Shift_with_sign, A_Reduction}, // ==
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error,
     A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction,
     A_Shift_with_sign, A_Reduction}, // !=
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Reduction, A_Reduction, A_Shift_with_sign,
     A_Reduction, A_Shift_with_sign, A_Reduction}, // &&
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Shift_with_sign,
     A_Reduction, A_Shift_with_sign, A_Reduction}, // ||
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_without_sign, A_Shift_with_sign, A_Error}, // (
    {A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Error, A_Reduction,
     A_Error, A_Reduction}, // )
    {A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction,
     A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Error, A_Reduction,
     A_Error, A_Reduction}, // i
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign,
     A_Error, A_Shift_with_sign, A_Error} // $
};

// Initializes an empty stack.
static void stack_init(Stack *s)
{
  s->items = NULL;
  s->count = 0;
  s->capacity = 0;
}

/* Pushes 'item' on top of the stack, doubling the capacity when full.
   Returns false on allocation failure. */
static bool stack_push(Stack *s, Stack_item item)
{
  if (s->count == s->capacity)
  {
    size_t new_capacity = (s->capacity == 0) ? 16 : s->capacity * 2;
    Stack_item *temp = realloc(s->items, new_capacity * sizeof(Stack_item));
    if (temp == NULL)
    {
      return false;
    }
    s->items = temp;
    s->capacity = new_capacity;
  }
  s->items[s->count] = item;
  s->count++;
  return true;
}

// Creates a terminal item carrying an optional AST node.
static Stack_item make_term(Terminal t, AST_node *node)
{
  Stack_item item;
  item.kind = ITEM_TERM;
  item.term = t;
  item.node = node;
  return item;
}

// Creates a nonterminal (E) item carrying an AST subtree.
static Stack_item make_nonterm(AST_node *node)
{
  Stack_item item;
  item.kind = ITEM_NONTERM;
  item.term = T_Dollar_sign;
  item.node = node;
  return item;
}

// Creates a handle mark item.
static Stack_item make_mark(void)
{
  Stack_item item;
  item.kind = ITEM_MARK;
  item.term = T_Dollar_sign;
  item.node = NULL;
  return item;
}

/* Returns the index of the topmost terminal (nonterminals and marks are
   skipped), or -1 if there is none. */
static int stack_top_terminal(const Stack *s)
{
  for (int i = (int)s->count - 1; i >= 0; i--)
  {
    if (s->items[i].kind == ITEM_TERM)
    {
      return i;
    }
  }
  return -1;
}

/* Inserts a handle mark directly above the item at index 'idx'
   (the topmost terminal); items above it are shifted up by one.
   Returns false on allocation failure. */
static bool stack_insert_mark_after(Stack *s, int idx)
{
  if (!stack_push(s, make_mark()))
  {
    return false;
  }
  for (int i = (int)s->count - 1; i >= idx + 2; i--)
  {
    s->items[i] = s->items[i - 1];
  }
  s->items[idx + 1] = make_mark();
  return true;
}

// Returns the index of the topmost handle mark, or -1 if there is none.
static int stack_find_mark(const Stack *s)
{
  for (int i = (int)s->count - 1; i >= 0; i--)
  {
    if (s->items[i].kind == ITEM_MARK)
    {
      return i;
    }
  }
  return -1;
}

// Frees all AST nodes still owned by the stack and the stack itself.
static void stack_free(Stack *s)
{
  for (size_t i = 0; i < s->count; i++)
  {
    AST_free(s->items[i].node);
  }
  free(s->items);
  stack_init(s);
}

/* Converts a token to a terminal of the precedence table.
   + and - are unary when 'unary_allowed' is true (start of the expression,
   after an operator or '('). ')' at depth 0 and every token that cannot be
   part of an expression are treated as $ (end of the expression).
   Has no side effects. */
static Terminal token_to_terminal(const token_t *tok, int depth,
                                  bool unary_allowed)
{
  switch (tok->type)
  {
  case TOKEN_ADD:
    return unary_allowed ? T_Unary_plus : T_Addition;
    break;
  case TOKEN_SUB:
    return unary_allowed ? T_Unary_minus : T_Subtraction;
    break;
  case TOKEN_MUL:
    return T_Multiplication;
    break;
  case TOKEN_DIV:
    return T_Division;
    break;
  case TOKEN_GT:
    return T_Greater_than;
    break;
  case TOKEN_LT:
    return T_Less_than;
    break;
  case TOKEN_GE:
    return T_Greater_than_or_equal_to;
    break;
  case TOKEN_LE:
    return T_Less_than_or_equal_to;
    break;
  case TOKEN_EQ:
    return T_Equal_to;
    break;
  case TOKEN_NE:
    return T_Not_equal_to;
    break;
  case TOKEN_L_PAREN:
    return T_Left_parenthesis;
    break;
  case TOKEN_R_PAREN:
    return depth == 0 ? T_Dollar_sign : T_Right_parenthesis;
    break;
  case TOKEN_NOT:
    return T_Not;
    break;
  case TOKEN_AND:
    return T_And;
    break;
  case TOKEN_OR:
    return T_Or;
    break;
  case TOKEN_TRUE:
  case TOKEN_FALSE:
  case TOKEN_INT:
  case TOKEN_DOUBLE:
  case TOKEN_STRING:
  case TOKEN_ID:
    return T_Letter_i;
    break;
  default:
    return T_Dollar_sign;
    break;
  }
}

// Converts an operator terminal to the corresponding AST operator.
static AST_op_type token_to_op(Terminal t)
{
  switch (t)
  {
  case T_Addition:
    return OP_add;
    break;
  case T_Subtraction:
    return OP_sub;
    break;
  case T_Multiplication:
    return OP_mul;
    break;
  case T_Division:
    return OP_div;
    break;
  case T_Greater_than:
    return OP_gt;
    break;
  case T_Less_than:
    return OP_lt;
    break;
  case T_Greater_than_or_equal_to:
    return OP_ge;
    break;
  case T_Less_than_or_equal_to:
    return OP_le;
    break;
  case T_Equal_to:
    return OP_eq;
    break;
  case T_Not_equal_to:
    return OP_ne;
    break;
  case T_Unary_minus:
    return OP_neg;
    break;
  case T_Unary_plus:
    return OP_pos;
    break;
  case T_And:
    return OP_and;
    break;
  case T_Or:
    return OP_or;
    break;
  case T_Not:
    return OP_not;
    break;
  default:
    return OP_add; // Never happens
    break;
  }
}

/* Creates an AST leaf (literal or identifier) from 'tok'. The string
   attribute of the token is freed after copying and set to NULL.
   Returns OK, or ERROR_INTERNAL on allocation failure. */
static ErrorCode make_leaf(token_t *tok, AST_node **out)
{
  size_t line = (size_t)tok->line;
  *out = NULL;
  switch (tok->type)
  {
  case TOKEN_INT:
    *out = AST_new_int_lit(tok->attribute.i, line);
    break;
  case TOKEN_DOUBLE:
    *out = AST_new_double_lit(tok->attribute.d, line);
    break;
  case TOKEN_STRING:
    *out = AST_new_string_lit(tok->attribute.s, line);
    free(tok->attribute.s);
    tok->attribute.s = NULL;
    break;
  case TOKEN_ID:
    *out = AST_new_id(tok->attribute.s, line);
    free(tok->attribute.s);
    tok->attribute.s = NULL;
    break;
  case TOKEN_TRUE:
    *out = AST_new_bool_lit(true, line);
    break;
  case TOKEN_FALSE:
    *out = AST_new_bool_lit(false, line);
    break;
  default:
    return ERROR_SYNTAX;
    break;
  }
  if (*out == NULL)
  {
    return ERROR_INTERNAL;
  }
  return OK;
}

// Returns true if 't' is a binary operator terminal.
static bool is_binary_op(Terminal t) { return t >= T_Addition && t <= T_Or; }

/* Reduces the handle (items above the topmost mark) using one of the rules:
     E -> i        E -> ( )      E -> ( E )       E -> !E
     E -> -E       E -> +E       E -> E op E
   The handle and the mark are replaced by a new nonterminal E with the
   corresponding AST node. 'line' is used for the Unit literal.
   Returns ERROR_SYNTAX if no rule matches, ERROR_INTERNAL on allocation
   failure. */
static ErrorCode reduce(Stack *s, size_t line)
{
  int mark_index = stack_find_mark(s);
  if (mark_index < 0)
  {
    return ERROR_SYNTAX;
  }

  size_t handle_size = s->count - (size_t)(mark_index + 1);
  Stack_item *handle = &s->items[mark_index + 1];
  AST_node *new_node = NULL;
  if (handle_size == 1 && handle[0].kind == ITEM_TERM &&
      handle[0].term == T_Letter_i)
  {
    new_node = handle[0].node;
    handle[0].node = NULL;
  }
  else if (handle_size == 2 && handle[0].kind == ITEM_TERM &&
           handle[0].term == T_Left_parenthesis &&
           handle[1].kind == ITEM_TERM &&
           handle[1].term == T_Right_parenthesis)
  {
    new_node = AST_new_unit_lit(line);
    if (new_node == NULL)
    {
      return ERROR_INTERNAL;
    }
  }
  else if (handle_size == 2 && handle[0].kind == ITEM_TERM &&
           (handle[0].term == T_Unary_minus ||
            handle[0].term == T_Unary_plus || handle[0].term == T_Not) &&
           handle[1].kind == ITEM_NONTERM)
  {
    AST_node *op = handle[1].node;
    handle[1].node = NULL;
    new_node = AST_new_unop(token_to_op(handle[0].term), op, op->line);
    if (new_node == NULL)
    {
      return ERROR_INTERNAL;
    }
  }
  else if (handle_size == 3 && handle[0].kind == ITEM_TERM &&
           handle[0].term == T_Left_parenthesis &&
           handle[1].kind == ITEM_NONTERM && handle[2].kind == ITEM_TERM &&
           handle[2].term == T_Right_parenthesis)
  {
    new_node = handle[1].node;
    handle[1].node = NULL;
  }
  else if (handle_size == 3 && handle[0].kind == ITEM_NONTERM &&
           handle[1].kind == ITEM_TERM && is_binary_op(handle[1].term) &&
           handle[2].kind == ITEM_NONTERM)
  {
    AST_node *left = handle[0].node;
    AST_node *right = handle[2].node;
    handle[0].node = NULL;
    handle[2].node = NULL;
    new_node =
        AST_new_binop(token_to_op(handle[1].term), left, right, left->line);
    if (new_node == NULL)
    {
      return ERROR_INTERNAL;
    }
  }
  else
  {
    return ERROR_SYNTAX;
  }
  s->count = (size_t)mark_index;
  if (!stack_push(s, make_nonterm(new_node)))
  {
    AST_free(new_node);
    return ERROR_INTERNAL;
  }
  return OK;
}

// Parses call arguments up to and including the closing ')'.
// On entry 'token' is the first token after '('.
// On exit 'token' is the first token after ')'.
// Arguments are appended to 'args'. On error the caller frees the node that
// owns 'args'.
static ErrorCode parse_args(token_t *token, AST_node_list *args)
{
  if (token->type == TOKEN_R_PAREN)
  {
    ErrorCode err = (ErrorCode)get_next_token(token);
    if (err != OK)
    {
      return err;
    }
    return OK;
  }

  while (true)
  {
    AST_node *arg = NULL;
    ErrorCode err = expr_parse(token, &arg);
    if (err != OK)
    {
      return err;
    }
    if (!AST_list_push(args, arg))
    {
      AST_free(arg);
      return ERROR_INTERNAL;
    }
    if (token->type == TOKEN_COMMA)
    {
      err = (ErrorCode)get_next_token(token);
      if (err != OK)
      {
        return err;
      }
      if (token->type == TOKEN_R_PAREN)
      {
        err = (ErrorCode)get_next_token(token);
        if (err != OK)
        {
          return err;
        }
        break;
      }
      continue;
    }
    if (token->type == TOKEN_R_PAREN)
    {
      err = (ErrorCode)get_next_token(token);
      if (err != OK)
      {
        return err;
      }
      break;
    }
    return ERROR_SYNTAX;
  }
  return OK;
}

// Wraps 'term' into AST_member nodes while the next token is '.'.
// On entry 'token' is the first token after the term.
// On exit 'token' is the first token after the whole chain.
// *term always holds a valid tree or NULL; on error the caller frees it.
static ErrorCode parse_member_chain(token_t *token, AST_node **term)
{
  while (token->type == TOKEN_DOT)
  {
    int line = token->line;
    ErrorCode err = (ErrorCode)get_next_token(token);
    if (err != OK)
    {
      return err;
    }
    if (token->type != TOKEN_ID)
    {
      return ERROR_SYNTAX;
    }
    char *name = token->attribute.s;
    token->attribute.s = NULL;
    err = (ErrorCode)get_next_token(token);
    if (err != OK)
    {
      free(name);
      return err;
    }
    bool has_parens = (token->type == TOKEN_L_PAREN);
    AST_node *member = AST_new_member(*term, name, has_parens, line);
    free(name);
    if (member == NULL)
    {
      *term = NULL;
      return ERROR_INTERNAL;
    }
    *term = member;

    if (has_parens)
    {
      err = (ErrorCode)get_next_token(token);
      if (err != OK)
      {
        return err;
      }
      err = parse_args(token, &member->as.member.args);
      if (err != OK)
      {
        return err;
      }
    }
  }
  return OK;
}

static ErrorCode parse_call(token_t *id_token, token_t *token, AST_node **out)
{
  *out = NULL;

  AST_node *call = AST_new_call(id_token->attribute.s, (size_t)id_token->line);
  free(id_token->attribute.s);
  id_token->attribute.s = NULL;
  if (call == NULL)
  {
    return ERROR_INTERNAL;
  }

  ErrorCode err = (ErrorCode)get_next_token(token);
  if (err != OK)
  {
    AST_free(call);
    return err;
  }

  err = parse_args(token, &call->as.call.args);
  if (err != OK)
  {
    AST_free(call);
    return err;
  }

  *out = call;
  return OK;
}

static ErrorCode make_id_or_call(token_t *id_token, token_t *token,
                                 AST_node **out)
{
  if (token->type == TOKEN_L_PAREN)
  {
    return parse_call(id_token, token, out);
  }
  return make_leaf(id_token, out);
}

/* Precedence syntax analysis of one expression. See expr.h for the
   interface (input/output token, result ownership, return values). */
static ErrorCode parse_expression(token_t *token, token_t *first_id,
                                  AST_node **result)
{
  *result = NULL;
  Stack stack;
  stack_init(&stack);
  ErrorCode err = OK;
  if (!stack_push(&stack, make_term(T_Dollar_sign, NULL)))
  {
    if (first_id != NULL)
    {
      free(first_id->attribute.s);
      first_id->attribute.s = NULL;
    }
    return ERROR_INTERNAL;
  }

  int depth = 0;
  bool unary_allowed = true;

  if (first_id != NULL)
  {
    AST_node *leaf = NULL;
    err = make_id_or_call(first_id, token, &leaf);
    if (err != OK)
    {
      goto cleanup;
    }
    err = parse_member_chain(token, &leaf);
    if (err != OK)
    {
      AST_free(leaf);
      goto cleanup;
    }
    if (!stack_insert_mark_after(&stack, 0))
    {
      AST_free(leaf);
      err = ERROR_INTERNAL;
      goto cleanup;
    }
    if (!stack_push(&stack, make_term(T_Letter_i, leaf)))
    {
      AST_free(leaf);
      err = ERROR_INTERNAL;
      goto cleanup;
    }
    unary_allowed = false;
  }

  while (true)
  {
    Terminal input_terminal = token_to_terminal(token, depth, unary_allowed);
    int top_index = stack_top_terminal(&stack);
    if (top_index < 0)
    {
      err = ERROR_INTERNAL;
      goto cleanup;
    }
    Terminal top_terminal = stack.items[top_index].term;
    if (top_terminal == T_Dollar_sign && input_terminal == T_Dollar_sign)
    {
      if (stack.count == 2 && stack.items[1].kind == ITEM_NONTERM)
      {
        *result = stack.items[1].node;
        stack.items[1].node = NULL;
        stack_free(&stack);
        return OK;
      }
      err = ERROR_SYNTAX;
      goto cleanup;
    }
    Action action = Action_precedence_table[top_terminal][input_terminal];
    if (action == A_Error)
    {
      err = ERROR_SYNTAX;
      goto cleanup;
    }
    if (action == A_Reduction)
    {
      err = reduce(&stack, (size_t)token->line);
      if (err != OK)
      {
        goto cleanup;
      }
      continue;
    }
    if (action == A_Shift_with_sign)
    {
      if (!stack_insert_mark_after(&stack, top_index))
      {
        err = ERROR_INTERNAL;
        goto cleanup;
      }
    }
    AST_node *leaf = NULL;
    if (input_terminal == T_Letter_i && token->type == TOKEN_ID)
    {
      token_t id_token = *token;
      token->attribute.s = NULL;
      err = get_next_token(token);
      if (err != OK)
      {
        free(id_token.attribute.s);
        goto cleanup;
      }
      err = make_id_or_call(&id_token, token, &leaf);
      if (err != OK)
      {
        goto cleanup;
      }
      err = parse_member_chain(token, &leaf);
      if (err != OK)
      {
        AST_free(leaf);
        goto cleanup;
      }
      if (!stack_push(&stack, make_term(T_Letter_i, leaf)))
      {
        AST_free(leaf);
        err = ERROR_INTERNAL;
        goto cleanup;
      }
      unary_allowed = false;
      continue;
    }
    if (input_terminal == T_Letter_i)
    {
      err = make_leaf(token, &leaf);
      if (err != OK)
      {
        goto cleanup;
      }
      err = get_next_token(token);
      if (err != OK)
      {
        AST_free(leaf);
        goto cleanup;
      }
      err = parse_member_chain(token, &leaf);
      if (err != OK)
      {
        AST_free(leaf);
        goto cleanup;
      }
      if (!stack_push(&stack, make_term(T_Letter_i, leaf)))
      {
        err = ERROR_INTERNAL;
        goto cleanup;
      }
      unary_allowed = false;
      continue;
    }

    if (!stack_push(&stack, make_term(input_terminal, leaf)))
    {
      AST_free(leaf);
      err = ERROR_INTERNAL;
      goto cleanup;
    }
    if (input_terminal == T_Left_parenthesis)
    {
      depth++;
    }
    if (input_terminal == T_Right_parenthesis)
    {
      depth--;
    }
    unary_allowed = !(input_terminal == T_Letter_i ||
                      input_terminal == T_Right_parenthesis);
    err = (ErrorCode)get_next_token(token);
    if (err != OK)
    {
      goto cleanup;
    }
  }
  // Error path: free everything still on the stack
cleanup:
  stack_free(&stack);
  return err;
}

ErrorCode expr_parse(token_t *token, AST_node **result)
{
  return parse_expression(token, NULL, result);
}

ErrorCode expr_parse_after_id(token_t *id_token, token_t *token,
                              AST_node **result)
{
  return parse_expression(token, id_token, result);
}
