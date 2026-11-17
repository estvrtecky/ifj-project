/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    parser.c
    # Brief:   Parser
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "parser.h"
#include "scanner.h"
#include "types.h"

static token_t current;

static ErrorCode expect(token_type_t token_type);
static char *extract_string(void);
static ErrorCode parse_annotation(bool *is_main);
static ErrorCode parse_block(void);
static ErrorCode parse_block_body(void);
static ErrorCode parse_func(AST_node **func_ast);
static ErrorCode parse_func_body(void);
static ErrorCode parse_id_tail(token_t id_token);
static ErrorCode parse_line_statement(void);
static ErrorCode parse_line_statement_c(void);
static ErrorCode parse_param(AST_node **func_ast);
static ErrorCode parse_params(AST_node **func_ast);
static ErrorCode parse_params_c(AST_node **func_ast);
static ErrorCode parse_prologue(void);
static ErrorCode parse_return_type(Data_types *data_type);
static ErrorCode parse_return_value(void);
static ErrorCode parse_statement(void);
static ErrorCode parse_statement_list(void);
static ErrorCode parse_type(Data_types *data_type);
static ErrorCode parse_var_type(Data_types *data_type);

/**
 * @brief Expects the current token to be the given token type.
 *
 * If the current token is not the expected type, returns ERROR_SYNTAX.
 * If the current token is the expected type, it reads the next token and returns OK.
 * If there is an error while reading the next token, it returns ERROR_LEX or ERROR_INTERNAL.
 *
 * @param token_type Expected token type.
 * @return ErrorCode
 */
static ErrorCode expect(token_type_t token_type)
{
    if (current.type != token_type)
        return ERROR_SYNTAX;

    if (current.type == TOKEN_ID || current.type == TOKEN_STRING)
    {
        free(current.attribute.s);
        current.attribute.s = NULL;
    }

    return get_next_token(&current);
}

/**
 * @brief Extracts the string from the current token and sets the current token's string attribute to NULL.
 *
 * The caller is responsible for freeing the memory.
 *
 * @return Pointer to the extracted string.
 */
static char *extract_string(void)
{
    char *s = current.attribute.s;
    current.attribute.s = NULL;
    return s;
}

/**
 * @brief Parses annotation.
 */
static ErrorCode parse_annotation(bool *is_main)
{
    switch (current.type)
    {
    case TOKEN_AT:
        char *id = NULL;
        ErrorCode err = expect(TOKEN_AT);
        if (err != OK)
            goto fail;

        if (current.type != TOKEN_ID)
        {
            err = ERROR_SYNTAX;
            goto fail;
        }

        id = extract_string();
        err = expect(TOKEN_ID);
        if (err != OK)
            goto fail;

        // annotation end
        switch (current.type)
        {
        case TOKEN_EOL:
            err = expect(TOKEN_EOL);
            if (err != OK)
                goto fail;
            break;
        case TOKEN_DEF:
            break;
        default:
            err = ERROR_SYNTAX;
            goto fail;
        }

        *is_main = strcmp(id, "main") == 0;
    fail:
        free(id);
        return err;
    case TOKEN_DEF:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_block(void)
{
    switch (current.type)
    {
    case TOKEN_INDENT:
        ErrorCode err = expect(TOKEN_INDENT);
        if (err != OK)
            return err;

        err = parse_block_body();
        if (err != OK)
            return err;

        err = expect(TOKEN_DEDENT);
        if (err != OK)
            return err;

        return err;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_block_body(void)
{
    switch (current.type)
    {
    case TOKEN_ID:
    case TOKEN_IF:
    case TOKEN_RETURN:
    case TOKEN_VAL:
    case TOKEN_VAR:
    case TOKEN_WHILE:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        ErrorCode err = parse_statement();
        if (err != OK)
            return err;

        return parse_statement_list();
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_func(AST_node **out)
{
    ErrorCode err;

    switch (current.type)
    {
    case TOKEN_DEF:
    case TOKEN_AT:
        bool is_main = false;
        char *func_id = NULL;
        AST_node *func_ast = NULL;

        err = parse_annotation(&is_main);
        if (err != OK)
            goto fail;

        err = expect(TOKEN_DEF);
        if (err != OK)
            goto fail;

        if (current.type != TOKEN_ID)
        {
            err = ERROR_SYNTAX;
            goto fail;
        }

        func_id = extract_string();
        err = expect(TOKEN_ID);
        if (err != OK)
            goto fail;

        func_ast = AST_new_func_def(func_id, 0);

        err = expect(TOKEN_L_PAREN);
        if (err != OK)
            goto fail;

        err = parse_params(&func_ast);
        if (err != OK)
            goto fail;

        err = expect(TOKEN_R_PAREN);
        if (err != OK)
            goto fail;

        Data_types data_type = TYPE_UNKNOWN;
        err = parse_return_type(&data_type);
        if (err != OK)
            goto fail;
        func_ast->as.func_def.return_type = data_type;

        err = expect(TOKEN_ASSIGN);
        if (err != OK)
            goto fail;

        err = parse_func_body();
        if (err != OK)
            goto fail;

        *out = func_ast;
        free(func_id);
        return err;

    fail:
        free(func_id);
        AST_free(func_ast);
        return err;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_func_body(void)
{
    ErrorCode err;

    switch (current.type)
    {
    case TOKEN_ID:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        AST_node *expr_ast = NULL;
        err = expr_parse(&current, &expr_ast);
        if (err != OK)
            return err;

        return expect(TOKEN_EOL);
    case TOKEN_EOL:
        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        return parse_block();
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_id_tail(token_t id_token)
{
    (void)id_token;
    ErrorCode err;
    AST_node *expr_ast = NULL;

    switch (current.type)
    {
    case TOKEN_ADD:
    case TOKEN_SUB:
    case TOKEN_MUL:
    case TOKEN_DIV:
    case TOKEN_EQ:
    case TOKEN_NE:
    case TOKEN_LT:
    case TOKEN_LE:
    case TOKEN_GT:
    case TOKEN_GE:
        return expr_parse(&current, &expr_ast);
    case TOKEN_ASSIGN:
        err = expect(TOKEN_ASSIGN);
        if (err != OK)
            return err;

        return expr_parse(&current, &expr_ast);
    case TOKEN_SEMICOLON:
    case TOKEN_EOL:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_line_statement(void)
{
    ErrorCode err;
    Data_types data_type;
    AST_node *expr_ast = NULL;

    switch (current.type)
    {
    case TOKEN_VAR:
        err = expect(TOKEN_VAR);
        if (err != OK)
            return err;

        err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        err = parse_var_type(&data_type);
        if (err != OK)
            return err;

        err = expect(TOKEN_ASSIGN);
        if (err != OK)
            return err;

        return expr_parse(&current, &expr_ast);
    case TOKEN_VAL:
        err = expect(TOKEN_VAL);
        if (err != OK)
            return err;

        err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        err = parse_var_type(&data_type);
        if (err != OK)
            return err;

        err = expect(TOKEN_ASSIGN);
        if (err != OK)
            return err;

        return expr_parse(&current, &expr_ast);
    case TOKEN_RETURN:
        err = expect(TOKEN_RETURN);
        if (err != OK)
            return err;

        return parse_return_value();
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        return expr_parse(&current, &expr_ast);
    case TOKEN_ID:
        token_t id_token = {current.line,
                            current.type,
                            current.attribute};

        err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        return parse_id_tail(id_token);
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_line_statement_c(void)
{
    switch (current.type)
    {
    case TOKEN_SEMICOLON:
        ErrorCode err = expect(TOKEN_SEMICOLON);
        if (err != OK)
            return err;

        err = parse_line_statement();
        if (err != OK)
            return err;

        return parse_line_statement_c();
    case TOKEN_EOL:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_param(AST_node **func_ast)
{
    switch (current.type)
    {
    case TOKEN_ID:
        char *id = extract_string();
        ErrorCode err = expect(TOKEN_ID);
        if (err != OK)
            goto fail;

        err = expect(TOKEN_COLON);
        if (err != OK)
            goto fail;

        Data_types data_type;
        err = parse_type(&data_type);
        if (err != OK)
            goto fail;

        if (!AST_func_add_param(*func_ast, id, data_type))
        {
            err = ERROR_INTERNAL;
            goto fail;
        }

    fail:
        free(id);
        return err;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_params(AST_node **func_ast)
{
    switch (current.type)
    {
    case TOKEN_ID:
        ErrorCode err = parse_param(func_ast);
        if (err != OK)
            return err;

        return parse_params_c(func_ast);
    case TOKEN_R_PAREN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_params_c(AST_node **func_ast)
{
    switch (current.type)
    {
    case TOKEN_COMMA:
        ErrorCode err = expect(TOKEN_COMMA);
        if (err != OK)
            return err;

        err = parse_param(func_ast);
        if (err != OK)
            return err;

        return parse_params_c(func_ast);
    case TOKEN_R_PAREN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_prologue(void)
{
    ErrorCode err = expect(TOKEN_IMPORT);
    if (err != OK)
        return err;

    if (current.type != TOKEN_ID || strcmp(current.attribute.s, "ifj26") != 0)
        return ERROR_SYNTAX;

    err = expect(TOKEN_ID);
    if (err != OK)
        return err;

    err = expect(TOKEN_DOT);
    if (err != OK)
        return ERROR_SYNTAX;

    err = expect(TOKEN_MUL);
    if (err != OK)
        return ERROR_SYNTAX;

    return expect(TOKEN_EOL);
}

static ErrorCode parse_return_type(Data_types *data_type)
{
    switch (current.type)
    {
    case TOKEN_COLON:
        ErrorCode err = expect(TOKEN_COLON);
        if (err != OK)
            return err;

        return parse_type(data_type);
    case TOKEN_ASSIGN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_return_value(void)
{
    switch (current.type)
    {
    case TOKEN_ID:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        AST_node *expr_ast = NULL;
        return expr_parse(&current, &expr_ast);
    case TOKEN_SEMICOLON:
    case TOKEN_EOL:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_statement(void)
{
    ErrorCode err;
    AST_node *expr_ast = NULL;

    switch (current.type)
    {
    case TOKEN_IF:
        err = expect(TOKEN_IF);
        if (err != OK)
            return err;

        err = expr_parse(&current, &expr_ast);
        if (err != OK)
            return err;

        err = expect(TOKEN_THEN);
        if (err != OK)
            return err;

        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        err = parse_block();
        if (err != OK)
            return err;

        err = expect(TOKEN_ELSE);
        if (err != OK)
            return err;

        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        return parse_block();
    case TOKEN_WHILE:
        err = expect(TOKEN_WHILE);
        if (err != OK)
            return err;

        err = expr_parse(&current, &expr_ast);
        if (err != OK)
            return err;

        err = expect(TOKEN_DO);
        if (err != OK)
            return err;

        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        return parse_block();
    case TOKEN_ID:
    case TOKEN_RETURN:
    case TOKEN_VAL:
    case TOKEN_VAR:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        err = parse_line_statement();
        if (err != OK)
            return err;

        err = parse_line_statement_c();
        if (err != OK)
            return err;

        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        return err;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_statement_list(void)
{
    switch (current.type)
    {
    case TOKEN_ID:
    case TOKEN_IF:
    case TOKEN_RETURN:
    case TOKEN_VAR:
    case TOKEN_VAL:
    case TOKEN_WHILE:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_L_PAREN:
    case TOKEN_SUB:
    case TOKEN_ADD:
        ErrorCode err = parse_statement();
        if (err != OK)
            return err;

        return parse_statement_list();
    case TOKEN_DEDENT:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

/**
 * @brief Parses type.
 */
static ErrorCode parse_type(Data_types *data_type)
{
    ErrorCode err;

    switch (current.type)
    {
    case TOKEN_TYPE_INT:
        err = expect(TOKEN_TYPE_INT);
        *data_type = TYPE_INT;
        break;
    case TOKEN_TYPE_DOUBLE:
        err = expect(TOKEN_TYPE_DOUBLE);
        *data_type = TYPE_DOUBLE;
        break;
    case TOKEN_TYPE_STRING:
        err = expect(TOKEN_TYPE_STRING);
        *data_type = TYPE_STRING;
        break;
    case TOKEN_TYPE_UNIT:
        err = expect(TOKEN_TYPE_UNIT);
        *data_type = TYPE_UNIT;
        break;
    default:
        return ERROR_SYNTAX;
    }

    return err;
}

static ErrorCode parse_var_type(Data_types *data_type)
{
    switch (current.type)
    {
    case TOKEN_COLON:
        ErrorCode err = expect(TOKEN_COLON);
        if (err != OK)
            return err;

        return parse_type(data_type);
    case TOKEN_ASSIGN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

ErrorCode parser_init(void)
{
    return get_next_token(&current);
}

/**
 * @brief program -> prologue { func_def } EOF
 */
ErrorCode parse_program(AST_node **ast)
{
    *ast = NULL;
    ErrorCode err;
    AST_node *program = AST_new_program(1);

    if (program == NULL)
        return ERROR_INTERNAL;

    err = parse_prologue();
    if (err != OK)
        goto fail;

    while (current.type != TOKEN_EOF)
    {
        AST_node *func = NULL;

        err = parse_func(&func);
        if (err != OK)
            goto fail;

        if (!AST_list_push(&program->as.program.funcs, func))
        {
            AST_free(func);
            err = ERROR_INTERNAL;
            goto fail;
        }
    }

    *ast = program;
    return OK;

fail:
    AST_free(program);
    return err;
}