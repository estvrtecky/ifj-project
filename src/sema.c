/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       sema.c
   # Brief:      Semantic analyze 
   # Authors:    Adam Vallus     (xvallua00)
*/

// Tests of functions in this file was executed by Claude Sonnet 5.5

#include <stdlib.h>
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
    AST_node *main_node;
} Sema_ctx;

static void visit_block(Sema_ctx *ctx, AST_node *block);
static void visit_stmt(Sema_ctx *ctx, AST_node *stmt);
static Data_types visit_expr(Sema_ctx *ctx, AST_node *exprt);
static Data_types visit_call(Sema_ctx *ctx, AST_node *call);
static Data_types visit_binop(Sema_ctx *ctx, AST_node *binop);
static Data_types visit_unop(Sema_ctx *ctx, AST_node *unop);
static Data_types visit_member(Sema_ctx *ctx, AST_node *f);
static void visit_func(Sema_ctx *ctx, AST_node *f);
static Data_types tail_type(Sema_ctx *ctx, AST_node *stmt, Data_types target);


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

static bool is_numeric(Data_types param){
    return (param == TYPE_INT || param == TYPE_DOUBLE)? true : false;
}

static void unify_numeric(Sema_ctx *ctx, AST_node *binop){
    if (binop->as.binop.left->data_type == TYPE_INT){
        wrap_conv(ctx, &binop->as.binop.left);
    }
    else{
        wrap_conv(ctx, &binop->as.binop.right);
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
        if(status != SYM_OK){
            set_error(ctx, sym_status_to_error(status));
            return;
        }

        Func_info *info = malloc(sizeof(Func_info));
        if(info == NULL){
            set_error(ctx, 99);
            return;
        }

        info->node = program->as.program.funcs.items[i];
        info->state = FUNC_NOT_VISITED;
        if(ht_insert(ctx->funcs, f->name, info) != HT_OK){
            free(info);
            set_error(ctx, 99);
            return;
        }
    }
}

static void check_main(Sema_ctx *ctx, AST_node *program){
    size_t main_count = 0;
    AST_node *main_func = NULL;
    for (size_t i = 0; i < program->as.program.funcs.count; i++){
        AST_node *f = program->as.program.funcs.items[i];
        if(f->as.func_def.is_main == true){
            main_count++;
            main_func = f;
        }
    }
    if(main_count == 0){
        set_error(ctx, 3);
        return;
    }
    if(main_count > 1){
        set_error(ctx, 4);
        return;
    }
    ctx->main_node = main_func;
    if(main_func->as.func_def.param_count > 0){
        set_error(ctx, 5);
        return;
    }
    if(main_func->as.func_def.return_explicit == true && main_func->as.func_def.return_type != TYPE_UNIT){
        set_error(ctx, 6);
        return;
    }   
}

static AST_node *last_stmt(AST_node *block){
    AST_node_list *list = &block->as.block.stmts;
    if(list->count == 0){
        return NULL;
    }
    return list->items[list->count - 1];
}

static Data_types tail_type(Sema_ctx *ctx, AST_node *stmt, Data_types target){
    if(stmt == NULL){
        return TYPE_UNIT;
    }
    switch (stmt->kind){
    case AST_return:
        return target;
    case AST_expr_stmt:
        if(target != TYPE_UNKNOWN){
            coerce(ctx, target, &stmt->as.expr_stmt.expr, 6);
        }
        stmt->as.expr_stmt.expr->is_result = true;
        return stmt->as.expr_stmt.expr->data_type;
    case AST_if:
        if(stmt->as.if_stmt.else_block == NULL){
            return TYPE_UNIT;
        }
        Data_types t_then = tail_type(ctx, last_stmt(stmt->as.if_stmt.then_block), target);
        Data_types t_else = tail_type(ctx, last_stmt(stmt->as.if_stmt.else_block), target);
        if(failed(ctx)){
            return TYPE_UNKNOWN;
        }
        if(t_then != t_else){
            return TYPE_UNKNOWN;
        }
        return t_then;
    case AST_var_def:
    case AST_assign:
    case AST_while:
        return TYPE_UNIT;
    default:
        set_error(ctx, 99);
        return TYPE_UNKNOWN;
    }
}

static void analyze_func(Sema_ctx *ctx, AST_node *f){
    void *found;
    Func_info *info;
    if(ht_search(ctx->funcs, f->as.func_def.name, &found) != HT_OK){
        set_error(ctx, 99);
        return;
    }
    info = found;
    if(info->state == FUNC_DONE){
        return;
    }
    info->state = FUNC_IN_PROGRESS;
    visit_func(ctx, f);
    info->state = FUNC_DONE;
}

// This block was co-developed with Claude Sonet 5.5 with promt: "Can you help me with this function not working properly" - Suggested functions st_attach_scopes & st_detach_scopes in symtable

static void ensure_return_type(Sema_ctx *ctx, const char *name){
    void *found = NULL;
    if (ht_search(ctx->funcs, name, &found) != HT_OK){
        set_error(ctx, 99);
        return;
    }
    Func_info *info = found;
    if(info->state == FUNC_DONE){
        return;
    }
    if(info->state == FUNC_IN_PROGRESS){
        set_error(ctx, 10);
        return;
    }

    Scope *saved = NULL;
    if (st_detach_scopes(ctx->st, &saved) != SYM_OK){
        set_error(ctx, 99);
        return;
    }
    analyze_func(ctx, info->node);
    if (st_attach_scopes(ctx->st, saved) != SYM_OK){
        set_error(ctx, 99);
        return;
    }
}

// End of block co-developed with Claude Sonet 5.5

static void check_body_end(Sema_ctx *ctx, AST_node *fn){
    AST_func_def_data *f = &fn->as.func_def;

    if (f->return_explicit && f->return_type == TYPE_UNIT){
        return;
    }
    Data_types target = (f->return_explicit)? f->return_type : TYPE_UNKNOWN;
    Data_types t = tail_type(ctx, last_stmt(f->body), target);
    if(failed(ctx)){
        return;
    }
    if(f->return_explicit){
        if(t != f->return_type){
            set_error(ctx, 6);
            return;
        }
    }
    if(t == TYPE_UNKNOWN){
        set_error(ctx, 6);
        return;
    }
    f->return_type = t;
    Sym_status status = st_set_return_type(ctx->st, f->name, f->return_type);
    if (status != SYM_OK){
        set_error(ctx, 99);
    }
}

static void visit_var_def(Sema_ctx *ctx, AST_node *var_def){
    AST_var_def_data *data = &var_def->as.var_def;
    Data_types init_type = visit_expr(ctx, data->init);
    if (failed(ctx)){
        return;
    }
    Data_types var_type = init_type;

    if (data->decl_type != TYPE_UNKNOWN){
        coerce(ctx, data->decl_type, &data->init, 6);
        if(failed(ctx)){
            return;
        }
        var_type = data->decl_type;
    }

    if (var_type == TYPE_UNKNOWN){
        set_error(ctx, 6);
        return;
    }

    Sym_status status = st_define_var(ctx->st, data->name, var_type, data->is_mutable, false);
    if (status != SYM_OK){
        set_error(ctx, sym_status_to_error(status));
        return;
    }
    const Var_symbol *sym = st_lookup_var(ctx->st, data->name);
    var_def->sym_id = (int) st_var_id(sym);
}

static void visit_assign(Sema_ctx *ctx, AST_node *assign){
    AST_assign_data *asgn = &assign->as.assign;
    const Var_symbol *sym = st_lookup_var(ctx->st, asgn->name);
    if (sym == NULL){
        set_error(ctx, 3);
        return;
    }
    if (!st_var_mutable(sym) || st_var_param(sym)){
        set_error(ctx, 7);
        return;
    }
    visit_expr(ctx, asgn->value);
    if (failed(ctx)){
        return;
    }

    coerce(ctx, st_var_type(sym), &asgn->value, 6);
    if(failed(ctx)){
        return;
    }
    assign->sym_id = (int) st_var_id(sym);
}

static void visit_if(Sema_ctx *ctx, AST_node *if_stmt){
    AST_if_data *if_data = &if_stmt->as.if_stmt;
    Data_types cond_type = visit_expr(ctx, if_data->cond);
    if (failed(ctx)){
        return;
    }
    if (cond_type != TYPE_BOOLEAN){
        set_error(ctx, 6);
        return;
    }
    visit_block(ctx, if_data->then_block);
    if (failed(ctx)){
        return;
    }
    if (if_data->else_block != NULL){
        visit_block(ctx, if_data->else_block);
    }
}

static void visit_while(Sema_ctx *ctx, AST_node *while_stmt){
    AST_while_data *while_data = &while_stmt->as.while_stmt;
    Data_types cond_type = visit_expr(ctx, while_data->cond);
    if (failed(ctx)){
        return;
    }
    if (cond_type != TYPE_BOOLEAN){
        set_error(ctx, 6);
        return;
    }
    visit_block(ctx, while_data->body);
}

static void visit_return(Sema_ctx *ctx, AST_node *return_stmt){
    AST_func_def_data *f = &ctx->func_node->as.func_def;
    if (!f->return_explicit){
        set_error(ctx, 10);
        return;
    }

    AST_node *value = return_stmt->as.return_stmt.value;
    if (value == NULL){
        if (f->return_type != TYPE_UNIT){
            set_error(ctx, 6);
        }
        return;
    }

    visit_expr(ctx, value);
    if (failed(ctx)){
        return;
    }

    if (f->return_type == TYPE_UNIT){
        return;
    }

    coerce(ctx, f->return_type, &return_stmt->as.return_stmt.value, 6);
}

static void visit_stmt(Sema_ctx *ctx, AST_node *stmt){
    switch (stmt->kind)
    {
    case AST_var_def:
        visit_var_def(ctx, stmt);
        break;
    case AST_assign:
        visit_assign(ctx, stmt);
        break;
    case AST_if:
        visit_if(ctx, stmt);
        break;
    case AST_while:
        visit_while(ctx, stmt);
        break;
    case AST_expr_stmt:
        visit_expr(ctx, stmt->as.expr_stmt.expr);
        break;
    case AST_return:
        visit_return(ctx, stmt);
        break;
    default:
        set_error(ctx, 99);
        break;
    }
}

static Data_types visit_expr(Sema_ctx *ctx, AST_node *expr){
    switch (expr->kind){
    case AST_int_lit:
        expr->data_type = TYPE_INT;
        break;
    case AST_double_lit:
        expr->data_type = TYPE_DOUBLE;
        break;
    case AST_string_lit:
        expr->data_type = TYPE_STRING;
        break;
    case AST_unit_lit:
        expr->data_type = TYPE_UNIT;
        break;
    case AST_bool_lit:
        expr->data_type = TYPE_BOOLEAN;
        break;
    case AST_id:
        {
        const Var_symbol *symbol = st_lookup_var(ctx->st, expr->as.id.name);
        if(symbol == NULL){
            set_error(ctx, 3);
            return TYPE_UNKNOWN;
        }
        expr->data_type = st_var_type(symbol);
        expr->sym_id = (int) st_var_id(symbol);
        }
        break;
    case AST_binop:
        expr->data_type = visit_binop(ctx, expr);
        break;
    case AST_unop:
        expr->data_type = visit_unop(ctx, expr);
        break;
    case AST_call:
        expr->data_type = visit_call(ctx, expr);
        break;
    case AST_member:
        expr->data_type = visit_member(ctx, expr);
        break;
    case AST_conv_int_to_double:
        break;
    default:
        set_error(ctx, 99);
        return TYPE_UNKNOWN;
    }
    return expr->data_type;
}

static Data_types visit_binop(Sema_ctx *ctx, AST_node *binop){
    Data_types l = visit_expr(ctx, binop->as.binop.left);
    if (failed(ctx)){
        return TYPE_UNKNOWN;
    }
    Data_types r = visit_expr(ctx, binop->as.binop.right);
    if (failed(ctx)){
        return TYPE_UNKNOWN;
    }
    if (binop->as.binop.op ==  OP_add || binop->as.binop.op ==  OP_sub || binop->as.binop.op ==  OP_mul || binop->as.binop.op ==  OP_div){
        if( binop->as.binop.op == OP_add &&
            l == TYPE_STRING &&
            r == TYPE_STRING){
                return TYPE_STRING;
        }
        if (is_numeric(l) && is_numeric(r) ){
            if(l != r){
                unify_numeric(ctx, binop);
                if(failed(ctx)){
                    return TYPE_UNKNOWN;
                }
                return TYPE_DOUBLE;
            }
            else {
                return l;
            }
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;
    }
    if (binop->as.binop.op == OP_lt || binop->as.binop.op == OP_le || binop->as.binop.op == OP_gt || binop->as.binop.op == OP_ge){
        if(is_numeric(l) && is_numeric(r)){
            if( l != r){
                unify_numeric(ctx, binop);
                if(failed(ctx)){
                    return TYPE_UNKNOWN;
                }
            }
            return TYPE_BOOLEAN;
        }
        if(l == TYPE_STRING && r == TYPE_STRING){
            return TYPE_BOOLEAN;
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;
    }
    
    if (binop->as.binop.op == OP_eq || binop->as.binop.op == OP_ne ){
        if(is_numeric(l) && is_numeric(r)){
            if(l != r){
                unify_numeric(ctx, binop);
                if(failed(ctx)){
                    return TYPE_UNKNOWN;
                }
            }
            return TYPE_BOOLEAN;
        }
        if(l == r){
            return TYPE_BOOLEAN;
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;       
    }

    if (binop->as.binop.op == OP_and || binop->as.binop.op == OP_or){
        if(l == TYPE_BOOLEAN && r == TYPE_BOOLEAN){
            return TYPE_BOOLEAN;
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;       
    }

    set_error(ctx, 6);
    return TYPE_UNKNOWN;
}

static Data_types visit_unop(Sema_ctx *ctx, AST_node *unop){
    Data_types operand = visit_expr(ctx, unop->as.unop.operand);
    if (failed(ctx)){
        return TYPE_UNKNOWN;
    }
    if (unop->as.unop.op == OP_neg || unop->as.unop.op == OP_pos){
        if( operand == TYPE_INT || operand == TYPE_DOUBLE){
            return operand;
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;
    }
    if (unop->as.unop.op == OP_not){
        if( operand == TYPE_BOOLEAN){
            return TYPE_BOOLEAN;
        }
        set_error(ctx, 6);
        return TYPE_UNKNOWN;
    }
    set_error(ctx, 99);
    return TYPE_UNKNOWN;
}

static Data_types visit_call(Sema_ctx *ctx, AST_node *call){
    const Func_symbol *f = st_lookup_func(ctx->st, call->as.call.name);
    if (f == NULL){
        set_error(ctx, 3);
        return TYPE_UNKNOWN;
    }
    if (call->as.call.args.count != st_func_param_count(f)){
        set_error(ctx, 5);
        return TYPE_UNKNOWN;
    }
    for (size_t i = 0; i < call->as.call.args.count; i++){
        visit_expr(ctx, call->as.call.args.items[i]);
        if(failed(ctx)){
            return TYPE_UNKNOWN;
        }
        coerce(ctx, st_func_param_type(f, i), &call->as.call.args.items[i], 5);
        if(failed(ctx)){
            return TYPE_UNKNOWN;
        }
    }
    if (!st_func_builtin(f) && !st_func_return_explicit(f)){
        ensure_return_type(ctx, call->as.call.name);
        if(failed(ctx)){
            return TYPE_UNKNOWN;
        }
    }
    return st_func_return_type(f);
    
}

static Data_types visit_member(Sema_ctx *ctx, AST_node *expr){
    Data_types recv_type = visit_expr(ctx, expr->as.member.receiver);
    if(failed(ctx)){
        return TYPE_UNKNOWN;
    }
    char *mangled = AST_ext_name(recv_type, expr->as.member.name);
    if(mangled == NULL){
        set_error(ctx, 3);
        return TYPE_UNKNOWN;
    }
    void *output = NULL;
    ht_search(ctx->funcs, mangled, &output);
    Func_info *info = output;
    if (info == NULL){
        free(mangled);
        set_error(ctx, 3);
        return TYPE_UNKNOWN;
    }
    if (expr->as.member.has_parens != info->node->as.func_def.has_param_list){
        free(mangled);
        set_error(ctx, 5);
        return TYPE_UNKNOWN;
    }
    AST_node *receiver = expr->as.member.receiver;
    AST_node_list old_args = expr->as.member.args;
    free(expr->as.member.name);
    expr->kind = AST_call;
    expr->as.call.name = mangled;
    AST_node_list new_clear_list = {.capacity = 0, .count = 0, .items = NULL};
    expr->as.call.args = new_clear_list;
    if(AST_list_push(&(expr->as.call.args), receiver) == false){
        set_error(ctx, 99);
        return TYPE_UNKNOWN;
    }
    for (size_t i = 0; i < old_args.count; i++){
        if(AST_list_push(&(expr->as.call.args), old_args.items[i]) == false){
            set_error(ctx, 99);
            return TYPE_UNKNOWN;
        }
    }
    free(old_args.items);
    expr->as.call.has_parens = true;
    return visit_call(ctx, expr);
}

static void visit_block(Sema_ctx *ctx, AST_node *block){
    if(st_push_scope(ctx->st) != SYM_OK){
        set_error(ctx, 99);
        return;
    }
    for (size_t i = 0; i < block->as.block.stmts.count; i++){
        visit_stmt(ctx, block->as.block.stmts.items[i]);
        if(failed(ctx)){
            break;
        }
    }
    st_pop_scope(ctx->st);
}

static void visit_func(Sema_ctx *ctx, AST_node *f){
    if (st_push_scope(ctx->st) != SYM_OK){
        set_error(ctx, 99);
        return;
    }
    for (size_t i = 0; i < f->as.func_def.param_count; i++){
        AST_param p = f->as.func_def.params[i];
        Sym_status status = st_define_var(ctx->st, p.name, p.type, false, true);
        if(status != SYM_OK){
            set_error(ctx, sym_status_to_error(status));
            break;
        }
        const Var_symbol *sym = st_lookup_var(ctx->st, p.name);
        f->as.func_def.params[i].sym_id = (int) st_var_id(sym);
    }
    AST_node *previous = ctx->func_node;
    ctx->func_node = f;
    if (!failed(ctx)){
        visit_block(ctx, f->as.func_def.body);
    }
    if (!failed(ctx)){
        check_body_end(ctx, f);
    }
    ctx->func_node = previous;
    st_pop_scope(ctx->st);   
}

static void pass2_bodies(Sema_ctx *ctx, AST_node *program){
    for (size_t i = 0; i < program->as.program.funcs.count; i++){
        analyze_func(ctx, program->as.program.funcs.items[i]);
        if(failed(ctx)){
            return;
        }
    }
    
}

int sema_analyze(AST_node *program){
    Sema_ctx ctx = {.error = 0, .func_node = NULL};
    ctx.st = st_init();
    if (ctx.st == NULL){
        return 99;
    }
    ctx.funcs = ht_init(8, free);
    if (ctx.funcs == NULL){
        st_free(ctx.st);
        return 99;
    }

    pass1_signatures(&ctx, program);
    if(!failed(&ctx)){
        check_main(&ctx, program);
    }
    if(!failed(&ctx)){
        pass2_bodies(&ctx, program);
    }
    if(!failed(&ctx)){
        AST_func_def_data *m = &ctx.main_node->as.func_def;
        if (!m->return_explicit && m->return_type != TYPE_UNIT){
            set_error(&ctx, 6);
        }
    }
    ht_free(ctx.funcs);
    st_free(ctx.st);
    return ctx.error;
}