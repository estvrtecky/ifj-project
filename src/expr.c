/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       expr.c
   # Brief:      Expression implementation.
   # Authors:    Damian Volosiny (xvolosd00)
*/

#include "expr.h"
#include <stdlib.h>
#include <string.h>

typedef enum
{
    T_Unary_plus,               // U+
    T_Unary_minus,              // U-
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
    T_Left_parenthesis,         // (
    T_Right_parenthesis,        // )
    T_Letter_i,                 // i (term: literal, identifier, call)
    T_Dollar_sign,              // $ (end of input)
} Terminal;

typedef enum
{
    A_Shift_with_sign,
    A_Shift_without_sign,
    A_Reduction,
    A_Error,
} Action;

typedef enum
{
    ITEM_TERM,
    ITEM_NONTERM,
    ITEM_MARK
} Item_type;

typedef struct
{
    Item_type kind;
    Terminal term;
    AST_node *node;
} Stack_item;

typedef struct
{
    Stack_item *items;
    size_t count;
    size_t capacity;
} Stack;

static const Action Action_precedence_table[16][16] = {
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Error, A_Error, A_Reduction, A_Reduction, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Error, A_Shift_with_sign, A_Reduction, A_Shift_with_sign, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_without_sign, A_Shift_with_sign, A_Error},
    {A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Error, A_Reduction, A_Error, A_Reduction},
    {A_Error, A_Error, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Reduction, A_Error, A_Reduction, A_Error, A_Reduction},
    {A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Shift_with_sign, A_Error, A_Shift_with_sign, A_Error}};

static void stack_init(Stack *s)
{
    s->items = NULL;
    s->count = 0;
    s->capacity = 0;
}

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

static Stack_item make_term(Terminal t, AST_node *node)
{
    Stack_item item;
    item.kind = ITEM_TERM;
    item.term = t;
    item.node = node;
    return item;
}

static Stack_item make_nonterm(AST_node *node)
{
    Stack_item item;
    item.kind = ITEM_NONTERM;
    item.term = T_Dollar_sign;
    item.node = node;
    return item;
}

static Stack_item make_mark(void)
{
    Stack_item item;
    item.kind = ITEM_MARK;
    item.term = T_Dollar_sign;
    item.node = NULL;
    return item;
}

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

static void stack_free(Stack *s)
{
    for (size_t i = 0; i < s->count; i++)
    {
        AST_free(s->items[i].node);
    }
    free(s->items);
    stack_init(s);
}

static Terminal token_to_terminal(const token_t *tok, int depth, bool unary_allowed)
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
    default:
        return OP_add; // Never happens
        break;
    }
}

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
