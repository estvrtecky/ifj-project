#include "codegen.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static FILE *out_file = NULL;
static int unique_id_counter = 0;
static bool header_printed = false;



static void translate_function(AST_node *func_node);
static void translate_statement(AST_node *stat_node);
static void translate_expresion(AST_node *expresion_node);
static void generate_builtin_functions(void);

static void translate_function(AST_node *func_node){
  //to do
}

static void translate_statement(AST_node *func_node){
  //to do
}

static void translate_expresion(AST_node *func_node){
  //to do
}

int generate_code(AST_node *program, FILE *o)
{
  if(program == NULL){
    return ERROR_INTERNAL;
  }
  if(o == NULL){
    return ERROR_INTERNAL;
  }
  if(program->kind != AST_program){
    return ERROR_INTERNAL;
  }
  out_file = o;
  if(header_printed == false){
    fprintf(out_file, ".IFJcode26\n");
    fprintf(out_file, "DEFVAR GF@_tmp1\n");
    fprintf(out_file, "DEFVAR GF@_tmp2\n");
    fprintf(out_file, "DEFVAR GF@_tmp3\n");
    header_printed = true;
  }
  for(size_t idx = 0; idx < program->as.program.funcs.count; idx++){
    AST_node *func = program->as.program.funcs.items[idx];
    if(func->kind == AST_func_def && func->as.func_def.is_main){
      fprintf(out_file, "CALL $func_%s\n", func->as.func_def.name);
      fprintf(out_file, "CLEARS\n");
    }
  }
  fprintf(out_file, "EXIT int@0\n\n");
  
  for(size_t idx=0< idx<program->as.program.funcs.count; idx++){
    translate_function(program->as.program.funcs.items[idx]);
  }
  
  return OK;
}
