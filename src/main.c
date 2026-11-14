#include "ast.h"
#include "error.h"
#include "parser.h"

int main(void)
{
    AST_node *ast = NULL;
    ErrorCode err = parser_init();
    if (err != OK)
        return err;

    err = parse_program(&ast);
    if (err != OK)
        return err;

    return OK;
}