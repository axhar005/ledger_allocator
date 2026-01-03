# Ledger Allocator

## Overview

Ledger Allocator is a custom memory tracking and management system designed to give developers full control over their data's lifecycle. Instead of dealing with "blind" pointers, this system treats memory as a series of "Smart Entries" where metadata—such as capacity and current length—is stored directly alongside the raw data.

The Ledger Allocator is essentially a hybrid system: it is built upon the speed of an Arena Allocator, integrated with classic memory management functions (alloc, free, realloc), and features a Chaining system where each overflow creates a child Ledger that is always 2x larger than the previous one.

This approach eliminates the need for constant manual tracking or expensive operations like `strlen()`. By embedding the state within the allocation itself, the Ledger Allocator acts as a "Memory Diary": every entry knows its own limits, and every "Page" (Ledger) is linked to the next through an automated chaining system, ensuring you never lose grip on your program's heap.

## Memory Visualization

### 🏗️ Block Structure (The "Ledger" Entry)
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
When a "page" (Ledger) is full, a new, larger page is automatically linked.
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

## Features

* **Embedded Metadata Tracking:** Every block stores its own `capacity` and `length`. This allows you to retrieve data size or string length in O(1) time without extra calculations or external variables.
* **Automated Memory Chaining:** If a Ledger exceeds its initial size, the system automatically allocates and links a "child" Ledger. Your allocations continue seamlessly across a linked list of memory segments.
* **16-Byte Data Alignment:** Built-in alignment logic ensures all user data is optimized for modern CPU architectures and safe for all data types.
* **Smart Reallocation:** The `realloc` engine attempts to expand blocks in-place by checking the remaining ledger offset or merging with adjacent free blocks before deciding to move data.
* **Fragmentation Recovery:** Includes a block-merging (coalescence) algorithm that scans and fuses adjacent free fragments into larger reusable spaces after a set number of frees.
* **Zero-Fill Security:** All freed memory is automatically cleared (memset to zero), preventing data leakage and making memory debugging significantly easier.
* **Full Chain Introspection:** Built-in printing functions allow you to inspect every block, across every linked ledger, with hex-dump previews of the content.


## Project Structure
```
.
├── include/
│   └── ledger.h      # API Definitions & Metadata structures
├── src/
│   ├── ledger.c      # Core logic (allocation, merging, chaining)
│   └── main.c       # Demonstration and testing suite
├── build.bat        # Windows Build Script (CMake/Ninja)
├── run.bat          # Windows Execution Script
├── Makefile         # Linux/macOS Build Script
└── CMakeLists.txt   # Cross-platform CMake configuration
```

## Installation
To use Ledger Allocator in your project, clone the repository and include the source files:

git clone https://github.com/axhar005/ledger_allocator.git

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
./ledger      # Run the executable
```

## Usage
Include the header file in your code:
#include "ledger.h"

Example:
---------------------------------------------------------
```c
#include <stdio.h>
#include <string.h>
#include "../include/ledger.h"

int main() {
    printf("=== Ledger Allocator: Notebook Edition Demo ===\n\n");

    // 1. Initialize the Notebook with a small page (1KB)
    // We use a small size to easily demonstrate the "Page Turning" (Child creation)
    Ledger *nb = ledger_create(1024); 
    if (!nb) {
        fprintf(stderr, "Failed to initialize ledger.\n");
        return 1;
    }

    // 2. Basic Allocation & Data Manipulation
    printf("[Step 1] Basic Allocation\n");
    char *entry1 = (char *)ledger_alloc(nb, 128);
    if (entry1) {
        strcpy(entry1, "Learning C at 42 is challenging but fun!");
        
        // Use helper functions to manage the "Notebook" entry
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        
        printf("  Data: %s\n", entry1);
        printf("  Capacity: %llu bytes\n", ledger_get_capacity(entry1));
        printf("  Length: %u bytes\n\n", ledger_get_lenght(entry1));
    }

    // 3. Smart Reallocation (In-place or Move)
    printf("[Step 2] Resizing an Entry (Realloc)\n");
    entry1 = (char *)ledger_realloc(nb, entry1, 256);
    strcat(entry1, " Adding more notes to the same page.");
    ledger_set_lenght(entry1, (u32)strlen(entry1));
    printf("  Updated Length: %u / New Capacity: %llu\n\n", 
            ledger_get_lenght(entry1), ledger_get_capacity(entry1));

    // 4. Triggering the "Next Page" (Child Ledger)
    // We request 2KB, which is more than the remaining space in the 1KB initial page.
    printf("[Step 3] Turning the Page (Overflow to Child Ledger)\n");
    void *huge_entry = ledger_alloc(nb, 2048);
    if (huge_entry) {
        printf("  Successfully allocated 2KB. The Notebook automatically added a child page!\n\n");
    }

    // 5. Freeing and Memory Management
    printf("[Step 4] Freeing Blocks\n");
    void *temp = ledger_alloc(nb, 64);
    ledger_free(nb, temp); // This block is now marked as free and can be reused
    printf("  Block freed. If enough frees occur, adjacent blocks will merge.\n\n");

    // 6. Detailed Introspection
    printf("[Step 5] Final Notebook Inspection\n");
    // This prints the status of all blocks in all pages (Used/Free, Capacity, Length, Hex)
    ledger_print_child(nb, false); 

    // 7. Shred the Notebook
    printf("Closing the notebook and freeing all system memory...\n");
    ledger_delete(nb);

    return 0;
}
```
---------------------------------------------------------

## Function Reference

### Ledger* ledger_create(u64 size)
```
Creates a new ledger (the first page of the notebook).
* size: The size of the ledger in bytes.
* Returns: A pointer to the created ledger.
```

### void* ledger_alloc(Ledger *ledger, u64 size)
```
Allocates a block of memory. If the current page is full, it searches for free blocks or creates a larger child page.
* Returns: A pointer to the allocated block.
```

### void ledger_free(Ledger *ledger, void *ptr)
```
Frees a previously allocated block. The block is cleared and marked as free for future reuse.
```

### void* ledger_realloc(Ledger *ledger, void *ptr, u64 new_size)
```
Resizes a block. If the block is at the end of the offset or next to a free block, it expands on the spot. Otherwise, it moves the data to a new location.
```

### void ledger_merge_free_blocks(Ledger *ledger)
```
Merges adjacent free blocks in the ledger into a single larger block to reduce fragmentation.
```

### void ledger_reset(Ledger *ledger)
```
Resets the entire chain of ledgers, clearing all allocations while keeping the memory for the next use cycle.
```

### void ledger_delete(Ledger *ledger)
```
Deletes the ledger and all its child pages, freeing all associated memory from the system.
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