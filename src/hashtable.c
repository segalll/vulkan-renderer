#include "hashtable.h"

unsigned long hash_function(char* str, int size) {
    unsigned long i = 0;
    for (int j = 0; str[j]; j++) {
        i += str[j];
    }
    return i % size;
}

static LinkedList* allocate_list() {
    LinkedList* list = calloc(1, sizeof(LinkedList));
    return list;
}

static LinkedList* linkedlist_insert(LinkedList* list, ht_item* item) {
    if (!list) {
        LinkedList* head = allocate_list();
        head->item = item;
        head->next = NULL;
        list = head;
        return list;
    } else if (list->next == NULL) {
        LinkedList* node = allocate_list();
        node->item = item;
        node->next = NULL;
        list->next = node;
        return list;
    }

    LinkedList* temp = list;
    while (temp->next) {
        temp = temp->next;
    }

    LinkedList* node = allocate_list();
    node->item = item;
    node->next = NULL;
    temp->next = node;

    return list;
}

static void free_linkedlist(LinkedList* list) {
    LinkedList* temp = list;
    if (!list) {
        return;
    }
    while (list) {
        temp = list;
        list = list->next;
        free(temp->item->key);
        free(temp->item);
        free(temp);
    }
}

static LinkedList** create_overflow_buckets(HashTable* table) {
    LinkedList** buckets = calloc(table->size, sizeof(LinkedList*));
    for (int i = 0; i < table->size; i++)
        buckets[i] = NULL;
    return buckets;
}

static void free_overflow_buckets(HashTable* table) {
    LinkedList** buckets = table->overflow_buckets;
    for (int i = 0; i < table->size; i++)
        free_linkedlist(buckets[i]);
    free(buckets);
}


ht_item* create_item(char* key, int value) {
    ht_item* item = malloc(sizeof(ht_item));
    item->key = calloc(strlen(key) + 1, sizeof(char));

    strcpy(item->key, key);
    item->value = value;

    return item;
}

HashTable* create_table(int size) {
    HashTable* table = malloc(sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->items = calloc(table->size, sizeof(ht_item*));
    for (int i = 0; i < table->size; i++)
        table->items[i] = NULL;
    table->overflow_buckets = create_overflow_buckets(table);
    return table;
}

void free_item(ht_item* item) {
    free(item->key);
    free(item);
}

void free_hashtable(HashTable* table) {
    for (int i = 0; i < table->size; i++) {
        ht_item* item = table->items[i];
        if (item != NULL)
            free_item(item);
    }

    free_overflow_buckets(table);
    free(table->items);
    free(table);
}

void handle_collision(HashTable* table, unsigned long index, ht_item* item) {
    LinkedList* head = table->overflow_buckets[index];

    if (head == NULL) {
        head = allocate_list();
        head->item = item;
        table->overflow_buckets[index] = head;
        return;
    } else {
        table->overflow_buckets[index] = linkedlist_insert(head, item);
        return;
    }
}

void ht_insert(HashTable* table, char* key, int value) {
    ht_item* item = create_item(key, value);

    unsigned long index = hash_function(key, table->size);

    ht_item* current_item = table->items[index];

    if (current_item == NULL) {
        if (table->count == table->size) {
            printf("Insert Error: Hash Table is full\n");
            free_item(item);
            return;
        }

        table->items[index] = item;
        table->count++;
    } else {
        if (strcmp(current_item->key, key) == 0) {
            table->items[index]->value = value;
            free_item(item);
            return;
        } else {
            handle_collision(table, index, item);
            return;
        }
    }
}

int ht_search(HashTable* table, char* key) {
    unsigned long index = hash_function(key, table->size);
    ht_item* item = table->items[index];
    LinkedList* head = table->overflow_buckets[index];

    while (item != NULL) {
        if (strcmp(item->key, key) == 0)
            return item->value;
        if (head == NULL)
            return -1;
        item = head->item;
        head = head->next;
    }
    return -1;
}
