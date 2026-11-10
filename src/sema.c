/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       sema.c
   # Brief:      
   # Authors:    Adam Vallus     (xvallua00)
*/


#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "sema.h"
#include "symtable.h"
#include "hashtable.h"

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
    AST_node *func_node; // Function which is analyzed at the moment
} Sema_ctx;


static void set_error(Sema_ctx *ctx, int err_code){
    if(ctx->error != 0){
        return;
    }
    ctx->error = err_code;
}

static bool failed(Sema_ctx *ctx){
    return ctx->error;
}

static int sym_status_to_error(Sym_status status){
    switch (status) {
    case SYM_OK:
        return 0;
    case SYM_REDEFINED:
        return 4;
    default:
        return 99;
    }
}

static void wrap_conv(Sema_ctx *ctx, AST_node **slot){
    AST_node *conv = calloc(1, sizeof(AST_node));
    if (conv == NULL){
        set_error(ctx, 99);
        return;
    }
    conv->is_result = -1;
    conv->kind = AST_conv_int_to_double;
    conv->as.conv.operand = *slot;
    conv->line = (*slot)->line;
    conv->data_type = TYPE_DOUBLE;
    *slot = conv;
}

static void coerce(Sema_ctx *ctx, Data_types target_type, AST_node **slot, int err_code){
    Data_types src = (*slot)->data_type;
    if (target_type == TYPE_ANY || target_type == src){
        return;
    }
    else if (target_type == TYPE_DOUBLE && src == TYPE_INT){
        wrap_conv(ctx, slot);
    }
    else {
        set_error(ctx, err_code);
    }
}