/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    parser.c
    # Brief:   Parser
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"

static token_t current;

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

    return OK;

fail:
    AST_free(program);
    return err;
}