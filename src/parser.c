/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    parser.c
    # Brief:   Parser
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "parser.h"
#include "scanner.h"

static token_t current;

static ErrorCode expect(token_type_t token_type);
static ErrorCode parse_annotation(void);
static ErrorCode parse_block(void);
static ErrorCode parse_block_body(void);
static ErrorCode parse_func(AST_node **func_ast);
static ErrorCode parse_func_body(void);
static ErrorCode parse_param(void);
static ErrorCode parse_params(void);
static ErrorCode parse_params_c(void);
static ErrorCode parse_prologue(void);
static ErrorCode parse_return_type(void);
static ErrorCode parse_type(void);

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

static ErrorCode parse_annotation(void)
{
    switch (current.type)
    {
    case TOKEN_AT:
        ErrorCode err = expect(TOKEN_AT);
        if (err != OK)
            return err;

        err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        // annotation end
        switch (current.type)
        {
        case TOKEN_EOL:
            err = expect(TOKEN_EOL);
            if (err != OK)
                return err;

            break;
        case TOKEN_DEF:
            break;
        default:
            return ERROR_SYNTAX;
        }

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
        fprintf(stderr, "INDENT ");

        err = parse_block_body();
        if (err != OK)
            return err;

        err = expect(TOKEN_DEDENT);
        if (err != OK)
            return err;
        fprintf(stderr, "DEDENT ");

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
        // parse_statement()
        // parse_statement_list()
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_func(AST_node **func_ast)
{
    (void)func_ast;
    ErrorCode err;

    switch (current.type)
    {
    case TOKEN_DEF:
    case TOKEN_AT:
        err = parse_annotation();
        if (err != OK)
            return err;

        err = expect(TOKEN_DEF);
        if (err != OK)
            return err;

        err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        err = expect(TOKEN_L_PAREN);
        if (err != OK)
            return err;

        err = parse_params();
        if (err != OK)
            return err;

        err = expect(TOKEN_R_PAREN);
        if (err != OK)
            return err;

        err = parse_return_type();
        if (err != OK)
            return err;

        err = expect(TOKEN_ASSIGN);
        if (err != OK)
            return err;

        return parse_func_body();
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

        return err;
    case TOKEN_EOL:
        err = expect(TOKEN_EOL);
        if (err != OK)
            return err;

        return parse_block();
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_param(void)
{
    switch (current.type)
    {
    case TOKEN_ID:
        ErrorCode err = expect(TOKEN_ID);
        if (err != OK)
            return err;

        err = expect(TOKEN_COLON);
        if (err != OK)
            return err;

        return parse_type();
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_params(void)
{
    switch (current.type)
    {
    case TOKEN_ID:
        ErrorCode err = parse_param();
        if (err != OK)
            return err;

        return parse_params_c();
    case TOKEN_R_PAREN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_params_c(void)
{
    switch (current.type)
    {
    case TOKEN_COMMA:
        ErrorCode err = expect(TOKEN_COMMA);
        if (err != OK)
            return err;

        err = parse_param();
        if (err != OK)
            return err;

        return parse_params_c();
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

static ErrorCode parse_return_type(void)
{
    switch (current.type)
    {
    case TOKEN_COLON:
        ErrorCode err = expect(TOKEN_COLON);
        if (err != OK)
            return err;

        err = parse_type();
        if (err != OK)
            return err;

        return err;
    case TOKEN_ASSIGN:
        return OK;
    default:
        return ERROR_SYNTAX;
    }
}

static ErrorCode parse_type(void)
{
    ErrorCode err;

    switch (current.type)
    {
    case TOKEN_TYPE_INT:
        err = expect(TOKEN_TYPE_INT);
        if (err != OK)
            return err;

        return err;
    case TOKEN_TYPE_DOUBLE:
        err = expect(TOKEN_TYPE_DOUBLE);
        if (err != OK)
            return err;

        return err;
    case TOKEN_TYPE_STRING:
        err = expect(TOKEN_TYPE_STRING);
        if (err != OK)
            return err;

        return err;
    case TOKEN_TYPE_UNIT:
        err = expect(TOKEN_TYPE_UNIT);
        if (err != OK)
            return err;

        return err;
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