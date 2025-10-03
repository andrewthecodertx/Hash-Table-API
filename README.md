# C Generic Hash Table

A generic, open-addressing hash table implementation in C designed for performance, low memory overhead, and flexibility.

## The Goal

Have you ever been told that some programming languages are just "better" than others for certain jobs? This project started as a response to the idea that C isn't well-suited for building high-performance, reusable, and memory-efficient data structures compared to more modern languages like C++.

This hash table is designed to show that with a bit of cleverness, C can be used to create code that is not only fast and efficient but also flexible and easy to reuse.

## Features

- **Works with Any Data Type**: You can use this hash table to store any kind of data, from simple numbers to complex custom structures.
- **Fast and Efficient**: It uses a technique called "open addressing" to store data in a way that's quick to access and friendly to your computer's memory.
- **Smart Memory Use**: The hash table stores all its data in a single, organized block of memory, which reduces clutter and improves performance.
- **Use Your Own Memory Manager**: If you have a special way of managing memory in your project, you can plug it right in.
- **Extremely Low Memory Overhead**: It uses a clever trick to keep track of data using only 2 bits of memory per entry, saving a significant amount of space.
- **Easy to Use**: The API is designed to be simple and straightforward, so you can get up and running quickly.

## How It Works

The hash table is designed to be a black box for the user. You interact with it through a simple API, and it handles all the complex details internally.

### The Hotel Analogy: 2-Bit Bookkeeping

To save memory, we use a clever trick to keep track of the status of each slot in the hash table. Think of it like a hotel with a very efficient receptionist. Instead of using a big, clunky notebook to track room status, the receptionist uses a compact system where each room's status (Empty, Occupied, or Needs Cleaning) is represented by a tiny 2-bit code.

- `00` (Empty): The room is available.
- `01` (Occupied): The room has a guest.
- `10` (Needs Cleaning): The room was used but is now empty (this tells us we can clean it and make it available again).

This method is incredibly space-efficient, allowing us to manage the hash table with minimal memory overhead.

### Handling Different Data Types

To make the hash table work with any data type, you provide a set of simple instructions, called "type handlers." These handlers tell the hash table how to perform basic operations on your data, such as:
- **Hashing**: Creating a unique identifier for a key.
- **Comparing**: Checking if two keys are the same.
- **Copying**: Making a copy of a key or value.
- **Deleting**: Freeing up the memory used by a key or value.

This approach gives you full control over your data while keeping the hash table's design clean and flexible.

### Custom Memory Management

For projects with special memory requirements, you can provide your own memory management functions. This is like telling our hotel to use a specific supplier for its resources, giving you more control over how memory is allocated and freed. If you don't provide a custom memory manager, it will use the standard C library functions.

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

This project shows that C, while a classic language, is more than capable of creating high-quality, modern data structures. It may require a bit more hands-on work, but the result is a hash table that is fast, memory-efficient, and highly flexible, proving that good engineering can make all the difference.
