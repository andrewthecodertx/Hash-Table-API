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

// Represents the state of a single entry in the hash table using 2 bits.
// 00: STATE_EMPTY
// 01: STATE_OCCUPIED
// 10: STATE_DELETED
#define STATE_EMPTY    0b00
#define STATE_OCCUPIED 0b01
#define STATE_DELETED  0b10

// The internal representation of a single hash table entry.
// Note: The state is stored separately in the `control_bytes` array.
struct InternalEntry {
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
    uint8_t*       control_bytes; // Stores the state of each slot (2 bits per slot)
    struct InternalEntry *entries;
};

// --- Helper Functions ---

// Default allocator if none is provided.
static void* default_alloc(size_t size) { return malloc(size); }
static void  default_free(void* ptr) { free(ptr); }

// Forward declaration for resize function.
static bool resize(HashTable* table, size_t new_capacity);

// Get the state of the slot at a given index.
static uint8_t get_state(const HashTable* table, size_t index) {
    size_t byte_index = index / 4;
    size_t bit_offset = (index % 4) * 2;
    return (table->control_bytes[byte_index] >> bit_offset) & 0b11;
}

// Set the state of the slot at a given index.
static void set_state(HashTable* table, size_t index, uint8_t state) {
    size_t byte_index = index / 4;
    size_t bit_offset = (index % 4) * 2;
    // Clear the 2 bits for the current slot
    table->control_bytes[byte_index] &= ~((uint8_t)0b11 << bit_offset);
    // Set the new state
    table->control_bytes[byte_index] |= (state & 0b11) << bit_offset;
}


// Helper to find an entry for a given key.
// Returns the index of the entry.
static size_t find_entry_index(const HashTable* table, const void* key, bool find_empty_for_insert) {
    uint64_t hash = table->key_handler.hash(key);
    size_t index = hash % table->capacity;
    size_t tombstone_index = (size_t)-1; // Use -1 as an invalid index

    // Linear probing loop.
    for (;;) {
        uint8_t state = get_state(table, index);
        switch (state) {
            case STATE_EMPTY:
                // If we are inserting, we can use a tombstone if we found one.
                return find_empty_for_insert && tombstone_index != (size_t)-1 ? tombstone_index : index;
            case STATE_DELETED:
                if (find_empty_for_insert && tombstone_index == (size_t)-1) {
                    // Found a reusable slot, but keep searching for the key itself.
                    tombstone_index = index;
                }
                break;
            case STATE_OCCUPIED:
                if (table->key_handler.equal(table->entries[index].key, key)) {
                    return index; // Found the key.
                }
                break;
        }
        index = (index + 1) % table->capacity;
    }
}

// Resize the hash table to a new capacity.
static bool resize(HashTable* table, size_t new_capacity) {
    struct InternalEntry* old_entries = table->entries;
    uint8_t* old_control_bytes = table->control_bytes;
    size_t old_capacity = table->capacity;

    // Allocate new memory
    size_t control_size = (new_capacity + 3) / 4; // +3 to round up integer division
    uint8_t* new_control_bytes = table->alloc_handler.alloc(control_size);
    if (!new_control_bytes) return false;
    memset(new_control_bytes, 0, control_size); // Initialize all states to EMPTY

    struct InternalEntry* new_entries = table->alloc_handler.alloc(sizeof(struct InternalEntry) * new_capacity);
    if (!new_entries) {
        table->alloc_handler.free(new_control_bytes);
        return false;
    }

    // Initialize new entries to a known state
    for (size_t i = 0; i < new_capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = NULL;
    }

    // Temporarily update the table to point to the new storage.
    table->entries = new_entries;
    table->control_bytes = new_control_bytes;
    table->capacity = new_capacity;
    table->count = 0;

    // Re-hash all existing entries from the old storage into the new one.
    for (size_t i = 0; i < old_capacity; i++) {
        if (get_state(&(const HashTable){.control_bytes=old_control_bytes}, i) == STATE_OCCUPIED) {
            // This is a "move" not a "copy". We insert the key/value pointers directly,
            // and hash_table_insert will re-hash and copy them into the new array.
            hash_table_insert(table, old_entries[i].key, old_entries[i].value);
            
            // Since insert copies the data, we must free the original K/V from the old table.
            table->key_handler.destroy(old_entries[i].key);
            table->value_handler.destroy(old_entries[i].value);
        }
    }

    table->alloc_handler.free(old_entries);
    table->alloc_handler.free(old_control_bytes);
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

    // Allocate control bytes. Each byte holds states for 4 slots.
    size_t control_size = (table->capacity + 3) / 4; // +3 to round up integer division
    table->control_bytes = alloc_h.alloc(control_size);
    if (!table->control_bytes) {
        alloc_h.free(table);
        return NULL;
    }
    memset(table->control_bytes, 0, control_size); // All slots are STATE_EMPTY

    table->entries = alloc_h.alloc(sizeof(struct InternalEntry) * table->capacity);
    if (!table->entries) {
        alloc_h.free(table->control_bytes);
        alloc_h.free(table);
        return NULL;
    }
    
    // No need to initialize each entry's state as it's in control_bytes.
    // But we should null out key/value for safety.
    for (size_t i = 0; i < table->capacity; i++) {
        table->entries[i].key = NULL;
        table->entries[i].value = NULL;
    }

    return table;
}

void hash_table_destroy(HashTable* table) {
    if (!table) return;
    for (size_t i = 0; i < table->capacity; i++) {
        if (get_state(table, i) == STATE_OCCUPIED) {
            table->key_handler.destroy(table->entries[i].key);
            table->value_handler.destroy(table->entries[i].value);
        }
    }
    table->alloc_handler.free(table->control_bytes);
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

    size_t index = find_entry_index(table, key, true);
    struct InternalEntry* entry = &table->entries[index];
    
    bool is_new_entry = (get_state(table, index) != STATE_OCCUPIED);

    if (is_new_entry) {
        // This is a new key.
        set_state(table, index, STATE_OCCUPIED);
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
    size_t index = find_entry_index(table, key, false);
    if (get_state(table, index) == STATE_OCCUPIED) {
        return table->entries[index].value;
    }
    return NULL;
}

bool hash_table_delete(HashTable* table, const void* key) {
    if (table->count == 0) return false;
    size_t index = find_entry_index(table, key, false);

    if (get_state(table, index) != STATE_OCCUPIED) {
        return false; // Key not found.
    }

    // Free the key and value and mark the entry as deleted (tombstone).
    struct InternalEntry* entry = &table->entries[index];
    table->key_handler.destroy(entry->key);
    table->value_handler.destroy(entry->value);
    set_state(table, index, STATE_DELETED);
    entry->key = NULL;
    entry->value = NULL;
    table->count--;

    return true;
}

size_t hash_table_count(const HashTable* table) {
    return table->count;
}
