/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       ast.c
   # Brief:      Abstract syntax tree implementation.
   # Authors:    Damian Volosiny (xvolosd00)
*/

#include "ast.h"
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *source)
{
    if (source == NULL)
    {
        return NULL;
    }
    size_t source_len = strlen(source);
    char *source_al = malloc(source_len + 1);
    if (source_al == NULL)
    {
        return NULL;
    }
    memcpy(source_al, source, source_len + 1);
    return source_al;
}

static void free_list(AST_node_list *list)
{
    for (size_t i = 0; i < list->count; i++)
    {
        AST_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static const char *type_name(Data_types type)
{
    switch (type)
    {
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

static bool is_expr_kind(AST_node_type type)
{
    switch (type)
    {
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

        return true;

    default:
        return false;
    }
}

AST_node *AST_new(AST_node_type kind, size_t line)
{
    AST_node *node = calloc(1, sizeof *node);
    if (node == NULL)
    {
        return NULL;
    }
    node->kind = kind;
    node->line = line;
    node->data_type = TYPE_UNKNOWN;
    node->sym_id = -1;
    node->is_result = false;
    return node;
}

AST_node *AST_new_int_lit(int64_t value, size_t line)
{
    AST_node *node = AST_new(AST_int_lit, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.int_lit.value = value;
    return node;
}

AST_node *AST_new_double_lit(double value, size_t line)
{
    AST_node *node = AST_new(AST_double_lit, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.double_lit.value = value;
    return node;
}

AST_node *AST_new_string_lit(const char *value, size_t line)
{
    AST_node *node = AST_new(AST_string_lit, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.string_lit.value = copy_string(value);
    if (node->as.string_lit.value == NULL)
    {
        free(node);
        return NULL;
    }
    return node;
}

AST_node *AST_new_bool_lit(bool value, size_t line)
{
    AST_node *node = AST_new(AST_bool_lit, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.bool_lit.value = value;
    return node;
}

AST_node *AST_new_unit_lit(size_t line) { return AST_new(AST_unit_lit, line); }

AST_node *AST_new_id(const char *name, size_t line)
{
    AST_node *node = AST_new(AST_id, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.id.name = copy_string(name);
    if (node->as.id.name == NULL)
    {
        free(node);
        return NULL;
    }
    return node;
}

AST_node *AST_new_binop(AST_op_type op, AST_node *left, AST_node *right,
                        size_t line)
{
    AST_node *node = AST_new(AST_binop, line);
    if (node == NULL)
    {
        AST_free(left);
        AST_free(right);
        return NULL;
    }
    node->as.binop.op = op;
    node->as.binop.left = left;
    node->as.binop.right = right;
    return node;
}

AST_node *AST_new_unop(AST_op_type op, AST_node *operand, size_t line)
{
    AST_node *node = AST_new(AST_unop, line);
    if (node == NULL)
    {
        AST_free(operand);
        return NULL;
    }
    node->as.unop.op = op;
    node->as.unop.operand = operand;
    return node;
}

AST_node *AST_new_call(const char *name, size_t line)
{
    AST_node *node = AST_new(AST_call, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.call.name = copy_string(name);
    if (node->as.call.name == NULL)
    {
        free(node);
        return NULL;
    }
    return node;
}

AST_node *AST_new_conv(AST_node *operand, size_t line)
{
    AST_node *node = AST_new(AST_conv_int_to_double, line);
    if (node == NULL)
    {
        AST_free(operand);
        return NULL;
    }
    node->as.conv.operand = operand;
    return node;
}

AST_node *AST_new_var_def(const char *name, bool is_mutable,
                          Data_types decl_type, AST_node *init, size_t line)
{
    AST_node *node = AST_new(AST_var_def, line);
    if (node == NULL)
    {
        AST_free(init);
        return NULL;
    }
    node->as.var_def.name = copy_string(name);
    if (node->as.var_def.name == NULL)
    {
        free(node);
        AST_free(init);
        return NULL;
    }
    node->as.var_def.is_mutable = is_mutable;
    node->as.var_def.decl_type = decl_type;
    node->as.var_def.init = init;
    return node;
}

AST_node *AST_new_assign(const char *name, AST_node *value, size_t line)
{
    AST_node *node = AST_new(AST_assign, line);
    if (node == NULL)
    {
        AST_free(value);
        return NULL;
    }
    node->as.assign.name = copy_string(name);
    if (node->as.assign.name == NULL)
    {
        free(node);
        AST_free(value);
        return NULL;
    }
    node->as.assign.value = value;
    return node;
}

AST_node *AST_new_if(AST_node *cond, AST_node *then_block, AST_node *else_block,
                     size_t line)
{
    AST_node *node = AST_new(AST_if, line);
    if (node == NULL)
    {
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

AST_node *AST_new_while(AST_node *cond, AST_node *body, size_t line)
{
    AST_node *node = AST_new(AST_while, line);
    if (node == NULL)
    {
        AST_free(cond);
        AST_free(body);
        return NULL;
    }
    node->as.while_stmt.body = body;
    node->as.while_stmt.cond = cond;
    return node;
}

AST_node *AST_new_return(AST_node *value, size_t line)
{
    AST_node *node = AST_new(AST_return, line);
    if (node == NULL)
    {
        AST_free(value);
        return NULL;
    }
    node->as.return_stmt.value = value;
    return node;
}

AST_node *AST_new_expr_stmt(AST_node *expr, size_t line)
{
    AST_node *node = AST_new(AST_expr_stmt, line);
    if (node == NULL)
    {
        AST_free(expr);
        return NULL;
    }
    node->as.expr_stmt.expr = expr;
    return node;
}

AST_node *AST_new_block(size_t line) { return AST_new(AST_block, line); }

AST_node *AST_new_program(size_t line) { return AST_new(AST_program, line); }

AST_node *AST_new_func_def(const char *name, size_t line)
{
    AST_node *node = AST_new(AST_func_def, line);
    if (node == NULL)
    {
        return NULL;
    }
    node->as.func_def.name = copy_string(name);
    if (node->as.func_def.name == NULL)
    {
        free(node);
        return NULL;
    }
    node->as.func_def.return_type = TYPE_UNKNOWN;
    return node;
}

bool AST_list_push(AST_node_list *list, AST_node *node)
{
    if (list == NULL)
    {
        return false;
    }
    if (list->count == list->capacity)
    {
        size_t new_capacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        AST_node **temp = realloc(list->items, new_capacity * sizeof(AST_node *));
        if (temp == NULL)
        {
            return false;
        }
        list->items = temp;
        list->capacity = new_capacity;
    }
    list->items[list->count] = node;
    list->count++;
    return true;
}

bool AST_func_add_param(AST_node *func, const char *name, Data_types type)
{
    if (func == NULL || func->kind != AST_func_def)
    {
        return false;
    }
    char *copy = copy_string(name);
    if (copy == NULL)
    {
        return false;
    }
    if (func->as.func_def.param_count == func->as.func_def.param_capacity)
    {
        size_t new_capacity = (func->as.func_def.param_capacity == 0) ? 4 : func->as.func_def.param_capacity * 2;
        AST_param *temp = realloc(func->as.func_def.params, new_capacity * sizeof(AST_param));
        if (temp == NULL)
        {
            free(copy);
            return false;
        }
        func->as.func_def.params = temp;
        func->as.func_def.param_capacity = new_capacity;
    }
    func->as.func_def.params[func->as.func_def.param_count].name = copy;
    func->as.func_def.params[func->as.func_def.param_count].type = type;
    func->as.func_def.param_count++;
    func->as.func_def.params[func->as.func_def.param_count].sym_id = -1; // Initialize sym_id to -1 for the new parameter
    return true;
}

void AST_free(AST_node *node)
{
    if (node == NULL)
    {
        return;
    }
    switch (node->kind)
    {
    case AST_program:
        free_list(&node->as.program.funcs);
        break;
    case AST_func_def:
        free(node->as.func_def.name);
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
    case AST_int_lit:
    case AST_double_lit:
    case AST_bool_lit:
    case AST_unit_lit:
    case AST_for:
    case AST_if_expr:
    case AST_member:
        break;
    default:
        break;
    }
    free(node);
}

const char *AST_kind_name(AST_node_type kind)
{
    switch (kind)
    {
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
    default:
        return "?";
    }
}

const char *AST_op_name(AST_op_type op)
{
    switch (op)
    {
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
