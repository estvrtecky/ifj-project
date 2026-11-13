#include "codegen.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static FILE *out_file = NULL;
static int unique_id_counter = 0;



static void translate_function(AST_node *func_node);
static void translate_keyword(AST_node *stat_node);
static void translate_expresion(AST_node *expresion_node);
static void generate_builtin_functions(void);

static void translate_function(AST_node *func_node){
  //checks if node is node of a function
  if(func_node == NULL){
    return;
  }
  //beginning of function
  AST_func_def_data *function = &func_node->as.func_def;
  fprintf(out_file, "LABEL $%s\n", function->name);
  fprintf(out_file, "CREATEFRAME\n");
  fprintf(out_file, "PUSHFRAME\n");
  
  //params of function
  for (size_t idx=function->param_count; idx > 0; idx--){
    size_t jdx = idx - 1;
    fprintf(out_file, "DEFVAR LF@%s$%d\n", function->params[jdx].name, function->params[jdx].sym_id);
  }
  //unpush parameters of function
  for(size_t idx=function->param_count; idx > 0; idx--){
    size_t jdx = idx - 1;
    fprintf(out_file, "POPS LF@%s$%d\n", function->params[jdx].name, function->params[jdx].sym_id);
    }
  //translate inside block of function  
  translate_keyword(function->body);
  fprintf(out_file, "PUSHS nil@nil\n");
  fprintf(out_file, "POPFRAME\n");
  fprintf(out_file, "RETURN\n");
}

static void translate_keyword(AST_node *stat_node){
  //to do
}

static void translate_expresion(AST_node *expresion_node){
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
  unique_id_counter=0;
  out_file = o;
    fprintf(out_file, ".IFJcode26\n");
    fprintf(out_file, "DEFVAR GF@_t0\n");
    fprintf(out_file, "DEFVAR GF@_t1\n");
    fprintf(out_file, "DEFVAR GF@_t2\n");
    fprintf(out_file, "DEFVAR GF@_t3\n");
  bool has_main = false;
  for(size_t idx = 0; idx< program->as.program.funcs.count; idx++){
    AST_node *func = program->as.program.funcs.items[idx];
    if(func->kind == AST_func_def && func->as.func_def.is_main){
      fprintf(out_file, "CALL $%s\n", func->as.func_def.name);
      has_main = true;
    }
    }
    
  if(!has_main){
    return ERROR_INTERNAL;
  }
  fprintf(out_file, "EXIT int@0\n\n");
  
  for(size_t idx=0;idx<program->as.program.funcs.count; idx++){
    translate_function(program->as.program.funcs.items[idx]);
  }
  
  return OK;
}
