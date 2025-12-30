# Arena Allocator (Notebook Edition)

## Overview
Arena Allocator is a high-performance memory management library utilizing an arena-based allocation strategy with a unique Notebook architecture. 

Unlike traditional linear allocators, it manages memory in "pages" (child arenas). When a page is full, it automatically "turns the page" by creating a new, larger arena. It balances the extreme speed of an arena with the flexibility of a free-list, allowing for individual block deallocation, smart reallocation, and fragmentation reduction.



## Features
* The Notebook Strategy: Automatic chaining of child arenas to handle overflow seamlessly.
* Efficient Allocation: Quick O(1) allocation and deallocation within the current memory offset.
* Fragmentation Reduction: Automatically merges adjacent free blocks after reaching MAX_FREE_COUNT.
* SIMD Ready: Guaranteed 16-byte alignment for high-performance data structures.
* Smart Realloc: In-place block expansion when adjacent memory is free or at the arena boundary.
* Lightweight: Minimal memory overhead with a simple, clean C API.

## Memory Visualization

### 🏗️ Block Structure (The "Notebook" Entry)
Each allocation is composed of a metadata header followed by 16-byte aligned user data.
```

+-------------------+-------------------+-------------------+
|  Metadata (16B)   |   User Data (N)   |  Padding (Align)  |
| [Used|Cap|Size|L] |  "Hello World"    |  0x00 00 00 ...   |
+-------------------+-------------------+-------------------+
 ^                   ^
 |--- (Internal) ----|--- (User Pointer) -------------------|
```

### 📖 The Notebook Concept (Chaining)
When a "page" (arena) is full, a new, larger page is automatically linked.
```
  PAGE 1 (Current)          PAGE 2 (Child)            PAGE 3 (Child)
+-------------------+     +-------------------+     +-------------------+
| [Block 1]         |     | [Block 3]         |     |                   |
| [Block 2]         | --> | [Free Block]      | --> |  (Empty Space)    |
| [Remaining Space] |     | [Block 4]         |     |                   |
+-------------------+     +-------------------+     +-------------------+
          |                         |                         |
    Initial Size              Size x 2                  Size x 4
```

## Project Structure
```
.
├── include/
│   └── arena.h      # API Definitions & Metadata structures
├── src/
│   ├── arena.c      # Core logic (allocation, merging, chaining)
│   └── main.c       # Demonstration and testing suite
├── build.bat        # Windows Build Script (CMake/Ninja)
├── run.bat          # Windows Execution Script
├── Makefile         # Linux/macOS Build Script
└── CMakeLists.txt   # Cross-platform CMake configuration
```

## Installation
To use Arena Allocator in your project, clone the repository and include the source files:

git clone https://github.com/axhar005/arena_allocator.git

### Building on Windows
This project includes dedicated scripts for Windows users:
```
build.bat    :: Compiles the project using CMake
```
```
run.bat      :: Executes the compiled demo
```

### Building on Linux/macOS
```
make         # Compiles the library and main example
./arena      # Run the executable
```

## Usage
Include the header file in your code:
#include "arena.h"

Example:
---------------------------------------------------------
```
#include <stdio.h>
#include "include/arena.h"

int main() {
    // 1. Create a "Notebook" with an initial 1MB page
    Arena *nb = arena_create(ARENA_SIZE);
    if (!nb) return 1;

    // 2. Allocate blocks
    char *ptr1 = (char *)arena_alloc(nb, 100);
    char *ptr2 = (char *)arena_alloc(nb, 200);

    // 3. Use and Reallocate
    if (ptr1 && ptr2) {
        sprintf(ptr1, "Hello, Arena Allocator!");
        printf("%s\n", ptr1);
        
        // Grows the block if possible
        ptr1 = (char *)arena_realloc(nb, ptr1, 300);
    }

    // 4. Free specific blocks (marked for reuse)
    arena_free(nb, ptr1);
    arena_free(nb, ptr2);

    // 5. Inspect the Notebook (Page stats & Hex dumps)
    arena_print_child(nb, true);

    // 6. Delete the entire chain when done
    arena_delete(nb);

    return 0;
}
```
---------------------------------------------------------

## Function Reference

### Arena* arena_create(u64 size)
```
Creates a new arena (the first page of the notebook).
* size: The size of the arena in bytes.
* Returns: A pointer to the created arena.
```

### void* arena_alloc(Arena *arena, u64 size)
```
Allocates a block of memory. If the current page is full, it searches for free blocks or creates a larger child page.
* Returns: A pointer to the allocated block.
```

### void arena_free(Arena *arena, void *ptr)
```
Frees a previously allocated block. The block is cleared and marked as free for future reuse.
```

### void* arena_realloc(Arena *arena, void *ptr, u64 new_size)
```
Resizes a block. If the block is at the end of the offset or next to a free block, it expands on the spot. Otherwise, it moves the data to a new location.
```

### void arena_merge_free_blocks(Arena *arena)
```
Merges adjacent free blocks in the arena into a single larger block to reduce fragmentation.
```

### void arena_reset(Arena *arena)
```
Resets the entire chain of arenas, clearing all allocations while keeping the memory for the next use cycle.
```

### void arena_delete(Arena *arena)
```
Deletes the arena and all its child pages, freeing all associated memory from the system.
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