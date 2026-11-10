/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       sema.h
   # Brief:      
   # Authors:    Adam Vallus     (xvallua00)
*/
#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "symtable.h"
#include "hashtable.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum{
        FUNC_NOT_VISITED,
        FUNC_IN_PROGRESS,
        FUNC_DONE
    } Func_state;

        
    typedef struct {
        AST_node *node;
        Func_state state;
    } Func_info;

    typedef struct {
        Symtable *st;
        Table *funcs;
        int error;
        AST_node *func_node;
    } Sema_ctx;

    int sema_check(AST_node *program);

#ifdef __cplusplus
}
#endif

#endif
