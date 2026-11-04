#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "scanner.h"

typedef enum
{
    STATE_START,
    STATE_F_SPACES,

    // Identifier & keywords
    STATE_Q_ID,
    STATE_F_ID,

    // Integer literal
    STATE_F_ZERO,
    STATE_F_INT,
    STATE_Q1,

    // Floating-point literal
    STATE_Q_D,
    STATE_F_D,
    STATE_Q2,
    STATE_Q_E,
    STATE_Q_ES,
    STATE_F_E,
    STATE_Q3,

    // String literal
    STATE_Q4,
    STATE_Q5,
    STATE_Q6,
    STATE_Q7,
    STATE_Q8,
    STATE_Q9,
    STATE_F_EMPTY,

    // Operations & operators
    STATE_F_ASSIGN,
    STATE_F_LT,
    STATE_F_GT,
    STATE_F_NOT,
    STATE_Q_AND,
    STATE_Q_OR,

    STATE_INDENT,
    STATE_F_SLASH,
    STATE_COMMENT,
    STATE_MULTLINE_COMMENT,
    STATE_ML_BEGIN,
    STATE_ML_END,
    STATE_F_DOT
} state_t;

typedef struct
{
    const char *text;
    token_type_t type;
} keyword_t;

static bool is_new_line = true;
static int *indent_stack = NULL;
static int stack_top = 0;
static int bracket_openings_counter=0;
static int pending_dedents = 0;
static bool is_div = false;
static int multiline_comment_openings_counter = 0;

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

/**
 * @brief function saves char to array
 * @param str is array to which we save char
 * @param lenght is current lenght of string
 * @param capacity is capacity of array
 * @param c is char we want to add
 * @return char will be added to array
 */
static int append_char(char **str, size_t *length, size_t *capacity, char c)
{
    if (*length + 1 >= *capacity)
    {
        *capacity *= 2;
        char *tmp = realloc(*str, *capacity);

        if (tmp == NULL)
            return ERROR_INTERNAL;

        *str = tmp;
    }

    (*str)[(*length)++] = c;
    (*str)[*length] = '\0';

    return OK;
}

/**
 * @brief function compares received token with type id and compares its value to find if it is not keyword
 * @param str is string value of token
 * @return token type which will be updated if str is keyword
 */
static token_type_t get_keyword(const char *str)
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strcmp(str, keywords[i].text) == 0)
            return keywords[i].type;
    }

    return TOKEN_ID;
}

/**
 * @brief function return boolean value based on current indentation
 * @param spaces is number of counted whitespaces in indentation
 * @param token_type is type of token we indentify
 * @return int
 */
static int is_indent_changed(int spaces, token_type_t *token_type)
{
    if (indent_stack[stack_top] < spaces)
    {
        indent_stack[++stack_top] = spaces;
        *token_type = TOKEN_INDENT;
        return 1;
    }
    else if (spaces < indent_stack[stack_top])
    {
        int dedent_count = 0;
        while (spaces < indent_stack[stack_top])
        {
            stack_top--;
            dedent_count++;
        }

        if (indent_stack[stack_top] != spaces)
            return -1;

        pending_dedents = dedent_count - 1;
        *token_type = TOKEN_DEDENT;
        return 1;
    }

    return 0;
}

/**
 * @brief function returns one token
 * @return function returns struct token
 */
token_t get_next_token(void)
{
    token_t token;
    bool is_multiline_comment=false;
    if (pending_dedents > 0)
    {
        pending_dedents--;
        token.type = TOKEN_DEDENT;
        return token;
    }

    size_t capacity = 16;
    size_t length = 0;
    char *str = calloc(capacity, 1);
    if (str == NULL)
        exit(99);

    state_t state = STATE_START;
    int spaces = 0;

    if (indent_stack == NULL)
    {
        indent_stack = malloc(sizeof(int) * 256);
        if (indent_stack == NULL)
        {
            exit(99);
        }
    }
    if(is_div)
    {
        is_div = false;
        token.type = TOKEN_DIV;
        return token;
    }

    while (1)
    {
        int c = getchar();

        switch (state)
        {
        case STATE_START:
            if(is_multiline_comment)
            {
                if(c == ' ')
                {
                    break;
                }
                else if( c == '\n' || c == EOF)
                {
                    is_multiline_comment = false;
                }
                else
                {
                    free(str);
                    token.type = TOKEN_ERROR;
                    return token;
                }
            }
            // Every char except space and \n turns off new line
            if (is_new_line && c != ' ' && c != '\n' && c != EOF && c!='/')
            {
                is_new_line = false;

                int changed = is_indent_changed(spaces, &token.type);
                if (changed == -1)
                {
                    fprintf(stderr, "Inconsistent indentation\n");
                    exit(ERROR_LEX);
                }

                if (changed)
                {
                    ungetc(c, stdin);
                    free(str);
                    return token;
                }
            }

            if (c == '0')
            {
                state = STATE_F_ZERO;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if ('1' <= c && c <= '9')
            {
                state = STATE_F_INT;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
            {
                state = STATE_F_ID;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '.')
            {
                state = STATE_F_DOT;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
            {
                state = STATE_Q_ID;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '"')
            {
                state = STATE_Q4;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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
                token.type = TOKEN_MUL;
                return token;
            }
            else if (c == '/')
                state = STATE_F_SLASH;
            else if (c == '=')
                state = STATE_F_ASSIGN;
            else if (c == '<')
                state = STATE_F_LT;
            else if (c == '>')
                state = STATE_F_GT;
            else if (c == '!')
                state = STATE_F_NOT;
            else if (c == '&')
                state = STATE_Q_AND;
            else if (c == '|')
                state = STATE_Q_OR;
            else if (c == '(')
            {
                bracket_openings_counter++;
                free(str);
                token.type = TOKEN_L_PAREN;
                return token;
            }
            else if (c == ')')
            {
                bracket_openings_counter--;
                free(str);
                token.type = TOKEN_R_PAREN;
                return token;
            }
            else if (c == ':')
            {
                free(str);
                token.type = TOKEN_COLON;
                return token;
            }
            else if (c == ';')
            {
                free(str);
                token.type = TOKEN_SEMICOLON;
                return token;
            }
            else if (c == ' ')
            {
                if (is_new_line)
                {
                    spaces++;
                    state = STATE_F_SPACES;
                }
            }
            else if (c == '\n')
            {
                if (!is_new_line && bracket_openings_counter==0)
                {
                    free(str);
                    is_new_line = true;
                    token.type = TOKEN_EOL;
                    return token;
                }
            }
            else if (c == EOF)
            {
                free(str);

                if (!is_new_line)
                {
                    is_new_line = true;
                    token.type = TOKEN_EOL;
                    return token;
                }

                if (stack_top > 0)
                {
                    stack_top--;
                    token.type = TOKEN_DEDENT;
                    return token;
                }

                token.type = TOKEN_EOF;
                return token;
            }
            break;

        case STATE_F_SPACES:
            if (c == ' ')
            {
                spaces++;
            }
            else if (c == '\n')
            {
                spaces = 0;
            }
            else if (c == EOF)
            {
                spaces = 0;
                state = STATE_START;
            }
            else if (c == '/')
            {
                state = STATE_F_SLASH;
            }
            else
            {
                ungetc(c, stdin);
                is_new_line = false;

                int changed = is_indent_changed(spaces, &token.type);
                if (changed == -1)
                {
                    fprintf(stderr, "Inconsistent indentation\n");
                    exit(ERROR_LEX);
                }

                if (changed)
                {
                    free(str);
                    return token;
                }
                else
                {
                    state = STATE_START;
                }
            }
            break;

        // Identifier & keywords
        case STATE_Q_ID:
            if (isalnum(c) || c == '_')
            {
                state = STATE_F_ID;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_F_ID:
            if (isalnum(c) || c == '_')
            {
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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

        // Integer literal
        case STATE_F_ZERO:
            if (c == '.')
            {
                state = STATE_Q_D;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                state = STATE_Q1;
            else if (c == '.')
            {
                state = STATE_Q_D;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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

        case STATE_Q1:
            if (isdigit(c))
            {
                state = STATE_F_INT;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                break;
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        // Floating-point literal
        case STATE_Q_D:
            if (isdigit(c))
            {
                state = STATE_F_D;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                state = STATE_Q2;
            else if (c == 'e' || c == 'E')
            {
                state = STATE_Q_E;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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

        case STATE_Q2:
            if (isdigit(c))
            {
                state = STATE_F_D;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                break;
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
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '-' || c == '+')
            {
                state = STATE_Q_ES;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q_ES:
            if (isdigit(c))
            {
                state = STATE_F_E;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
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
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                state = STATE_Q3;
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

        case STATE_Q3:
            if (isdigit(c))
            {
                state = STATE_F_E;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '_')
                break;
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        // String literal
        case STATE_Q4:
            if (c == '"')
            {
                state = STATE_F_EMPTY;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '\\')
            {
                state = STATE_Q5;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c != '\n' && c != '"' && c != '\\' && c != EOF)
            {
                state = STATE_Q6;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_F_EMPTY:
            if (c == '"')
            {
                state = STATE_Q7;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_STRING;
                token.attribute.s = str;
                return token;
            }
            break;

        case STATE_Q5:
            if (c == '\\' || c == '"' || c == 'n' || c == 'r' || c == 't')
            {
                state = STATE_Q6;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q6:
            if (c != '\n' && c != '"' && c != '\\' && c != EOF)
            {
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '\\')
            {
                state = STATE_Q5;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else if (c == '"')
            {
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
                token.type = TOKEN_STRING;
                token.attribute.s = str;
                return token;
            }
            else
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            break;

        case STATE_Q7:
            if (c == EOF)
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            else if (c == '"')
            {
                state = STATE_Q8;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            break;

        case STATE_Q8:
            if (c == EOF)
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            else if (c == '"')
            {
                state = STATE_Q9;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                state = STATE_Q7;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            break;

        case STATE_Q9:
            if (c == EOF)
            {
                fprintf(stderr, "Unexpected character: %c", c);
                free(str);
                exit(1);
            }
            else if (c == '"')
            {
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
                token.type = TOKEN_STRING;
                token.attribute.s = str;
                return token;
            }
            else
            {
                state = STATE_Q7;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            break;

        // Operations & operators
        case STATE_F_ASSIGN:
            if (c == '=')
                token.type = TOKEN_EQ;
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_ASSIGN;
            }

            free(str);
            return token;

        case STATE_F_LT:
            if (c == '=')
                token.type = TOKEN_LE;
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_LT;
            }

            free(str);
            return token;

        case STATE_F_GT:
            if (c == '=')
                token.type = TOKEN_GE;
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_GT;
            }

            free(str);
            return token;

        case STATE_F_NOT:
            if (c == '=')
                token.type = TOKEN_NE;
            else
            {
                ungetc(c, stdin);
                token.type = TOKEN_NOT;
            }

            free(str);
            return token;

        case STATE_Q_AND:
            free(str);

            if (c == '&')
            {
                token.type = TOKEN_AND;
                return token;
            }

            fprintf(stderr, "Unexpected character: %c", c);
            free(str);
            exit(1);

        case STATE_Q_OR:
            free(str);

            if (c == '|')
            {
                token.type = TOKEN_OR;
                return token;
            }

            fprintf(stderr, "Unexpected character: %c", c);
            free(str);
            exit(1);

        case STATE_F_SLASH:
            if (c == '/')
            {
                spaces=0;
                state = STATE_COMMENT;
            }
            else if (c == '*')
            {
                spaces = 0;
                multiline_comment_openings_counter++;
                state = STATE_MULTLINE_COMMENT;
            }
            else
            {
                ungetc(c, stdin);
                free(str);
                if(is_new_line)
                {
                    is_new_line = false;

                    int changed = is_indent_changed(spaces, &token.type);
                    if (changed == -1)
                    {
                        fprintf(stderr, "Inconsistent indentation\n");
                        exit(ERROR_LEX);
                    }

                    if (changed)
                    {
                        is_div = true;
                        return token;
                    }
                }
                token.type = TOKEN_DIV;
                return token;
            }
            break;

        case STATE_COMMENT:
            if (c == '\n' || c == EOF)
            {
                ungetc(c, stdin);
                state = STATE_START;
            }
            break;

        case STATE_MULTLINE_COMMENT:
            if (c == '*')
            {
                state = STATE_ML_END;
            }
            else if (c == '/')
            {
                state = STATE_ML_BEGIN;
            }
            else if (c == EOF)
            {
                free(str);
                token.type = TOKEN_ERROR;
                return token;
            }
            else if(c == '\n')
            {
                is_multiline_comment = true;
            }
            break;
        case STATE_ML_BEGIN:
            if (c == '*')
            {
                multiline_comment_openings_counter++;
                state = STATE_MULTLINE_COMMENT;
            }
            else if (c == '/')
            {
                state = STATE_ML_BEGIN;
            }
            else if(c == EOF)
            {
                free(str);
                token.type=TOKEN_ERROR;
                return token;
            }
            else if(c == '\n')
            {
                is_multiline_comment=true;
                state=STATE_MULTLINE_COMMENT;
            }
            else
            {
                state = STATE_MULTLINE_COMMENT;
            }
            break;
        case STATE_ML_END:
            if (c == '/')
            {
                multiline_comment_openings_counter--;
                if (multiline_comment_openings_counter == 0)
                {
                    state = STATE_START;
                }
            }
            else if (c == '*')
            {
                state = STATE_ML_END;
            }
            else if(c == EOF)
            {
                free(str);
                token.type = TOKEN_ERROR;
                return token;
            }
            else if(c=='\n')
            {
                is_multiline_comment=true;
                state=STATE_MULTLINE_COMMENT;
            }
            else
            {
                state = STATE_MULTLINE_COMMENT;
            }
            break;

        case STATE_F_DOT:
            if (isdigit(c))
            {
                state = STATE_F_D;
                if (append_char(&str, &length, &capacity, (char)c) != OK)
                {
                    free(str);
                    exit(ERROR_INTERNAL);
                }
            }
            else
            {
                ungetc(c, stdin);
                free(str);
                token.type = TOKEN_DOT;
                return token;
            }
            break;

        default:
            break;
        }
    }
}
