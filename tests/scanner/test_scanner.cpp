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

