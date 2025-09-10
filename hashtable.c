#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR 0.75
#define STATE_EMPTY 0b00
#define STATE_OCCUPIED 0b01
#define STATE_DELETED 0b10

struct InternalEntry {
  void *key;
  void *value;
};

struct HashTable {
  type_handler key_handler;
  type_handler value_handler;
  allocator alloc_handler;
  size_t capacity;
  size_t count;
  uint8_t *control_bytes;
  struct InternalEntry *entries;
};

static void *default_alloc(size_t size) { return malloc(size); }
static void default_free(void *ptr) { free(ptr); }
static bool resize(HashTable *table, size_t new_capacity);

static uint8_t get_state(const HashTable *table, size_t index) {
  size_t byte_index = index / 4;
  size_t bit_offset = (index % 4) * 2;
  return (table->control_bytes[byte_index] >> bit_offset) & 0b11;
}

static void set_state(HashTable *table, size_t index, uint8_t state) {
  size_t byte_index = index / 4;
  size_t bit_offset = (index % 4) * 2;
  table->control_bytes[byte_index] &= ~((uint8_t)0b11 << bit_offset);
  table->control_bytes[byte_index] |= (state & 0b11) << bit_offset;
}

static size_t find_entry_index(const HashTable *table, const void *key,
                               bool find_empty_for_insert) {
  uint64_t hash = table->key_handler.hash(key);
  size_t index = hash % table->capacity;
  size_t tombstone_index = (size_t)-1;

  for (;;) {
    uint8_t state = get_state(table, index);
    switch (state) {
    case STATE_EMPTY:
      return find_empty_for_insert && tombstone_index != (size_t)-1
                 ? tombstone_index
                 : index;
    case STATE_DELETED:
      if (find_empty_for_insert && tombstone_index == (size_t)-1) {
        tombstone_index = index;
      }
      break;
    case STATE_OCCUPIED:
      if (table->key_handler.equal(table->entries[index].key, key)) {
        return index;
      }
      break;
    }
    index = (index + 1) % table->capacity;
  }
}

static bool resize(HashTable *table, size_t new_capacity) {
  struct InternalEntry *old_entries = table->entries;
  uint8_t *old_control_bytes = table->control_bytes;
  size_t old_capacity = table->capacity;

  size_t control_size = (new_capacity + 3) / 4;
  uint8_t *new_control_bytes = table->alloc_handler.alloc(control_size);
  if (!new_control_bytes)
    return false;
  memset(new_control_bytes, 0, control_size);

  struct InternalEntry *new_entries =
      table->alloc_handler.alloc(sizeof(struct InternalEntry) * new_capacity);
  if (!new_entries) {
    table->alloc_handler.free(new_control_bytes);
    return false;
  }

  for (size_t i = 0; i < new_capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = NULL;
  }

  table->entries = new_entries;
  table->control_bytes = new_control_bytes;
  table->capacity = new_capacity;
  table->count = 0;

  for (size_t i = 0; i < old_capacity; i++) {
    if (get_state(&(const HashTable){.control_bytes = old_control_bytes}, i) ==
        STATE_OCCUPIED) {
      hash_table_insert(table, old_entries[i].key, old_entries[i].value);

      table->key_handler.destroy(old_entries[i].key);
      table->value_handler.destroy(old_entries[i].value);
    }
  }

  table->alloc_handler.free(old_entries);
  table->alloc_handler.free(old_control_bytes);
  return true;
}

HashTable *hash_table_create(type_handler key_handler,
                             type_handler value_handler,
                             allocator *custom_allocator) {
  allocator alloc_h = {
      custom_allocator ? custom_allocator->alloc : default_alloc,
      custom_allocator ? custom_allocator->free : default_free};

  HashTable *table = alloc_h.alloc(sizeof(HashTable));
  if (!table)
    return NULL;

  table->key_handler = key_handler;
  table->value_handler = value_handler;
  table->alloc_handler = alloc_h;
  table->capacity = INITIAL_CAPACITY;
  table->count = 0;

  size_t control_size =
      (table->capacity + 3) / 4; // +3 to round up integer division
  table->control_bytes = alloc_h.alloc(control_size);
  if (!table->control_bytes) {
    alloc_h.free(table);
    return NULL;
  }
  memset(table->control_bytes, 0, control_size); // All slots are STATE_EMPTY

  table->entries =
      alloc_h.alloc(sizeof(struct InternalEntry) * table->capacity);
  if (!table->entries) {
    alloc_h.free(table->control_bytes);
    alloc_h.free(table);
    return NULL;
  }

  for (size_t i = 0; i < table->capacity; i++) {
    table->entries[i].key = NULL;
    table->entries[i].value = NULL;
  }

  return table;
}

void hash_table_destroy(HashTable *table) {
  if (!table)
    return;
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

bool hash_table_insert(HashTable *table, void *key, void *value) {
  if (table->count + 1 > table->capacity * MAX_LOAD_FACTOR) {
    if (!resize(table, table->capacity * 2)) {
      return false;
    }
  }

  size_t index = find_entry_index(table, key, true);
  struct InternalEntry *entry = &table->entries[index];

  bool is_new_entry = (get_state(table, index) != STATE_OCCUPIED);

  if (is_new_entry) {
    set_state(table, index, STATE_OCCUPIED);
    entry->key = table->key_handler.copy(key);
    entry->value = table->value_handler.copy(value);
    table->count++;
  } else {
    table->value_handler.destroy(entry->value);
    entry->value = table->value_handler.copy(value);
  }

  return true;
}

void *hash_table_lookup(const HashTable *table, const void *key) {
  if (table->count == 0)
    return NULL;
  size_t index = find_entry_index(table, key, false);
  if (get_state(table, index) == STATE_OCCUPIED) {
    return table->entries[index].value;
  }
  return NULL;
}

bool hash_table_delete(HashTable *table, const void *key) {
  if (table->count == 0)
    return false;
  size_t index = find_entry_index(table, key, false);

  if (get_state(table, index) != STATE_OCCUPIED) {
    return false;
  }

  struct InternalEntry *entry = &table->entries[index];
  table->key_handler.destroy(entry->key);
  table->value_handler.destroy(entry->value);
  set_state(table, index, STATE_DELETED);
  entry->key = NULL;
  entry->value = NULL;
  table->count--;

  return true;
}

size_t hash_table_count(const HashTable *table) { return table->count; }
