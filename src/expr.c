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
