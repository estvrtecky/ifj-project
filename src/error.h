#ifndef ERROR_H
#define ERROR_H

typedef enum
{
    OK = 0,
    ERROR_LEX = 1,

    ERROR_INTERNAL = 99
} ErrorCode;

#endif