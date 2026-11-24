/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       hashtable.h
   # Brief:      Hash table with open addressing and implicit chaining
   #             (Variant TRP-izp), using double hashing
   #             Public interface of the hash table module
   # Authors:    Damian Volosiny (xvolosd00)
   #             Adam Vallus     (xvallua00)
*/
#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* result codes returned by the hash table operations*/
    typedef enum
    {
        HT_OK,        /* operation succeeded */
        HT_EXISTING,  /* key is already in the table*/
        HT_NOT_FOUND, /* key is not in the table*/
        HT_MEM_ERROR  /* memory allocation failed */
    } Return_state;

    /* opaque hash table type, its contents are hidden in hastable.c*/
    typedef struct Table Table;

    typedef void (*Ht_free_fn)(void *);

    Table *ht_init(size_t size, Ht_free_fn free_value);

    Return_state ht_search(const Table *table, const char *key, void **output);

    Return_state ht_insert(Table *table, const char *key, void *value);

    Return_state ht_delete(Table *table, const char *key);

    void ht_free(Table *table);

#ifdef __cplusplus
}
#endif

#endif
