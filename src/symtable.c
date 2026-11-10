/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       symtable.c
   # Brief:      Symbol table built on top of the hash table module
   #             Scope stack as a linked list of hash tables,
   #             separate tables for user and built-in functions
   # Authors:    Damian Volosiny (xvolosd00)
   #             Adam Vallus     (xvallua00)
*/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "symtable.h"
#include "hashtable.h"

struct Var_symbol
{
    Data_types data_type;
    bool is_mutable;
    bool is_param;
    size_t unique_id;
};
struct Func_symbol
{
    size_t parameter_count;
    Data_types *parameter_types;
    Data_types return_type;
    bool return_explicit;
    bool builtin;
};
typedef struct Scope Scope;
struct Scope
{
    Table *table;
    Scope *parent;
};
struct Symtable
{
    Scope *top_scope;
    Table *user_func_table;
    Table *builtin_func_table;
    size_t next_id;
};

typedef struct
{
    const char *name;
    size_t param_count;
    Data_types param_types[3];
    Data_types return_type;
} Builtin_func;

static const Builtin_func builtin_func_array[] = {
    {"print", 1, {TYPE_ANY}, TYPE_UNIT},
    {"readLine", 0, {TYPE_UNKNOWN}, TYPE_STRING},
    {"readInt", 0, {TYPE_UNKNOWN}, TYPE_INT},
    {"readDouble", 0, {TYPE_UNKNOWN}, TYPE_DOUBLE},
    {"toStr", 1, {TYPE_ANY}, TYPE_STRING},
    {"toInt", 1, {TYPE_DOUBLE}, TYPE_INT},
    {"strLength", 1, {TYPE_STRING}, TYPE_INT},
    {"subStr", 3, {TYPE_STRING, TYPE_INT, TYPE_INT}, TYPE_STRING},
    {"ord", 2, {TYPE_STRING, TYPE_INT}, TYPE_INT},
    {"chr", 1, {TYPE_INT}, TYPE_STRING}};

static void free_func_symbol(void *func_symbol)
{
    if (func_symbol == NULL)
    {
        return;
    }
    Func_symbol *f = func_symbol;
    free(f->parameter_types);
    free(f);
}

static Var_symbol *find_var(const Symtable *symtable, const char *name)
{
    Scope *scope = symtable->top_scope;
    while (scope != NULL)
    {
        void *found;
        Return_state result = ht_search(scope->table, name, &found);
        if (result == HT_OK)
        {
            return (Var_symbol *)found;
        }
        else
        {
            scope = scope->parent;
        }
    }
    return NULL;
}

static Func_symbol *create_func_symbol(size_t param_count, const Data_types *param_types, Data_types return_type, bool return_explicit, bool builtin)
{
    Func_symbol *func_symbol = malloc(sizeof(Func_symbol));
    if (func_symbol == NULL)
    {
        return NULL;
    }
    func_symbol->builtin = builtin;
    func_symbol->parameter_count = param_count;
    func_symbol->return_type = return_type;
    func_symbol->return_explicit = return_explicit;

    if (param_count == 0)
    {
        func_symbol->parameter_types = NULL;
    }
    else
    {
        func_symbol->parameter_types = malloc(sizeof(Data_types) * param_count);
        if (func_symbol->parameter_types == NULL)
        {
            free(func_symbol);
            return NULL;
        }
        memcpy(func_symbol->parameter_types, param_types, sizeof(Data_types) * param_count);
    }
    return func_symbol;
}

Symtable *st_init(void)
{
    Symtable *symtable = calloc(1, sizeof(Symtable));
    if (symtable == NULL)
    {
        return NULL;
    }

    symtable->user_func_table = ht_init(8, free_func_symbol);
    if (symtable->user_func_table == NULL)
    {
        st_free(symtable);
        return NULL;
    }

    symtable->builtin_func_table = ht_init(16, free_func_symbol);
    if (symtable->builtin_func_table == NULL)
    {
        st_free(symtable);
        return NULL;
    }

    for (size_t i = 0; i < (sizeof(builtin_func_array) / sizeof(builtin_func_array[0])); i++)
    {
        Func_symbol *func_symbol = create_func_symbol(builtin_func_array[i].param_count, builtin_func_array[i].param_types, builtin_func_array[i].return_type, true, true);
        if (func_symbol == NULL)
        {
            st_free(symtable);
            return NULL;
        }
        Return_state result = ht_insert(symtable->builtin_func_table, builtin_func_array[i].name, func_symbol);
        if (result != HT_OK)
        {
            free_func_symbol(func_symbol);
            st_free(symtable);
            return NULL;
        }
    }

    return symtable;
}

void st_free(Symtable *symtable)
{
    if (symtable == NULL)
    {
        return;
    }

    while (symtable->top_scope != NULL)
    {
        st_pop_scope(symtable);
    }
    ht_free(symtable->user_func_table);
    ht_free(symtable->builtin_func_table);
    free(symtable);
}

Sym_status st_push_scope(Symtable *symtable)
{
    if (symtable == NULL)
    {
        return SYM_MEM_ERROR;
    }
    Scope *scope = calloc(1, sizeof(Scope));
    if (scope == NULL)
    {
        return SYM_MEM_ERROR;
    }
    scope->table = ht_init(8, free);
    if (scope->table == NULL)
    {
        free(scope);
        return SYM_MEM_ERROR;
    }
    scope->parent = symtable->top_scope;
    symtable->top_scope = scope;
    return SYM_OK;
}

void st_pop_scope(Symtable *symtable)
{
    if (symtable == NULL)
    {
        return;
    }
    if (symtable->top_scope == NULL)
    {
        return;
    }
    Scope *temp = symtable->top_scope;
    symtable->top_scope = symtable->top_scope->parent;
    ht_free(temp->table);
    free(temp);
}

Sym_status st_define_var(Symtable *symtable, const char *name, Data_types type, bool is_mutable, bool is_param)
{
    if (symtable == NULL)
    {
        return SYM_MEM_ERROR;
    }
    if (symtable->top_scope == NULL)
    {
        return SYM_NO_SCOPE;
    }
    Var_symbol *var_symbol = malloc(sizeof(Var_symbol));
    if (var_symbol == NULL)
    {
        return SYM_MEM_ERROR;
    }
    var_symbol->data_type = type;
    var_symbol->is_mutable = is_mutable;
    var_symbol->is_param = is_param;
    var_symbol->unique_id = symtable->next_id;
    Return_state result = ht_insert(symtable->top_scope->table, name, var_symbol);
    switch (result)
    {
    case HT_OK:
        symtable->next_id++;
        return SYM_OK;
    case HT_EXISTING:
        free(var_symbol);
        return SYM_REDEFINED;
    case HT_MEM_ERROR:
        free(var_symbol);
        return SYM_MEM_ERROR;
    default:
        free(var_symbol);
        return SYM_MEM_ERROR;
    }
}

const Var_symbol *st_lookup_var(const Symtable *symtable, const char *name)
{
    if (symtable == NULL)
    {
        return NULL;
    }
    return find_var(symtable, name);
}

Sym_status st_set_var_type(Symtable *symtable, const char *name, Data_types type)
{
    if (symtable == NULL)
    {
        return SYM_MEM_ERROR;
    }
    Var_symbol *var_symbol = find_var(symtable, name);
    if (var_symbol == NULL)
    {
        return SYM_NOT_FOUND;
    }
    var_symbol->data_type = type;
    return SYM_OK;
}

Data_types st_var_type(const Var_symbol *var_symbol)
{
    if (var_symbol == NULL)
    {
        return TYPE_UNKNOWN;
    }
    return var_symbol->data_type;
}

bool st_var_mutable(const Var_symbol *var_symbol)
{
    if (var_symbol == NULL)
    {
        return false;
    }
    return var_symbol->is_mutable;
}

bool st_var_param(const Var_symbol *var_symbol)
{
    if (var_symbol == NULL)
    {
        return false;
    }
    return var_symbol->is_param;
}

size_t st_var_id(const Var_symbol *var_symbol)
{
    if (var_symbol == NULL)
    {
        return 0;
    }
    return var_symbol->unique_id;
}

Sym_status st_define_func(Symtable *symtable, const char *name, size_t param_count, const Data_types *param_types, Data_types return_type, bool return_explicit)
{
    if (symtable == NULL)
    {
        return SYM_MEM_ERROR;
    }
    if (param_count > 0 && param_types == NULL)
    {
        return SYM_MEM_ERROR;
    }

    Func_symbol *func_symbol = create_func_symbol(param_count, param_types, return_type, return_explicit, false);
    if (func_symbol == NULL)
    {
        return SYM_MEM_ERROR;
    }
    Return_state result = ht_insert(symtable->user_func_table, name, func_symbol);
    switch (result)
    {
    case HT_OK:
        return SYM_OK;
    case HT_EXISTING:
        free_func_symbol(func_symbol);
        return SYM_REDEFINED;
    case HT_MEM_ERROR:
        free_func_symbol(func_symbol);
        return SYM_MEM_ERROR;
    default:
        free_func_symbol(func_symbol);
        return SYM_MEM_ERROR;
    }
}

const Func_symbol *st_lookup_func(const Symtable *symtable, const char *name)
{
    if (symtable == NULL)
    {
        return NULL;
    }
    void *func_symbol;
    Return_state result = ht_search(symtable->user_func_table, name, &func_symbol);
    if (result == HT_OK)
    {
        return (Func_symbol *)func_symbol;
    }
    else
    {
        result = ht_search(symtable->builtin_func_table, name, &func_symbol);
        if (result == HT_OK)
        {
            return (Func_symbol *)func_symbol;
        }
    }
    return NULL;
}

Sym_status st_set_return_type(Symtable *symtable, const char *name, Data_types return_type)
{
    if (symtable == NULL)
    {
        return SYM_MEM_ERROR;
    }
    void *func_symbol;
    Return_state result = ht_search(symtable->user_func_table, name, &func_symbol);
    if (result != HT_OK)
    {
        return SYM_NOT_FOUND;
    }
    ((Func_symbol *)func_symbol)->return_type = return_type;
    return SYM_OK;
}

size_t st_func_param_count(const Func_symbol *func_symbol)
{
    if (func_symbol == NULL)
    {
        return 0;
    }
    return func_symbol->parameter_count;
}

Data_types st_func_param_type(const Func_symbol *func_symbol, size_t index)
{
    if (func_symbol == NULL)
    {
        return TYPE_UNKNOWN;
    }
    if (func_symbol->parameter_count <= index)
    {
        return TYPE_UNKNOWN;
    }
    return func_symbol->parameter_types[index];
}

Data_types st_func_return_type(const Func_symbol *func_symbol)
{
    if (func_symbol == NULL)
    {
        return TYPE_UNKNOWN;
    }
    return func_symbol->return_type;
}

bool st_func_return_explicit(const Func_symbol *func_symbol)
{
    if (func_symbol == NULL)
    {
        return false;
    }
    return func_symbol->return_explicit;
}

bool st_func_builtin(const Func_symbol *func_symbol)
{
    if (func_symbol == NULL)
    {
        return false;
    }
    return func_symbol->builtin;
}
