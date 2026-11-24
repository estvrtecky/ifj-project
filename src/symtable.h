/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       symtable.h
   # Brief:      Symbol table built on top of the hash table module
   #             Public interface: scopes, variables, user and built-in functions
   # Authors:    Damian Volosiny (xvolosd00)
   #             Adam Vallus     (xvallua00)
*/
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        SYM_OK,
        SYM_REDEFINED,
        SYM_NOT_FOUND,
        SYM_NO_SCOPE,
        SYM_MEM_ERROR
    } Sym_status;

    typedef struct Var_symbol Var_symbol;
    typedef struct Func_symbol Func_symbol;
    typedef struct Symtable Symtable;
    typedef struct Scope Scope;

    Symtable *st_init(void);
    void st_free(Symtable *symtable);

    Sym_status st_push_scope(Symtable *symtable);
    void st_pop_scope(Symtable *symtable);

    Sym_status st_define_var(Symtable *symtable, const char *name, Data_types type, bool is_mutable, bool is_param);
    const Var_symbol *st_lookup_var(const Symtable *symtable, const char *name);

    Sym_status st_set_var_type(Symtable *symtable, const char *name, Data_types type);

    Data_types st_var_type(const Var_symbol *var_symbol);
    bool st_var_mutable(const Var_symbol *var_symbol);
    bool st_var_param(const Var_symbol *var_symbol);
    size_t st_var_id(const Var_symbol *var_symbol);

    Sym_status st_define_func(Symtable *symtable, const char *name, size_t param_count, const Data_types *param_types, Data_types return_type, bool return_explicit);
    const Func_symbol *st_lookup_func(const Symtable *symtable, const char *name);
    Sym_status st_set_return_type(Symtable *symtable, const char *name, Data_types return_type);

    size_t st_func_param_count(const Func_symbol *func_symbol);
    Data_types st_func_param_type(const Func_symbol *func_symbol, size_t index);
    Data_types st_func_return_type(const Func_symbol *func_symbol);
    bool st_func_return_explicit(const Func_symbol *func_symbol);
    bool st_func_builtin(const Func_symbol *func_symbol);

    Sym_status st_detach_scopes(Symtable *symtable, Scope **output);
    Sym_status st_attach_scopes(Symtable *symtable, Scope *saved);

#ifdef __cplusplus
}
#endif

#endif
