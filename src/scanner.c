#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

typedef enum
{
    STATE_START,
    STATE_Q1,
    STATE_F_ID
} state_t;

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
            if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
            {
                state = STATE_F_ID;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == '_')
            {
                state = STATE_Q1;
                append_char(&str, &length, &capacity, (char)c);
            }
            else if (c == EOF)
            {
                free(str);
                token.type = TOKEN_EOF;
                return token;
            }
            break;

        case STATE_F_ID:
            if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_')
            {
                append_char(&str, &length, &capacity, (char)c);
            }
            else
            {
                ungetc(c, stdin);
                // TODO: if str is keyword
                token.type = TOKEN_ID;
                token.attribute.s = str;
                return token;
            }
            break;

        case STATE_Q1:
            if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_')
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

        default:
            break;
        }
    }
}