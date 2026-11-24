/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       expr.h
   # Brief:      Expression parser interface (precedence analysis)
   # Authors:    Damian Volosiny (xvolosd00)
*/

/* Interface comment in this file was drafted with AI assistance (Claude).
   Prompt: "Vies mi vygenerovat komentar pre expr.h?" */
#ifndef EXPR_H
#define EXPR_H

#include "ast.h"
#include "error.h"
#include "scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses one expression using precedence syntax analysis and builds its AST.
 *
 * Input:
 *   *token  - the first token of the expression, already read by the caller.
 *
 * Output:
 *   *token  - the first token after the expression. It is NOT consumed and is
 *             left for the caller (e.g. EOL, 'then', 'do', ',', ';', DEDENT,
 *             EOF, or ')' when the parenthesis depth is zero).
 *   *result - on success, the root of the expression AST. The caller owns it
 *             and must free it with AST_free. On error it is set to NULL and
 *             everything allocated during parsing is freed.
 *
 * Return value:
 *   OK  - success
 *   ERROR_LEX  - lexical error (returned by the scanner)
 *   ERROR_SYNTAX  - syntax error in the expression
 *   ERROR_INTERNAL - internal error (memory allocation failure)
 *
 * Notes:
 *   - An identifier followed by '(' is parsed as a function call (FUNEXP);
 *     each argument is parsed recursively as an expression.
 *   - Relational and equality operators are non-associative, so chains such
 *     as a < b < c or a == b == c are syntax errors.
 *   - '()' is parsed as a Unit literal.
 *   - Types are not checked here; this is done by semantic analysis.
 */

ErrorCode expr_parse(token_t *token, AST_node **result);

ErrorCode expr_parse_after_id(token_t *id_token, token_t *token,
                              AST_node **result);

#ifdef __cplusplus
}
#endif

#endif
