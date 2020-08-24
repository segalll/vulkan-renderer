#ifndef hashtable_h
#define hashtable_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* key;
    int value;
} ht_item;

typedef struct LinkedList {
    ht_item* item;
    struct LinkedList* next;
} LinkedList;

typedef struct {
    ht_item** items;
    LinkedList** overflow_buckets;
    int size;
    int count;
} HashTable;

unsigned long hash_function(char* str, int size);
static LinkedList* allocate_list(void);
static LinkedList* linkedlist_insert(LinkedList* list, ht_item* item);
static void free_linkedlist(LinkedList* list);
static LinkedList** create_overflow_buckets(HashTable* table);
static void free_overflow_buckets(HashTable* table);
ht_item* create_item(char* key, int value);
HashTable* create_table(int size);
void free_item(ht_item* item);
void free_hashtable(HashTable* table);
void handle_collision(HashTable* table, unsigned long index, ht_item* item);
void ht_insert(HashTable* table, char* key, int value);
int ht_search(HashTable* table, char* key);

#endif
