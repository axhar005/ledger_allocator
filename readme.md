# Arena Allocator
## Overview

Arena Allocator is a high-performance memory management library that utilizes arena-based allocation strategies to provide efficient and fast memory handling. This project is designed to optimize memory usage, reduce fragmentation, and simplify allocation and deallocation processes.
Features

    Efficient Memory Allocation: Quickly allocate and deallocate memory using an arena-based system.
    Fragmentation Reduction: Merge free memory blocks to minimize fragmentation.  
	Flexible Management: Support for multiple child arenas to handle overflow.  
	Simple API: Easy-to-use interface for integrating with existing projects.   
	Lightweight: Minimal overhead and fast operations.

## Installation

To use Arena Allocator in your project, simply clone this repository and include the source files in your build system.

bash

git clone https://github.com/axhar005/arena_allocator.git

Include the header file in your code:

```c
#include "arena.h"
```
Usage

Here is a simple example to demonstrate how to use the Arena Allocator:

```c
#include <stdio.h>
#include "arena.h"

int main() {
    // Create an arena with a size of 1 MB
    Arena *arena = arena_create(ARENA_SIZE);
    if (!arena)
    	return 1;

    // Allocate memory blocks
    void *ptr1 = arena_alloc(arena, 100);
    void *ptr2 = arena_alloc(arena, 200);

    // Use the allocated memory
    if (ptr1 && ptr2) {
        sprintf(ptr1, "Hello, Arena Allocator!");
        printf("%s\n", ptr1);
    }

    // Free the memory blocks
    arena_free(ptr1);
    arena_free(ptr2);

    // Merge free blocks to optimize memory
    arena_merge_free_blocks(arena);

    // Reset the arena
    arena_reset(arena);

    // Delete the arena when done
    arena_delete(arena);

    return 0;
}
```
## Function Reference
### Arena* arena_create(u64 size)

Creates a new arena with the specified size.
```
    size: The size of the arena in bytes.
    Returns: A pointer to the created arena.
```

### void* arena_alloc(Arena *arena, u64 size)

Allocates a block of memory of the specified size in the arena.
```
    arena: A pointer to the arena.
    size: The size of the block to allocate in bytes.
    Returns: A pointer to the allocated block.
```

### void arena_free(void *ptr)
```
Frees a previously allocated block of memory.

    ptr: A pointer to the block to free.
```

### void arena_reset(Arena *arena)
```
Resets the arena, freeing all allocations made.

    arena: A pointer to the arena.
```

### void arena_delete(Arena *arena)
```
Deletes an arena and frees all associated memory.

    arena: A pointer to the arena to delete.
```

### void arena_merge_free_blocks(Arena *arena)
```
Merges adjacent free blocks in the arena into a single larger block to reduce fragmentation.

    arena: A pointer to the arena.
```

## Contributing
Contributions are welcome! If you have suggestions, bug reports, or feature requests, please open an issue or submit a pull request.

    Fork the repository.
    Create a new branch (git checkout -b feature-branch).
    Make your changes and commit them (git commit -m 'Add new feature').
    Push to the branch (git push origin feature-branch).
    Open a pull request.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
Contact
