/*
   # Project:    Implementation of the IFJ26 imperative language compiler
   # File:       hashtable.c
   # Brief:      Hash table with open addressing and implicit chaining
   #             (Variant TRP-izp), using double hashing
   # Authors:    Damian Volosiny (xvolosd00)
   #             Adam Vallus     (xvallua00)
*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hashtable.h"

enum Slot_state
{
    ht_empty,
    ht_used,
    ht_deleted
};

typedef struct
{
    char *key;
    void *value;
    enum Slot_state state; // Empty (0), Used (1), Deleted(2)
} Item;

struct Table
{
    Item *items;
    size_t capacity;
    size_t used;
    size_t count;
    Ht_free_fn free_value;
};

// Source: Bit Twiddling Hacks, Sean Eron Anderson
static size_t nearest_power_of_2(size_t input_number)
{
    if (input_number <= 1)
        return 1;

    size_t next_power_of_2 = input_number;
    next_power_of_2--;
    next_power_of_2 |= next_power_of_2 >> 1;
    next_power_of_2 |= next_power_of_2 >> 2;
    next_power_of_2 |= next_power_of_2 >> 4;
    next_power_of_2 |= next_power_of_2 >> 8;
    next_power_of_2 |= next_power_of_2 >> 16;
    next_power_of_2 |= next_power_of_2 >> 32;
    next_power_of_2++; // next_power_of_2  = next higher power of 2

    return next_power_of_2;
}

Table *ht_init(size_t size, Ht_free_fn free_value)
{
    size = nearest_power_of_2(size);
    Table *table = calloc(1, sizeof(Table));
    if (table == NULL)
    {
        return NULL;
    }
    table->free_value = free_value;
    table->capacity = size;
    table->count = 0;
    table->used = 0;
    table->items = (Item *)calloc((size), sizeof(Item));
    if (table->items == NULL)
    {
        free(table);
        return NULL;
    }
    return table;
}

static uint32_t FNV_1a_hash(const char *key)
{
    uint32_t hash_0 = 2166136261u;
    while (*key != '\0')
    {
        hash_0 ^= (unsigned char)(*key++);
        hash_0 *= 16777619;
    }
    return hash_0;
}

static size_t hash_index(uint32_t hash, size_t capacity)
{
    return (hash & (capacity - 1));
}

static size_t hash_step(uint32_t hash)
{
    return ((hash >> 16) | 1);
}

static bool find(const Table *table, const char *key, size_t *found_item_index)
{
    uint32_t hash = FNV_1a_hash(key);
    size_t index = hash_index(hash, table->capacity);
    size_t step = hash_step(hash);
    size_t possible_item_index = table->capacity; // invalid index
    for (size_t i = 0; i < table->capacity; i++)
    {
        size_t actual_item_index = (index + (step * i)) % table->capacity;
        switch (table->items[actual_item_index].state)
        {
        case ht_empty:
            if (possible_item_index == table->capacity)
            {
                *found_item_index = actual_item_index;
            }
            else
            {
                *found_item_index = possible_item_index;
            }
            return false;
        case ht_used:
            if (strcmp(table->items[actual_item_index].key, key) == 0)
            {
                *found_item_index = actual_item_index;
                return true;
            }
            break;
        case ht_deleted:
            if (possible_item_index == table->capacity)
                possible_item_index = actual_item_index;
            break;
        default:
            break;
        }
    }
    *found_item_index = possible_item_index;
    return false;
}

Return_state ht_search(const Table *table, const char *key, void **output)
{
    size_t slot;
    if (find(table, key, &slot) == true)
    {
        if (output != NULL)
        {
            *output = table->items[slot].value;
        }
        return HT_OK;
    }
    else
    {
        return HT_NOT_FOUND;
    }
}

static void rehash_replace(Item *items, size_t capacity, char *key, void *value)
{
    uint32_t hash = FNV_1a_hash(key);
    size_t index = hash_index(hash, capacity);
    size_t step = hash_step(hash);

    for (size_t i = 0; i < capacity; i++)
    {
        size_t actual_item_index = (index + (step * i)) % capacity;
        if (items[actual_item_index].state == ht_used)
        {
            continue;
        }
        else
        {
            items[actual_item_index].key = key;
            items[actual_item_index].value = value;
            items[actual_item_index].state = ht_used;
            return;
        }
    }
}

static Return_state rehash(Table *table)
{

    size_t new_capacity = table->capacity;

    if (table->count * 2 >= table->capacity)
    {
        new_capacity *= 2;
    }

    Item *temp;
    temp = calloc(new_capacity, sizeof(Item));
    if (temp == NULL)
    {
        return HT_MEM_ERROR;
    }

    for (size_t i = 0; i < table->capacity; i++)
    {
        if (table->items[i].state == ht_used)
        {
            rehash_replace(temp, new_capacity, table->items[i].key, table->items[i].value);
        }
    }

    free(table->items);

    table->items = temp;
    table->capacity = new_capacity;
    table->used = table->count;

    return HT_OK;
}

Return_state ht_insert(Table *table, const char *key, void *value)
{

    if (((table->used + 1) * 4) > (table->capacity * 3))
    {
        if (rehash(table) == HT_MEM_ERROR)
            return HT_MEM_ERROR;
    }

    size_t index;
    if (find(table, key, &index) == true)
    {
        return HT_EXISTING;
    }
    else
    {
        if (index == table->capacity)
        {
            return HT_MEM_ERROR;
        }
        else
        {
            size_t aloc_size = strlen(key);
            char *aloc = (char *)calloc(aloc_size + 1, sizeof(char));
            if (aloc == NULL)
            {
                // Allocation failed
                return HT_MEM_ERROR;
            }
            table->items[index].key = aloc;

            strcpy(table->items[index].key, key);
            table->items[index].value = value;
            switch (table->items[index].state)
            {
            case ht_empty:
                table->used++;
                /* fall through*/
            case ht_deleted:
                table->count++;
                /* fall through*/
            default:
                table->items[index].state = ht_used;
                break;
            }
            return HT_OK;
        }
    }
}

Return_state ht_delete(Table *table, const char *key)
{
    size_t index;
    if (find(table, key, &index) == false)
    {
        return HT_NOT_FOUND;
    }
    free(table->items[index].key);
    table->items[index].key = NULL;

    if (table->free_value != NULL)
    {
        table->free_value(table->items[index].value);
    }
    table->items[index].value = NULL;
    table->items[index].state = ht_deleted;
    table->count--;
    return HT_OK;
}

void ht_free(Table *table)
{

    if (table != NULL)
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            if (table->items[i].state == ht_used)
            {
                free(table->items[i].key);

                if (table->free_value != NULL)
                {
                    table->free_value(table->items[i].value);
                }
            }
        }
        free(table->items);
        free(table);
    }
}
