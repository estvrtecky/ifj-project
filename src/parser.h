/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    parser.h
    # Brief:   Parser interface
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "error.h"

ErrorCode parser_init(void);
ErrorCode parse_program(AST_node **ast);

#endif