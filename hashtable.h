/*
 * -----------------------------------------------------------------------------
 * hashtable.h - Header File
 * -----------------------------------------------------------------------------
 * This file defines the public interface for a generic, open-addressing
 * hash table in C. It is designed for performance and flexibility, using
 * function pointers to support arbitrary key/value types and custom allocators.
 */

#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// --- Public API Structs ---

// A function pointer type for hashing a key.
typedef uint64_t (*hash_function)(const void* key);

// A function pointer type for comparing two keys.
typedef bool (*key_equal_function)(const void* key1, const void* key2);

// A function pointer type for copying a key or value.
typedef void* (*copy_function)(const void* original);

// A function pointer type for destroying/freeing a key or value.
typedef void (*destroy_function)(void* data);

// A function pointer type for a custom memory allocator.
typedef void* (*alloc_function)(size_t size);

// A function pointer type for a custom memory deallocator.
typedef void (*free_function)(void* ptr);

// Defines the set of functions required for managing a key or value type.
typedef struct {
    copy_function    copy;
    destroy_function destroy;
    key_equal_function equal; // Only used for keys
    hash_function    hash;    // Only used for keys
} type_handler;

// Defines the custom allocator functions.
typedef struct {
    alloc_function alloc;
    free_function  free;
} allocator;

// The main hash table structure.
// This is an opaque type; its internal layout is defined in the .c file.
typedef struct HashTable HashTable;

// --- Public API Functions ---

/**
 * @brief Creates a new hash table.
 *
 * @param key_handler A struct of function pointers for managing keys.
 * @param value_handler A struct of function pointers for managing values.
 * @param custom_allocator A struct with custom alloc/free functions. Can be NULL to use standard malloc/free.
 * @return A pointer to the newly created HashTable, or NULL on failure.
 */
HashTable* hash_table_create(
    type_handler key_handler,
    type_handler value_handler,
    allocator* custom_allocator
);

/**
 * @brief Destroys a hash table and frees all associated memory.
 *
 * @param table The hash table to destroy.
 */
void hash_table_destroy(HashTable* table);

/**
 * @brief Inserts a key-value pair into the hash table.
 *
 * If a key already exists, its value is updated. The table takes ownership
 * of the provided key and value, so the caller should not free them.
 *
 * @param table The hash table.
 * @param key A pointer to the key.
 * @param value A pointer to the value.
 * @return true on success, false on failure (e.g., memory allocation error).
 */
bool hash_table_insert(HashTable* table, void* key, void* value);

/**
 * @brief Looks up a value by its key in the hash table.
 *
 * @param table The hash table.
 * @param key A pointer to the key to look up.
 * @return A pointer to the value if the key is found, otherwise NULL.
 * The returned pointer is owned by the table; do not free it.
 */
void* hash_table_lookup(const HashTable* table, const void* key);

/**
 * @brief Deletes a key-value pair from the hash table.
 *
 * @param table The hash table.
 * @param key A pointer to the key to delete.
 * @return true if the key was found and deleted, otherwise false.
 */
bool hash_table_delete(HashTable* table, const void* key);

/**
 * @brief Returns the number of items currently in the hash table.
 *
 * @param table The hash table.
 * @return The number of items.
 */
size_t hash_table_count(const HashTable* table);

#endif // CUSTOM_HASH_TABLE_H
