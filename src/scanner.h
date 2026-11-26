#ifndef SCANNER_H
#define SCANNER_H
#include <stdbool.h>

typedef enum
{
    // Identifier & keywords
    TOKEN_ID,
    TOKEN_DEF,
    TOKEN_DO,
    TOKEN_TYPE_DOUBLE,
    TOKEN_ELSE,
    TOKEN_IF,
    TOKEN_IMPORT,
    TOKEN_TYPE_INT,
    TOKEN_RETURN,
    TOKEN_TYPE_STRING,
    TOKEN_THEN,
    TOKEN_TYPE_UNIT,
    TOKEN_VAL,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_TYPE_BOOLEAN,
    TOKEN_CASE,
    TOKEN_CLASS,
    TOKEN_EXTENSION,
    TOKEN_FOR,
    TOKEN_LIST,
    TOKEN_MATCH,
    TOKEN_TYPE,
    TOKEN_TRUE,
    TOKEN_FALSE,

    // Data types
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,

    // Arithmetic operators
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,

    // Parentheses
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,

    // Relational operators
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,

    // Logical operators
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,

    // Operations
    TOKEN_ASSIGN,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_AT,

    // Other
    TOKEN_EOL,
    TOKEN_DOT,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_EOF
} token_type_t;

typedef struct
{
    int line;
    token_type_t type;
    bool first_on_new_line;
    union {
        char *s;
        int i;
        double d;
    } attribute;
} token_t;

void reset_scanner(void);
/**
 * @brief function returns one token
 * @return function returns struct token
 */
int get_next_token(token_t *token);

#endif
