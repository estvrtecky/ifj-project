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

typedef struct Func_info_s{
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
    conv->sym_id = -1;
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

static void pass1_signatures(Sema_ctx *ctx, AST_node *program){
    for (size_t i = 0; i < program->as.program.funcs.count; i++){
        
        AST_func_def_data *f = &program->as.program.funcs.items[i]->as.func_def;
        Data_types *types = calloc(f->param_count, sizeof(Data_types));
        if(types == NULL && f->param_count > 0){
            set_error(ctx, 99);
            return;
        }
        
        for (size_t j = 0; j < f->param_count; j++){
            types[j] = f->params[j].type;
        }
        
        Sym_status status = st_define_func(ctx->st, f->name, f->param_count, types, f->return_type, f->return_explicit);
        free(types);
        Func_info *info = malloc(sizeof(Func_info));
        if(info == NULL){
            set_error(ctx, 99);
            return;
        }
        info->node = program->as.program.funcs.items[i];
        info->state = FUNC_NOT_VISITED;
        if(status != SYM_OK){
            free(info);
            set_error(ctx, sym_status_to_error(status));
            return;
        }
        if(ht_insert(ctx->funcs, f->name, info) == HT_MEM_ERROR){
            free(info);
            set_error(ctx, 99);
            return;
        }
    }
}


int sema_analyze(Sema_ctx *ctx,AST_node *program){
    ctx->error = 0;
    ctx->st = st_init();
    if (ctx->st == NULL){
        set_error(ctx, 99);
        return 99;
    }
    ctx->funcs = ht_init(8, free);
    if (ctx->funcs == NULL){
        st_free(ctx->st);
        set_error(ctx, 99);
        return 99;
    }

    pass1_signatures(ctx, program);
    if(!failed(ctx)){
        check_main(ctx, program);
    }
    if(!failed(ctx)){
        pass2_bodies(ctx, program);
    }
    ht_free(ctx->funcs);
    st_free(ctx->st);
    return ctx->error;
}

