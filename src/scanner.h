#ifndef SCANNER_H
#define SCANNER_H

typedef enum
{
    TOKEN_ID,

    //data types
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,

    //special words
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

    //Math operations
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,

    //Braces
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_LS_BRACE,
    TOKEN_RS_BRACE,

    //Compare
    TOKEN_LESS,
    TOKEN_MORE,
    TOKEN_MORE_EQ,
    TOKEN_LESS_EQ,
    TOKEN_EQ,
    TOKEN_NQ,

    //Boolean
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    //Ident, unident
    TOKEN_INDENT,
    TOKEN_UNIDENT,
    
    //Operations
    TOKEN_ASSIGN,
    TOKEN_SEMICOLON,
    //Other
    TOKEN_EOL,
    TOKEN_ERROR,
    TOKEN_NEW_LINE,
    TOKEN_EOF
} token_type_t;

typedef struct
{
    token_type_t type;
    union
    {
        char *s;
        int i;
        double d;
    } attribute;
} token_t;

typedef struct{
    int stack_capacity;
    int *stack;
    bool new_line_started;
    int current_line;
    int current_col;
    int ident_columns;
    int stack_top_idx;

} scanner_t;

token_t get_next_token(scanner_t *scanner);

#endif
