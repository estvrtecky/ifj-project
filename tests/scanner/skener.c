/*
Cely tento kod bol vygenerovany pomocou AI (Claude verzia 5.5 - Sonnet) ako reakcia na ziadost o vygenerovanie suboru ktory by obsahoval main a spustal scanner aby sme script.sh mohli skontrolovat spravnost scannera.
Uprava: STRING sa vypisuje ako STRING("obsah"), pricom uvodzovky pridava driver (scanner uz vracia obsah bez uvodzoviek a s dekodovanymi escape sekvenciami).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "scanner.h"

#define MAX_TOKENS 10000

static const char *token_name(token_type_t t)
{
    switch (t)
    {
    case TOKEN_DEF: return "DEF";
    case TOKEN_DO: return "DO";
    case TOKEN_ELSE: return "ELSE";
    case TOKEN_IF: return "IF";
    case TOKEN_IMPORT: return "IMPORT";
    case TOKEN_RETURN: return "RETURN";
    case TOKEN_THEN: return "THEN";
    case TOKEN_VAL: return "VAL";
    case TOKEN_VAR: return "VAR";
    case TOKEN_WHILE: return "WHILE";
    case TOKEN_CASE: return "CASE";
    case TOKEN_CLASS: return "CLASS";
    case TOKEN_EXTENSION: return "EXTENSION";
    case TOKEN_FOR: return "FOR";
    case TOKEN_LIST: return "LIST";
    case TOKEN_MATCH: return "MATCH";
    case TOKEN_TYPE: return "TYPE";
    case TOKEN_TYPE_INT: return "TYPE_INT";
    case TOKEN_TYPE_DOUBLE: return "TYPE_DOUBLE";
    case TOKEN_TYPE_STRING: return "TYPE_STRING";
    case TOKEN_TYPE_UNIT: return "TYPE_UNIT";
    case TOKEN_TYPE_BOOLEAN: return "TYPE_BOOLEAN";
    case TOKEN_ID: return "ID";
    case TOKEN_INT: return "INT";
    case TOKEN_DOUBLE: return "DOUBLE";
    case TOKEN_STRING: return "STRING";
    case TOKEN_ADD: return "ADD";
    case TOKEN_SUB: return "SUB";
    case TOKEN_MUL: return "MUL";
    case TOKEN_DIV: return "DIV";
    case TOKEN_ASSIGN: return "ASSIGN";
    case TOKEN_EQ: return "EQ";
    case TOKEN_NE: return "NE";
    case TOKEN_NOT: return "NOT";
    case TOKEN_AND: return "AND";
    case TOKEN_OR: return "OR";
    case TOKEN_LT: return "LT";
    case TOKEN_LE: return "LE";
    case TOKEN_GT: return "GT";
    case TOKEN_GE: return "GE";
    case TOKEN_L_PAREN: return "L_PAREN";
    case TOKEN_R_PAREN: return "R_PAREN";
    case TOKEN_COLON: return "COLON";
    case TOKEN_SEMICOLON: return "SEMICOLON";
    case TOKEN_DOT: return "DOT";
    case TOKEN_COMMA: return "COMMA";
    case TOKEN_AT: return "AT";
    case TOKEN_EOL: return "EOL";
    case TOKEN_INDENT: return "INDENT";
    case TOKEN_DEDENT: return "DEDENT";
    case TOKEN_EOF: return "EOF";
    //case TOKEN_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

/* Testovaci driver:
 *  - vypisuje tokeny oddelene medzerou na stdout
 *  - STRING sa vypisuje ako STRING("obsah"): uvodzovky okolo obsahu pridava driver,
 *    token.attribute.s uz obsahuje iba dekodovany obsah (bez uvodzoviek, bez """)
 *  - lexikalna chyba (ERROR_LEX)  -> vypise "ERROR", navratovy kod 1
 *  - vnutorna chyba (ERROR_INTERNAL) -> vypise "INTERNAL_ERROR", navratovy kod 99
 *  - TOKEN_EOF -> navratovy kod 0
 *  - ak scanner.h ma token_t.line (-DHAVE_TOKEN_LINE), s argumentom --lines za kazdy token vypise "@riadok" (token.line), napr. ID(a)@1
 *  - viac ako MAX_TOKENS tokenov -> "LIMIT", navratovy kod 2 (ochrana pred zacyklenim)
 */
int main(int argc, char **argv)
{
#ifdef HAVE_TOKEN_LINE
    int show_lines = (argc > 1 && strcmp(argv[1], "--lines") == 0);
#else
    (void)argc; (void)argv;
#endif

    for (int count = 0; count < MAX_TOKENS; count++)
    {
        token_t token = {0};
        int rc = get_next_token(&token);

        if (count > 0)
            putchar(' ');

        if (rc == ERROR_LEX)
        {
            puts("ERROR");
            return 1;
        }
        if (rc != OK)
        {
            puts("INTERNAL_ERROR");
            return 99;
        }

        switch (token.type)
        {
        case TOKEN_ID:
            printf("ID(%s)", token.attribute.s);
            free(token.attribute.s);
            break;
        case TOKEN_INT:
            printf("INT(%d)", token.attribute.i);
            break;
        case TOKEN_DOUBLE:
            printf("DOUBLE(%g)", token.attribute.d);
            break;
        case TOKEN_STRING:
            fputs("STRING(\"", stdout);
            fputs(token.attribute.s, stdout);
            fputs("\")", stdout);
            free(token.attribute.s);
            break;
        default:
            fputs(token_name(token.type), stdout);
            break;
        }

#ifdef HAVE_TOKEN_LINE
        if (show_lines)
            printf("@%d", token.line);
#endif

        if (token.type == TOKEN_EOF)
        {
            putchar('\n');
            return 0;
        }
    }

    puts(" LIMIT");
    return 2;
}
