# C Generic Hash Table

This project is a C-based implementation of a generic hash table that uses open
addressing with linear probing to resolve collisions. It is designed to be
flexible and efficient, allowing for any data type to be used for keys and
values, and it supports custom memory allocators.

## Features

* **Type-Generic:** By using `void*` pointers and function handlers, the hash
table can store keys and values of any data type.
* **Open Addressing:** Resolves hash collisions using linear probing, which can
be efficient and cache-friendly.
* **Dynamic Resizing:** Automatically grows the internal storage when the load
factor exceeds a predefined threshold (`0.75`) to maintain performance.
* **Custom Memory Management:** Users can provide their own `malloc` and `free`
functions for custom memory allocation strategies.
* **Simple and Clean API:** The public interface, defined in `hashtable.h`, is
straightforward and easy to use.

## File Structure

* `hashtable.h`: The header file containing the public API for the hash table.
* `hashtable.c`: The implementation file for the hash table.
* `main.c`: An example program demonstrating how to use the hash table with a
custom `user_key` and `user_value` type.
* `Makefile`: A simple Makefile to build the project.

## How to Build and Run

To build the project, you can use the provided `Makefile`.

```bash
make
```

This will compile the source files and create an executable named `hashtable_demo`.

To run the demo program, execute the following command:

```bash
./hashtable_demo
```

The output will show the steps of creating a hash table, inserting, looking up,
updating, and deleting key-value pairs.

## API Overview

The main functions provided by the hash table are:

* `hash_table_create`: Creates a new hash table.
* `hash_table_destroy`: Frees all memory associated with the hash table.
* `hash_table_insert`: Inserts a new key-value pair or updates the value if the
key already exists.
* `hash_table_lookup`: Retrieves the value for a given key.
* `hash_table_delete`: Removes a key-value pair from the table.
* `hash_table_count`: Returns the number of items in the table.

To use the hash table, you need to provide `type_handler` structs that contain
function pointers for hashing, comparing, copying, and destroying your custom
key and value types.
