#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

typedef enum
{
    STATE_START,
    STATE_Q1,
    STATE_F_ID,
    STATE_F_ZERO,
    STATE_F_INT,
    STATE_Q2,
    STATE_Q_E,
    STATE_F_E,
    STATE_Q_E_SIGN,
    STATE_Q3,
    STATE_Q_D,
    STATE_F_D,
    STATE_Q4,
    STATE_Q5,
    STATE_Q6,
    STATE_Q7,
    STATE_Q8,
    STATE_Q9,
    STATE_F_MULTI,
    STATE_F_EMPTY,
    STATE_F_STRING,
    STATE_F_LT,
    STATE_F_GT
} state_t;

typedef struct
{
    const char *text;
    token_type_t type;
} keyword_t;

static const keyword_t keywords[] = {
    {"def", TOKEN_DEF},
    {"do", TOKEN_DO},
    {"Double", TOKEN_TYPE_DOUBLE},
    {"else", TOKEN_ELSE},
    {"if", TOKEN_IF},
    {"import", TOKEN_IMPORT},
    {"Int", TOKEN_TYPE_INT},
    {"return", TOKEN_RETURN},
    {"String", TOKEN_TYPE_STRING},
    {"then", TOKEN_THEN},
    {"Unit", TOKEN_TYPE_UNIT},
    {"val", TOKEN_VAL},
    {"var", TOKEN_VAR},
    {"while", TOKEN_WHILE},
    {"Boolean", TOKEN_TYPE_BOOLEAN},
    {"case", TOKEN_CASE},
    {"class", TOKEN_CLASS},
    {"extension", TOKEN_EXTENSION},
    {"for", TOKEN_FOR},
    {"List", TOKEN_LIST},
    {"match", TOKEN_MATCH},
    {"type", TOKEN_TYPE},
};

static void append_char(char **str, size_t *length, size_t *capacity, char c)
{
    if (*length + 1 >= *capacity)
    {
        *capacity *= 2;
        char *tmp = realloc(*str, *capacity);

        if (tmp == NULL)
        {
            free(*str);
            exit(99);
        }

        *str = tmp;
    }

    (*str)[(*length)++] = c;
    (*str)[*length] = '\0';
}

static token_type_t get_keyword(const char *str)
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strcmp(str, keywords[i].text) == 0)
            return keywords[i].type;
    }

    return TOKEN_ID;
}

token_t get_next_token(void)
{
    token_t token;

    size_t capacity = 16;
    size_t length = 0;
    char *str = calloc(capacity, 1);
    if (str == NULL)
        exit(99);

    state_t state = STATE_START;

    while (1)
    {
        int c = getchar();

        switch (state)
        {
        case STATE_START:
            if (c == '0')
            {
                state = STATE_F_ZERO;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if ('1' <= c && c <= '9')
            {
                state = STATE_F_INT;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
            {
                state = STATE_F_ID;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '.')
            {
                state = STATE_Q_D;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '_')
            {
                state = STATE_Q1;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '"')
            {
                state = STATE_Q4;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '<')
            {
                state = STATE_F_LT;
            }
            else if (c == '>')
            {
                state = STATE_F_GT;
            }
            else if (c == '+')
            {
                free(str);
                token.type = TOKEN_ADD;
                return token;
            }
            else if (c == '-')
            {
                free(str);
                token.type = TOKEN_SUB;
                return token;
            }
            else if (c == '*')
            {
                free(str);
                token.type = TOKEN_MULTIPLY;
                return token;
            }
            else if (c == '/')
            {
                free(str);
                token.type = TOKEN_DIVIDE;
                return token;
            }
            else if (c == EOF)
            {
                free(str);
                token.type = TOKEN_EOF;
                return token;
            }
            break;

        case STATE_F_LT:
            if (c == '=')
            {
                free(str);
                token.type = TOKEN_LESS_EQ;
                return token;
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_LESS;
                free(str);
                return token;
            }
            break;

        case STATE_F_GT:
            if (c == '=')
            {
                free(str);
                token.type = TOKEN_MORE_EQ;
                return token;
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_MORE;
                free(str);
                return token;
            }
            break;

        case STATE_F_ID:
            if (isalnum(c) || c == '_')
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                ungetc(c, stdin);
                token.type = get_keyword(str);

                if (token.type == TOKEN_ID)
                    token.attribute.s = str;
                else
                    free(str);

                return token;
            }
            break;

        case STATE_Q1:
            if (isalnum(c) || c == '_')
            {
                state = STATE_F_ID;
                str[length++] = (char)c;
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_F_ZERO:
            if (c == '.')
            {
                state = STATE_Q_D;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_INT;
                token.attribute.i = 0;
                free(str);
                return token;
            }
            break;

        case STATE_F_INT:
            if (isdigit(c))
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '_')
            {
                state = STATE_Q2;
            }
            else if (c == '.')
            {
                state = STATE_Q_D;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_INT;
                token.attribute.i = (int)strtol(str, NULL, 10);
                free(str);
                return token;
            }
            break;

        case STATE_Q2:
            if (isdigit(c))
            {
                state = STATE_F_INT;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q_E:
            if (isdigit(c))
            {
                state = STATE_F_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '-' || c == '+')
            {
                state = STATE_Q_E_SIGN;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q_E_SIGN:
            if (isdigit(c))
            {
                state = STATE_F_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_F_E:
            if (isdigit(c))
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == 'd' || c == 'D')
            {
                token.type = TOKEN_DOUBLE;
                token.attribute.d = strtod(str, NULL);
                free(str);
                return token;
            }
            else if (c == '_')
            {
                state = STATE_Q3;
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_DOUBLE;
                token.attribute.d = strtod(str, NULL);
                free(str);
                return token;
            }
            break;

        case STATE_Q3:
            if (isdigit(c))
            {
                state = STATE_F_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q_D:
            if (isdigit(c))
            {
                state = STATE_F_D;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_F_D:
            if (isdigit(c))
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == 'd' || c == 'D')
            {
                token.type = TOKEN_DOUBLE;
                token.attribute.d = strtod(str, NULL);
                free(str);
                return token;
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_DOUBLE;
                token.attribute.d = strtod(str, NULL);
                free(str);
                return token;
            }
            break;
        case STATE_Q4:
            if(c == '"')
            {
                state = STATE_F_EMPTY;
                append_char(&str, &length, &capacity, (char)c);
            }
            break;
        case STATE_F_EMPTY:
            if(c == '"')
            {
                state = STATE_Q7;
                append_char(&str, &length, &capacity, (char)c);
            }
            //we receive something other than ", move it back to START
            else
            {
                ungetc(c, stdin);
                state=STATE_START;
            }
            break;
        case STATE_Q7:
            if(c == '"')
            {
                state = STATE_Q8;
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            break;
        case STATE_Q8:
            if(c=='"')
            {
                state=STATE_Q9;
                append_char(&str, &length, &capacity, (char)c);
            }
            //if we add anything that is not a ", we need to go back to STATE_Q7
            else
            {
                state=STATE_Q7;
                append_char(&str, &length, &capacity, (char)c);
            }
            break;
        case STATE_Q9:
            if(c == '"')
            {
                state=STATE_F_MULTI;
                append_char(&str, &length, &capacity, (char)c);
            }
            else{
                state=STATE_Q7;
                append_char(&str, &length, &capacity, (char)c);
            }
            break;
        case STATE_F_MULTI:
            state=STATE_START;
            ungetc(c, stdin);
            break;

        default:
            break;
        }
    }
}