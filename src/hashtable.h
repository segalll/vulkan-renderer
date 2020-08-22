#ifndef hashtable_h
#define hashtable_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ht_item {
    char* key;
    int value;
};

struct LinkedList {
    ht_item* item;
    LinkedList* next;
};

struct HashTable {
    ht_item** items;
    LinkedList** overflow_buckets;
    int size;
    int count;
};

unsigned long hash_function(char* str, int size);
static LinkedList* allocate_list();
static LinkedList* linkedlist_insert(LinkedList* list, ht_item* item);
static ht_item* linkedlist_remove(LinkedList* list);
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