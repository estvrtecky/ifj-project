/**
 * Author : Andrea Valekova xvaleka00
 * brief : this file generates ifjcode26 which will be translated by ic26int
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "error.h"
#include <stdio.h>

int generate_code(AST_node *program, FILE *out);

#endif
