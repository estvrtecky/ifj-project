/*
    # Project: Implementation of the IFJ26 imperative language compiler
    # File:    error.h
    # Brief:   Global error codes
    # Authors: Erik Stvrtecky (xstvrte00)
*/

#ifndef ERROR_H
#define ERROR_H

typedef enum
{
    OK = 0,
    ERROR_LEX = 1,
    ERROR_SYNTAX = 2,
    ERROR_UNDEFINED = 3,
    ERROR_REDEFINED = 4,
    ERROR_ARGS = 5,
    ERROR_TYPE = 6,
    ERROR_ASSIGN = 7,
    ERROR_SEM_OTHER = 10,
    ERROR_INTERNAL = 99
} ErrorCode;

#endif