#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int id;
  char name[32];
} user_key;

typedef struct {
  double value;
  char metadata[64];
} user_value;

uint64_t hash_user_key(const void *key) {
  const user_key *uk = (const user_key *)key;
  uint64_t hash = 5381;
  hash = ((hash << 5) + hash) ^ uk->id;
  const char *p = uk->name;
  int c;
  while ((c = *p++)) {
    hash = ((hash << 5) + hash) ^ c;
  }
  return hash;
}

bool equal_user_key(const void *key1, const void *key2) {
  const user_key *uk1 = (const user_key *)key1;
  const user_key *uk2 = (const user_key *)key2;
  return (uk1->id == uk2->id) && (strcmp(uk1->name, uk2->name) == 0);
}

void *copy_user_key(const void *original) {
  user_key *new_key = malloc(sizeof(user_key));
  if (new_key) {
    memcpy(new_key, original, sizeof(user_key));
  }
  return new_key;
}

void *copy_user_value(const void *original) {
  user_value *new_value = malloc(sizeof(user_value));
  if (new_value) {
    memcpy(new_value, original, sizeof(user_value));
  }
  return new_value;
}

void destroy_user_data(void *data) { free(data); }

int main() {
  printf("--- C Generic Hash Table Demo ---\n\n");

  type_handler key_handler = {.copy = copy_user_key,
                              .destroy = destroy_user_data,
                              .equal = equal_user_key,
                              .hash = hash_user_key};

  type_handler value_handler = {.copy = copy_user_value,
                                .destroy = destroy_user_data,
                                .equal = NULL,
                                .hash = NULL};

  HashTable *my_table = hash_table_create(key_handler, value_handler, NULL);
  if (!my_table) {
    fprintf(stderr, "Failed to create hash table.\n");
    return 1;
  }

  printf("Table created. Initial count: %zu\n", hash_table_count(my_table));
  printf("\nInserting data...\n");

  user_key k1 = {101, "alpha"};
  user_value v1 = {3.14, "First item"};
  hash_table_insert(my_table, &k1, &v1);

  user_key k2 = {202, "beta"};
  user_value v2 = {2.71, "Second item"};
  hash_table_insert(my_table, &k2, &v2);

  user_key k3 = {303, "gamma"};
  user_value v3 = {1.61, "Third item"};
  hash_table_insert(my_table, &k3, &v3);

  printf("After insertions, count: %zu\n", hash_table_count(my_table));
  printf("\nLooking up data...\n");

  user_key lookup_key = {202, "beta"};
  user_value *found_value = hash_table_lookup(my_table, &lookup_key);

  if (found_value) {
    printf("Found key {202, 'beta'}. Value: {%.2f, '%s'}\n", found_value->value,
           found_value->metadata);
  } else {
    printf("Key {202, 'beta'} not found.\n");
  }

  user_key not_found_key = {999, "omega"};
  found_value = hash_table_lookup(my_table, &not_found_key);
  if (!found_value) {
    printf("Correctly did not find key {999, 'omega'}.\n");
  }

  printf("\nUpdating data...\n");
  user_key k1_update = {101, "alpha"};
  user_value v1_update = {9.81, "UPDATED first item"};
  hash_table_insert(my_table, &k1_update, &v1_update);

  user_value *updated_val = hash_table_lookup(my_table, &k1_update);
  if (updated_val) {
    printf("Looked up key {101, 'alpha'} again. New value: {%.2f, '%s'}\n",
           updated_val->value, updated_val->metadata);
  }
  printf("Count after update (should be unchanged): %zu\n",
         hash_table_count(my_table));
  printf("\nDeleting data...\n");

  user_key key_to_delete = {303, "gamma"};

  bool deleted = hash_table_delete(my_table, &key_to_delete);
  if (deleted) {
    printf("Successfully deleted key {303, 'gamma'}.\n");
  } else {
    printf("Failed to delete key {303, 'gamma'}.\n");
  }

  found_value = hash_table_lookup(my_table, &key_to_delete);
  if (!found_value) {
    printf("Correctly did not find deleted key.\n");
  }

  printf("Count after deletion: %zu\n", hash_table_count(my_table));
  printf("\nDestroying table.\n");
  hash_table_destroy(my_table);

  return 0;
}
