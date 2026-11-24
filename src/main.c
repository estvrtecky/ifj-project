/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    main.c
    # Brief:   Main
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#include "ast.h"
#include "codegen.h"
#include "error.h"
#include "parser.h"
#include "sema.h"

#include <stdio.h>

int main(void)
{
    AST_node *ast = NULL;
    ErrorCode err = parser_init();
    if (err != OK)
        goto fail;

    err = parse_program(&ast);
    if (err != OK)
        goto fail;

    err = sema_analyze(ast);
    if (err != OK)
        goto fail;

    err = generate_code(ast, stdout);
    if (err != OK)
        goto fail;

fail:
    AST_free(ast);
    return err;
}