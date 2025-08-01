# C Generic Hash Table

A generic, open-addressing hash table implementation in C designed for performance, low memory overhead, and flexibility.

## The Challenge

This project was created to address the "challenge" that C is a "terrible language" for creating efficient, reusable, and memory-optimized data structures compared to C++. The argument was that C lacks templates, modern allocators, and other abstractions that make this easy in C++.

This implementation aims to demonstrate that while C requires a different approach, it is more than capable of producing a high-quality, generic hash table that meets these goals.

## Features

- **Generic Key/Value Types**: Supports any data type for keys and values by using `void*` and user-provided `type_handler` functions for hashing, comparison, copying, and destruction.
- **Open Addressing**: Uses linear probing to resolve collisions, which is cache-friendly and avoids the overhead of linked list nodes used in chaining.
- **Single Allocation for Entries**: The main table storage is a single contiguous block of memory, reducing allocation overhead and improving data locality.
- **Custom Allocator Support**: Allows the user to provide their own memory allocation functions (`malloc`/`free` style) for integration with custom memory management schemes.
- **Low Memory Overhead**: Uses a `control_bytes` array to store the state of each slot (Empty, Occupied, Deleted) using only **2 bits per entry**. This is significantly more memory-efficient than storing a full byte or more for metadata per entry.
- **Drop-in Style API**: The API is designed to be straightforward and easy to integrate into existing C projects.

## How It Works

The core of the implementation is the `HashTable` struct, which is an opaque type to the user. It stores:
- Pointers to user-defined `type_handler` functions.
- Pointers to custom `allocator` functions (or defaults to `malloc`/`free`).
- The capacity, count, a pointer to the array of `InternalEntry` structs, and a pointer to the `control_bytes` array.

### 2-Bit Bookkeeping

To minimize memory overhead, the state of each slot is stored in a separate `control_bytes` array. Each byte in this array holds the state for four different slots, with each state encoded as follows:
- `00` (STATE_EMPTY): The slot has never been used.
- `01` (STATE_OCCUPIED): The slot holds a valid key-value pair.
- `10` (STATE_DELETED): The slot previously held data but was deleted (a "tombstone").

This approach reduces the bookkeeping cost to just 2 bits per entry, directly addressing one of the key memory optimization challenges.

### Type Handlers

To achieve genericity, the user must provide a `type_handler` struct, which contains function pointers for:
- `hash`: To compute a `uint64_t` hash from a key.
- `equal`: To check if two keys are equal.
- `copy`: To create a deep copy of a key or value.
- `destroy`: To free the memory of a key or value.

This approach gives the user full control over how their data types are managed, which is essential in a language without constructors/destructors.

### Memory Management

The hash table can be configured to use a custom allocator. This is useful in scenarios where you want to use a memory pool, a slab allocator, or another specialized memory management strategy to improve performance.

If no custom allocator is provided, it defaults to the standard library's `malloc` and `free`.

## Building and Running the Demo

A `Makefile` is provided to build the example program.

```bash
# Build the demo
make

# Run the demo
./hashtable_demo

# Clean up build files
make clean
```

## Conclusion

While C++ provides powerful, high-level abstractions that make generic programming very convenient, this project demonstrates that C is perfectly capable of creating efficient, reusable, and memory-conscious data structures. The trade-off is that C requires more explicit, manual management of types and memory, but this also gives the programmer a great deal of control.
