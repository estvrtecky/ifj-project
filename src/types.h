/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       types.h
   # Brief:      Data types of the IFJ26 language shared by the symbol table,
   #             AST, semantic analysis and code generation
   # Authors:    Damian Volosiny (xvolosd00)
   #             Adam Vallus     (xvallua00)
*/
#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        TYPE_UNKNOWN,
        TYPE_ANY,
        TYPE_STRING,
        TYPE_DOUBLE,
        TYPE_INT,
        TYPE_BOOLEAN,
        TYPE_UNIT
    } Data_types;

#ifdef __cplusplus
}
#endif

#endif
