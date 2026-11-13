/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       sema.h
   # Brief:      Header file for semantic analysis
   # Authors:    Adam Vallus     (xvallua00)
*/
#ifndef SEMA_H
#define SEMA_H

#include "ast.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int sema_analyze(AST_node *program);

#ifdef __cplusplus
}
#endif

#endif
