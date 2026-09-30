//At this points Starts AI Generated part of code. I used Gemini version 3.8 Flash
#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
    #include "scanner.h"
}

class ScannerTest : public ::testing::Test {
protected:
    FILE* original_stdin = nullptr;

    void SetUp() override {
        original_stdin = stdin;
    }

    void TearDown() override {
        if (stdin != original_stdin && stdin != nullptr) {
            fclose(stdin);
        }
        stdin = original_stdin;
    }

    void open_input(const char* filepath) {
        FILE* redirect = freopen(filepath, "r", stdin);
        ASSERT_NE(redirect, nullptr) << "Chyba: Subor sa nepodarilo otvorit: ";
        scanner_init();
    }
};

//At this point ends AI Generated part of code


TEST_F(ScannerTest, AllTokenTypes) {
    open_input("inputs/all_tokens.txt");
    token_t token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DEF);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DO);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ID);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_DOUBLE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ELSE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_IF);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_IMPORT);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ID);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_INT);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_RETURN);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_STRING);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_THEN);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_UNIT);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_VAL);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_VAR);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_WHILE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_BOOLEAN);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ASSING);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_CASE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE_CLASS);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_SEMICOLON);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_EXTENSION);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_INT);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_FOR);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DOUBLE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_LIST);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_MATCH);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_TYPE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DIVIDE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_MULTIPLY);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ADD);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_SUB);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_L_BRACE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_R_BRACE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_LS_BRACE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_RS_BRACE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_EQ);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_NQ);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_LESS);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_MORE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_AND);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_OR);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_NOT);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_LESS_EQ);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_MORE_EQ);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DOUBLE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_DOUBLE);
    token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_EOF);  
}

TEST_F(ScannerTest, ErrorTabulator){
    open_input("inputs/tab_error.txt");
    token_t token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ERROR);
}

TEST_F(ScannerTest, Error_at){
    open_input("inputs/at_error.txt");
    token_t token = get_next_token();
    ASSERT_EQ(token.type, TOKEN_ERROR);
}
    

