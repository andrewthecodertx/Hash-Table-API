#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t (*hash_function)(const void *key);
typedef bool (*key_equal_function)(const void *key1, const void *key2);
typedef void *(*copy_function)(const void *original);
typedef void (*destroy_function)(void *data);
typedef void *(*alloc_function)(size_t size);
typedef void (*free_function)(void *ptr);
typedef struct {
  copy_function copy;
  destroy_function destroy;
  key_equal_function equal; // Only used for keys
  hash_function hash;       // Only used for keys
} type_handler;

typedef struct {
  alloc_function alloc;
  free_function free;
} allocator;

typedef struct HashTable HashTable;

HashTable *hash_table_create(type_handler key_handler,
                             type_handler value_handler,
                             allocator *custom_allocator);

void hash_table_destroy(HashTable *table);
bool hash_table_insert(HashTable *table, void *key, void *value);
void *hash_table_lookup(const HashTable *table, const void *key);
bool hash_table_delete(HashTable *table, const void *key);
size_t hash_table_count(const HashTable *table);

#endif
