#ifndef SCANNER_H
#define SCANNER_H

typedef enum
{
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
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

token_t get_next_token(void);

#endif