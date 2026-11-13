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
  if(stat_node == NULL){
    return;
  }
  if(stat_node->kind == AST_block){
    for(size_t idx=0; idx<stat_node->as.block.stmts.count; idx++){
      translate_keyword(stat_node->as.block.stmts.items[idx]);
    }
  }
  else if(stat_node->kind == AST_expr_stmt){
    AST_node *expresion = stat_node->as.expr_stmt.expr;
    if(expresion == NULL){
      return;
    }
    translate_expresion(expresion);
    fprintf(out_file, "POPS GF@_t0\n");
  }
  else if(stat_node->kind == AST_return){
    AST_node *return_node = stat_node->as.return_stmt.value;
    if(return_node == NULL){
      fprintf(out_file, "PUSHS nil@nil\n");
    }
    else{
      translate_expresion(return_node);
    }
    fprintf(out_file, "POPFRAME\n");
    fprintf(out_file, "RETURN\n");
  }
  else if(stat_node->kind == AST_var_def){
    AST_var_def_data *definition = &stat_node->as.var_def;
    translate_expresion(definition->init);
    fprintf(out_file, "DEFVAR LF@%s$%d\n", definition->name);
    fprintf(out_file, "POPS LF@%s$%d\n", definition->name);
  }
  else if(stat_node->kind == AST_assign){
    AST_assign_data *assign = &stat_node->as.assign;
    translate_expresion(assign->value);
    fprintf(out_file, "POPS LF@%s\n", assign->name);
  }
  else if(stat_node->kind == AST_if){
    AST_if_data *condition = &stat_node->as.if_stmt;
    int id = unique_id_counter++;
    
    translate_expresion(condition->cond);
    fprintf(out_file, "PUSHS bool@false\n");
    fprintf(out_file, "JUMPIFEQS _L%d_else\n", id);
  
    translate_keyword(condition->then_block);
    fprintf(out_file, "JUMP _L%d_end\n", id);
    fprintf(out_file, "LABEL _L%d_else\n", id);
    if(condition->else_block != NULL){
      translate_keyword(condition->else_block);
    }
    fprintf(out_file, "LABEL _L%d_end\n", id);
  }
  else if(stat_node->kind == AST_while){
    AST_while_data *while_loop = &stat_node->as.while_stmt;
    int id = unique_id_counter++;
    fprintf(out_file, "LABEL _L%d_while\n", id);
    translate_expresion(while_loop->cond);
    fprintf(out_file, "PUSHS bool@false\n");
    fprintf(out_file, "JUMPIFEQS _L%d_wend_%d\n", id);
    translate_keyword(while_loop->body);
    fprintf(out_file, "JUMP _L%d_while\n", id);
    fprintf(out_file, "LABEL _L%d_wend\n", id);
    }
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
