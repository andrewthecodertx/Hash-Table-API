# C Generic Hash Table

A generic, open-addressing hash table in C. Stores any key and value type — you provide the type handlers, it handles the rest.

## Why use this

Most C hash table libraries lock you into one key type (usually strings). This one doesn't. You define how your keys are hashed, compared, copied, and freed — so you can use composite keys, struct keys, or anything else C lets you point at.

The 2-bit control scheme (empty/occupied/deleted) means just 2 bits of overhead per slot instead of a full byte or a separate flags array. Open addressing keeps everything in one contiguous block of memory — better cache locality than chained tables.

Custom allocators let you plug in arena allocators, pool allocators, or whatever your project needs. If you don't care, pass `NULL` and it uses `malloc`/`free`.

## API

```c
HashTable *hash_table_create(type_handler key_handler, type_handler value_handler, allocator *custom_allocator);
void       hash_table_destroy(HashTable *table);
bool       hash_table_insert(HashTable *table, void *key, void *value);
void      *hash_table_lookup(const HashTable *table, const void *key);
bool       hash_table_delete(HashTable *table, const void *key);
size_t     hash_table_count(const HashTable *table);
```

`insert` returns `true` on success. Inserting a key that already exists replaces the value (and frees the old one). `lookup` returns a pointer to the value, or `NULL` if the key isn't found.

## Quick example — string keys, int values

```c
#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- key handlers (C strings) --

uint64_t hash_string(const void *key) {
    uint64_t h = 5381;
    for (const char *p = (const char *)key; *p; p++)
        h = h * 33 ^ *p;
    return h;
}

bool eq_string(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b) == 0;
}

void *copy_string(const void *src) {
    return strdup((const char *)src);
}

void free_string(void *p) { free(p); }

// -- value handlers (heap-allocated ints) --

void *copy_int(const void *src) {
    int *copy = malloc(sizeof(int));
    if (copy) *copy = *(const int *)src;
    return copy;
}

void free_int(void *p) { free(p); }

int main(void) {
    type_handler key_h   = { .copy = copy_string, .destroy = free_string,
                             .equal = eq_string, .hash = hash_string };
    type_handler value_h = { .copy = copy_int,    .destroy = free_int };

    HashTable *ht = hash_table_create(key_h, value_h, NULL);

    // insert
    int age = 30;
    hash_table_insert(ht, "alice", &age);
    age = 25;
    hash_table_insert(ht, "bob", &age);

    // lookup
    int *result = hash_table_lookup(ht, "alice");
    if (result) printf("alice is %d\n", *result);  // alice is 30

    // update (inserting existing key replaces value)
    age = 31;
    hash_table_insert(ht, "alice", &age);
    result = hash_table_lookup(ht, "alice");
    if (result) printf("alice is now %d\n", *result);  // alice is now 31

    // delete
    hash_table_delete(ht, "bob");
    printf("count: %zu\n", hash_table_count(ht));  // count: 1

    hash_table_destroy(ht);
    return 0;
}
```

## Struct keys — composite lookups

The real point of this library is using something other than a string as a key. Here's a 2-field struct key:

```c
typedef struct { int id; char name[32]; } user_key;
typedef struct { double score; } user_value;

uint64_t hash_user_key(const void *key) {
    const user_key *uk = key;
    uint64_t h = 5381;
    h = h * 33 ^ uk->id;
    for (const char *p = uk->name; *p; p++)
        h = h * 33 ^ *p;
    return h;
}

bool eq_user_key(const void *a, const void *b) {
    const user_key *ka = a, *kb = b;
    return ka->id == kb->id && strcmp(ka->name, kb->name) == 0;
}

void *copy_user_key(const void *src) {
    user_key *copy = malloc(sizeof(user_key));
    if (copy) memcpy(copy, src, sizeof(user_key));
    return copy;
}

void *copy_user_value(const void *src) {
    user_value *copy = malloc(sizeof(user_value));
    if (copy) memcpy(copy, src, sizeof(user_value));
    return copy;
}

void destroy_data(void *p) { free(p); }

int main(void) {
    type_handler key_h   = { .copy = copy_user_key,   .destroy = destroy_data,
                             .equal = eq_user_key,    .hash = hash_user_key };
    type_handler value_h = { .copy = copy_user_value, .destroy = destroy_data };

    HashTable *ht = hash_table_create(key_h, value_h, NULL);

    user_key k = { .id = 42 };
    strncpy(k.name, "alice", sizeof(k.name));
    user_value v = { .score = 97.5 };
    hash_table_insert(ht, &k, &v);

    user_key lookup = { .id = 42 };
    strncpy(lookup.name, "alice", sizeof(lookup.name));
    user_value *found = hash_table_lookup(ht, &lookup);
    if (found) printf("score: %.1f\n", found->score);  // score: 97.5

    hash_table_destroy(ht);
    return 0;
}
```

## Custom allocator

Pass an `allocator` to `hash_table_create` if you want control over memory:

```c
allocator my_alloc = { .alloc = my_alloc_fn, .free = my_free_fn };
HashTable *ht = hash_table_create(key_h, value_h, &my_alloc);
```

Pass `NULL` to use the default (`malloc`/`free`).

## How it works

- **Open addressing** with linear probing — colliding entries go in the next slot
- **2-bit control bytes** — each slot is marked empty (`00`), occupied (`01`), or deleted/tombstone (`10`). That's 2 bits per entry instead of a full byte
- **Auto-resize** — doubles capacity when load factor exceeds 0.75, rehashes all live entries into the new table
- **Tombstone reuse** — deleted slots are marked for reuse; inserts can fill them, lookups skip over them

## Building and running

```bash
make            # build the demo
./hashtable_demo
make clean
```

The demo in `main.c` exercises insert, lookup, update, and delete with struct keys.

## License

MIT, see [LICENSE](LICENSE).