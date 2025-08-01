/*
 * -----------------------------------------------------------------------------
 * hashtable.c - Implementation File
 * -----------------------------------------------------------------------------
 * This file contains the implementation of the generic hash table.
 * It uses a single memory allocation for all entries to improve cache
 * performance and reduce allocation overhead.
 */

#include "hashtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Internal Constants and Structs ---

#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR 0.75

// Represents the state of a single entry in the hash table.
typedef enum {
    STATE_EMPTY,    // The slot has never been used.
    STATE_OCCUPIED, // The slot holds a valid key-value pair.
    STATE_DELETED   // The slot previously held data but was deleted.
} EntryState;

// The internal representation of a single hash table entry.
struct InternalEntry {
    EntryState state;
    void* key;
    void* value;
};

// The internal structure of the hash table, hiding implementation details.
struct HashTable {
    type_handler   key_handler;
    type_handler   value_handler;
    allocator      alloc_handler;
    size_t         capacity;
    size_t         count;
    struct InternalEntry *entries;
};

// --- Helper Functions ---

// Default allocator if none is provided.
static void* default_alloc(size_t size) { return malloc(size); }
static void  default_free(void* ptr) { free(ptr); }

// Forward declaration for resize function.
static bool resize(HashTable* table, size_t new_capacity);

// Helper to find an entry for a given key.
// Returns a pointer to the entry, which might be empty or a tombstone.
static struct InternalEntry* find_entry(const HashTable* table, const void* key, bool find_empty_for_insert) {
    uint64_t hash = table->key_handler.hash(key);
    size_t index = hash % table->capacity;
    struct InternalEntry* tombstone = NULL;

    // Linear probing loop.
    for (;;) {
        struct InternalEntry* entry = &table->entries[index];
        switch (entry->state) {
            case STATE_EMPTY:
                // If we are inserting, we can use a tombstone if we found one.
                return find_empty_for_insert && tombstone ? tombstone : entry;
            case STATE_DELETED:
                if (!tombstone) {
                    // Found a reusable slot, but keep searching for the key itself.
                    tombstone = entry;
                }
                break;
            case STATE_OCCUPIED:
                if (table->key_handler.equal(entry->key, key)) {
                    return entry; // Found the key.
                }
                break;
        }
        index = (index + 1) % table->capacity;
    }
}

// Resize the hash table to a new capacity.
static bool resize(HashTable* table, size_t new_capacity) {
    struct InternalEntry* old_entries = table->entries;
    size_t old_capacity = table->capacity;

    struct InternalEntry* new_entries = table->alloc_handler.alloc(sizeof(struct InternalEntry) * new_capacity);
    if (!new_entries) return false;

    for (size_t i = 0; i < new_capacity; i++) {
        new_entries[i].state = STATE_EMPTY;
        new_entries[i].key = NULL;
        new_entries[i].value = NULL;
    }

    // Temporarily update the table to point to the new storage.
    table->entries = new_entries;
    table->capacity = new_capacity;
    table->count = 0;

    // Re-hash all existing entries from the old storage into the new one.
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].state == STATE_OCCUPIED) {
            // We insert the key/value pointers directly.
            // hash_table_insert will re-hash and copy them into the new array.
            // Note: This is a "move" not a "copy", since the old entries will be freed.
            hash_table_insert(table, old_entries[i].key, old_entries[i].value);
            
            // Since insert copies the data, we must free the original K/V from the old table.
            table->key_handler.destroy(old_entries[i].key);
            table->value_handler.destroy(old_entries[i].value);
        }
    }

    table->alloc_handler.free(old_entries);
    return true;
}

// --- Public Function Implementations ---

HashTable* hash_table_create(type_handler key_handler, type_handler value_handler, allocator* custom_allocator) {
    allocator alloc_h = { custom_allocator ? custom_allocator->alloc : default_alloc,
                          custom_allocator ? custom_allocator->free : default_free };

    HashTable* table = alloc_h.alloc(sizeof(HashTable));
    if (!table) return NULL;

    table->key_handler = key_handler;
    table->value_handler = value_handler;
    table->alloc_handler = alloc_h;
    table->capacity = INITIAL_CAPACITY;
    table->count = 0;

    table->entries = alloc_h.alloc(sizeof(struct InternalEntry) * table->capacity);
    if (!table->entries) {
        alloc_h.free(table);
        return NULL;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        table->entries[i].state = STATE_EMPTY;
        table->entries[i].key = NULL;
        table->entries[i].value = NULL;
    }

    return table;
}

void hash_table_destroy(HashTable* table) {
    if (!table) return;
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->entries[i].state == STATE_OCCUPIED) {
            table->key_handler.destroy(table->entries[i].key);
            table->value_handler.destroy(table->entries[i].value);
        }
    }
    table->alloc_handler.free(table->entries);
    table->alloc_handler.free(table);
}

bool hash_table_insert(HashTable* table, void* key, void* value) {
    if (table->count + 1 > table->capacity * MAX_LOAD_FACTOR) {
        // Grow the table. The new capacity must be larger than the old one.
        if (!resize(table, table->capacity * 2)) {
            return false;
        }
    }

    struct InternalEntry* entry = find_entry(table, key, true);
    
    bool is_new_entry = (entry->state != STATE_OCCUPIED);

    if (is_new_entry) {
        // This is a new key.
        entry->state = STATE_OCCUPIED;
        entry->key = table->key_handler.copy(key);
        entry->value = table->value_handler.copy(value);
        table->count++;
    } else {
        // Key already exists, so update the value.
        // Destroy the old value before replacing it.
        table->value_handler.destroy(entry->value);
        entry->value = table->value_handler.copy(value);
    }

    return true;
}

void* hash_table_lookup(const HashTable* table, const void* key) {
    if (table->count == 0) return NULL;
    struct InternalEntry* entry = find_entry(table, key, false);
    if (entry->state == STATE_OCCUPIED) {
        return entry->value;
    }
    return NULL;
}

bool hash_table_delete(HashTable* table, const void* key) {
    if (table->count == 0) return false;
    struct InternalEntry* entry = find_entry(table, key, false);

    if (entry->state != STATE_OCCUPIED) {
        return false; // Key not found.
    }

    // Free the key and value and mark the entry as deleted (tombstone).
    table->key_handler.destroy(entry->key);
    table->value_handler.destroy(entry->value);
    entry->state = STATE_DELETED;
    entry->key = NULL;
    entry->value = NULL;
    table->count--;

    return true;
}

size_t hash_table_count(const HashTable* table) {
    return table->count;
}
