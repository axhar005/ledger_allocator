# Ledger Allocator

## Overview

Ledger Allocator is a custom memory tracking and management system designed to give developers full control over their data's lifecycle. Instead of dealing with "blind" pointers, this system treats memory as a series of "Smart Entries" where metadata—such as capacity, current length, and custom flags—is stored directly alongside the raw data.

The Ledger Allocator is essentially a hybrid system: it is built upon the speed of an Arena Allocator, integrated with classic memory management functions (malloc, calloc, free, realloc), and features a Chaining system where each overflow creates a child Ledger that is always 2x larger than the previous one.

This approach eliminates the need for constant manual tracking or expensive operations like `strlen()`. By embedding the state within the allocation itself, the Ledger Allocator acts as a "Memory Diary": every entry knows its own limits, supports custom tagging via flags, and every "Page" (Ledger) is linked to the next through an automated chaining system, ensuring you never lose grip on your program's heap.

## Why Use Ledger Allocator?

### 🐛 Debugging Made Easy
**"No more `valgrind` nightmares to find buffer sizes."**
- Every allocation carries its own metadata: capacity, length, and custom flags
- Use `ledger_dump()` to get a complete hex view of your entire memory layout
- Detect double-frees, invalid pointers, and memory leaks instantly with built-in safety checks

### ⚡ Performance Optimization
**"Reduce system calls, increase throughput."**
- Pre-allocated memory pages minimize expensive `brk/mmap` system calls
- O(1) metadata access without hash tables or external tracking structures
- Automatic child ledger creation (2x growth) prevents frequent reallocation
- Smart in-place reallocation reduces memory copying

### 🎮 Perfect for Data-Oriented Design
**"Built for ECS (Entity Component System) and memory-intensive applications."**
- Flag system allows instant categorization and filtering of memory blocks
- Iterate through all entities of a specific type in O(n) without external data structures
- Ideal for game engines, simulations, and high-performance computing where data locality matters
- Group related allocations in the same ledger for better cache coherency

## Memory Visualization

### 🏗️ Block Structure (The "Ledger" Entry)
Each allocation is composed of a metadata header followed by 16-byte aligned user data.
```
+-------------------+-------------------+-------------------+
|  Metadata (16B)   |   User Data (N)   |  Padding (Align)  |
| [Used|Flag|Cap|L] |  "Hello World"    |  0x00 00 00 ...   |
+-------------------+-------------------+-------------------+
 ^                   ^
 |--- (Internal) ----|--- (User Pointer) -------------------|
```

**Metadata Fields:**
- `block_used`: Status indicator (1 = used, 0 = free)
- `flag`: 16-bit user-defined flag for custom categorization
- `capacity`: Actual size allocated for user data
- `block_size`: Total size including metadata and alignment
- `length`: Current length of data stored in the block

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

### 🔗 Block Merging (Coalescence)
When adjacent blocks are freed, they automatically merge to reduce fragmentation.
```
BEFORE MERGE:
+--------+--------+--------+--------+--------+
| Used   | FREE   | FREE   | FREE   | Used   |
|  64B   |  32B   |  32B   |  48B   |  64B   |
+--------+--------+--------+--------+--------+

AFTER MERGE (automatic after MAX_FREE_COUNT frees):
+--------+--------+--------+--------+
| Used   |   FREE (112B)   | Used   |
|  64B   |                 |  64B   |
+--------+--------+--------+--------+
```

## Features

* **Embedded Metadata Tracking:** Every block stores its own `capacity`, `length`, and custom `flag`. Retrieve data size or string length in O(1) time without extra calculations.
* **Custom Flag System:** Attach 16-bit flags to any allocation for categorization, filtering, or searching. Perfect for tagging memory blocks by type, priority, or any custom logic.
* **malloc/calloc API:** Familiar function names (`ledger_malloc`, `ledger_calloc`) make integration seamless. Choose between uninitialized allocation (malloc) or zero-initialized memory (calloc).
* **Automated Memory Chaining:** If a Ledger exceeds its initial size, the system automatically allocates and links a "child" Ledger (2x size). Your allocations continue seamlessly.
* **16-Byte Data Alignment:** Built-in alignment logic ensures all user data is optimized for modern CPU architectures and safe for all data types.
* **Smart Reallocation:** The `realloc` engine attempts to expand blocks in-place by checking the remaining ledger offset or merging with adjacent free blocks before deciding to move data.
* **Automatic Fragmentation Recovery:** After `MAX_FREE_COUNT` (default: 10) frees, the system automatically merges adjacent free blocks into larger reusable spaces.
* **Zero-Fill Security:** Freed memory is automatically cleared (memset to zero), preventing data leakage and making memory debugging significantly easier.
* **Full Chain Introspection:** Built-in printing functions allow you to inspect every block, across every linked ledger, with hex-dump previews of the content and flag values.

## Algorithmic Complexity

| Operation | Best Case | Average Case | Worst Case | Notes |
|-----------|-----------|--------------|------------|-------|
| `ledger_malloc` | O(1) | O(1) | O(n) | O(1) when allocating at end; O(n) when searching for free blocks |
| `ledger_calloc` | O(1) | O(1) | O(n) | Same as malloc + memset overhead |
| `ledger_free` | O(1) | O(1) | O(n) | O(1) for marking free; O(n) every MAX_FREE_COUNT frees (merge) |
| `ledger_realloc` | O(1) | O(n) | O(n) | O(1) for in-place expansion; O(n) when copying to new location |
| `ledger_get_capacity` | O(1) | O(1) | O(1) | Direct metadata access |
| `ledger_get_lenght` | O(1) | O(1) | O(1) | Direct metadata access |
| `ledger_find_flag` | O(1) | O(n) | O(n) | Linear search through all blocks |
| `ledger_count_flag` | O(n) | O(n) | O(n) | Must traverse entire chain |
| `ledger_merge_free_blocks` | O(n) | O(n) | O(n) | Single pass through all blocks |

**n** = total number of allocated blocks across all ledger pages

## Project Structure
```
.
├── include/
│   └── ledger.h      # API Definitions & Metadata structures
├── src/
│   ├── ledger.c      # Core logic (allocation, merging, chaining)
│   └── main.c        # Demonstration and testing suite
├── build.bat         # Windows Build Script (CMake/Ninja)
├── run.bat           # Windows Execution Script
├── Makefile          # Linux/macOS Build Script
└── CMakeLists.txt    # Cross-platform CMake configuration
```

## Installation
To use Ledger Allocator in your project, clone the repository and include the source files:

```bash
git clone https://github.com/axhar005/ledger_allocator.git
```

### Building on Windows
This project includes dedicated scripts for Windows users:
```
build.bat    :: Compiles the project using CMake
run.bat      :: Executes the compiled demo
```

### Building on Linux/macOS
```bash
make         # Compiles the library and main example
./ledger     # Run the executable
```

## Usage
Include the header file in your code:
```c
#include "ledger.h"
```

### Example: Complete Feature Demonstration
```c
#include <stdio.h>
#include <string.h>
#include "../include/ledger.h"

int main() {
    printf("=== Ledger Allocator: Complete Feature Demo ===\n\n");

    // 1. Initialize the Ledger with a small page (1KB)
    Ledger *ledger = ledger_create(1024); 
    if (!ledger) {
        fprintf(stderr, "Failed to initialize ledger.\n");
        return 1;
    }
    printf("[OK] Ledger created (1KB initial size)\n\n");

    // 2. Basic malloc() - Allocate without initialization
    printf("[Step 1] Using ledger_malloc (uninitialized memory)\n");
    char *entry1 = (char *)ledger_malloc(ledger, 128);
    if (entry1) {
        strcpy(entry1, "Learning C at 42 is challenging but fun!");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        
        printf("  Data: %s\n", entry1);
        printf("  Capacity: %u | Length: %u\n", 
               ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 3. Using calloc() - Allocate with zero-initialization
    printf("[Step 2] Using ledger_calloc (zero-initialized memory)\n");
    int *numbers = (int *)ledger_calloc(ledger, 10 * sizeof(int));
    if (numbers) {
        printf("  Allocated 10 integers, all zeroed: [");
        for (int i = 0; i < 5; i++) {
            printf("%d ", numbers[i]);
        }
        printf("...]\n");
        
        // Fill with data
        for (int i = 0; i < 10; i++) {
            numbers[i] = i * 10;
        }
        ledger_set_lenght(numbers, 10 * sizeof(int));
    }
    printf("\n");

    // 4. Custom Flags System - Categorize allocations
    printf("[Step 3] Custom Flag System (Tagging & Filtering)\n");
    
    #define FLAG_STRING  0x0001
    #define FLAG_NUMBER  0x0002
    #define FLAG_BUFFER  0x0004
    
    char *text1 = (char *)ledger_malloc_flag(ledger, 64, FLAG_STRING);
    strcpy(text1, "Tagged as STRING");
    
    int *data1 = (int *)ledger_calloc_flag(ledger, 5 * sizeof(int), FLAG_NUMBER);
    data1[0] = 42;
    
    char *text2 = (char *)ledger_malloc_flag(ledger, 64, FLAG_STRING);
    strcpy(text2, "Another STRING entry");
    
    void *buffer = ledger_malloc_flag(ledger, 128, FLAG_BUFFER);
    
    printf("  Created blocks with flags: STRING(0x%04X), NUMBER(0x%04X), BUFFER(0x%04X)\n", 
           FLAG_STRING, FLAG_NUMBER, FLAG_BUFFER);
    printf("  Count of STRING blocks: %u\n", ledger_count_flag(ledger, FLAG_STRING));
    printf("  Count of NUMBER blocks: %u\n", ledger_count_flag(ledger, FLAG_NUMBER));
    printf("\n");

    // 5. Finding blocks by flag
    printf("[Step 4] Searching Blocks by Flag\n");
    void *found = NULL;
    int index = 1;
    while ((found = ledger_find_flag(ledger, found, FLAG_STRING))) {
        printf("  Found STRING block #%d: \"%s\"\n", index++, (char *)found);
    }
    printf("\n");

    // 6. Smart Reallocation
    printf("[Step 5] Smart Reallocation (In-place expansion)\n");
    char *entry1_new = (char *)ledger_realloc(ledger, entry1, 256);
    if (entry1_new) {
        entry1 = entry1_new;
        strcat(entry1, " Extended with more notes.");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        printf("  New Capacity: %u | New Length: %u\n", 
               ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 7. Triggering Child Ledger (Overflow)
    printf("[Step 6] Page Overflow (Creating Child Ledger)\n");
    void *huge = ledger_calloc(ledger, 2048); // Forces creation of 2KB child
    if (huge) {
        printf("  Allocated 2KB in child ledger (automatic chaining)\n");
    }
    printf("\n");

    // 8. Memory Management
    printf("[Step 7] Freeing & Merging Blocks\n");
    void *temp1 = ledger_malloc(ledger, 64);
    void *temp2 = ledger_malloc(ledger, 64);
    void *temp3 = ledger_malloc(ledger, 64);
    
    ledger_free(ledger, temp1);
    ledger_free(ledger, temp2);
    ledger_free(ledger, temp3);
    printf("  Freed 3 adjacent blocks. Merge count triggers automatic coalescence.\n\n");

    // 9. Security Testing
    printf("[Step 8] Security Features\n");
    void *test = ledger_malloc(ledger, 32);
    if (ledger_free(ledger, test)) printf("  [OK] First free successful\n");
    if (!ledger_free(ledger, test)) printf("  [OK] Double-free prevented\n");
    
    int stack_var = 0;
    if (!ledger_free(ledger, &stack_var)) printf("  [OK] Foreign pointer rejected\n");
    printf("\n");

    // 10. Complete Inspection
    printf("[Step 9] Complete Ledger State (All Pages)\n");
    ledger_dump(ledger);

    // 11. Cleanup
    printf("[Step 10] Cleanup & Destruction\n");
    if (ledger_reset(ledger)) printf("  [OK] Ledger reset successful\n");
    if (ledger_delete(ledger)) printf("  [OK] Ledger destroyed\n");

    return 0;
}
```

## Function Reference

### Memory Allocation

#### `Ledger* ledger_create(u64 size)`
Creates a new ledger (the first page of the notebook).
- **size**: The size of the ledger in bytes.
- **Returns**: A pointer to the created ledger.
- **Complexity**: O(1)

#### `void* ledger_malloc(Ledger *ledger, u32 size)`
Allocates a block of memory without initialization (like standard malloc).
- **ledger**: Pointer to the ledger.
- **size**: Size to allocate in bytes.
- **Returns**: Pointer to allocated block.
- **Complexity**: O(1) best case, O(n) worst case

#### `void* ledger_calloc(Ledger *ledger, u32 size)`
Allocates a block of memory and initializes it to zero (like standard calloc).
- **ledger**: Pointer to the ledger.
- **size**: Size to allocate in bytes.
- **Returns**: Pointer to zero-initialized block.
- **Complexity**: O(1) best case, O(n) worst case

#### `void* ledger_malloc_flag(Ledger *ledger, u32 size, u16 flag)`
Allocates memory with a custom flag for categorization.
- **flag**: User-defined 16-bit flag value.
- **Returns**: Pointer to allocated block with flag set.
- **Complexity**: O(1) best case, O(n) worst case

#### `void* ledger_calloc_flag(Ledger *ledger, u32 size, u16 flag)`
Allocates zero-initialized memory with a custom flag.
- **flag**: User-defined 16-bit flag value.
- **Returns**: Pointer to zero-initialized block with flag set.
- **Complexity**: O(1) best case, O(n) worst case

#### `void* ledger_realloc(Ledger *ledger, void *ptr, u32 new_size)`
Resizes a block. Attempts in-place expansion before moving data.
- **ptr**: Pointer to previously allocated block.
- **new_size**: New size in bytes.
- **Returns**: Pointer to resized block (may differ from original).
- **Complexity**: O(1) for in-place, O(n) when relocating

#### `bool ledger_free(Ledger *ledger, void *ptr)`
Frees a previously allocated block and clears its memory.
- **Returns**: `true` if successful, `false` otherwise.
- **Complexity**: O(1), triggers O(n) merge every MAX_FREE_COUNT frees

### Flag Management

#### `bool ledger_set_flag(void *ptr, u16 flag)`
Sets a custom flag on an allocated block.
- **flag**: 16-bit flag value.
- **Returns**: `true` if successful.
- **Complexity**: O(1)

#### `u16 ledger_get_flag(void *ptr)`
Retrieves the flag value from a block.
- **Returns**: The 16-bit flag value.
- **Complexity**: O(1)

#### `void* ledger_find_flag(Ledger *ledger, void *start_after, u16 flag)`
Finds the next block with the specified flag.
- **start_after**: Pointer to start searching after (NULL for start).
- **flag**: Flag value to search for.
- **Returns**: Pointer to found block or NULL.
- **Complexity**: O(n) where n is the number of blocks

#### `u32 ledger_count_flag(Ledger *ledger, u16 flag)`
Counts blocks with the specified flag across all pages.
- **Returns**: Number of blocks matching the flag.
- **Complexity**: O(n) where n is the total number of blocks

### Metadata Management

#### `bool ledger_set_lenght(void *ptr, u32 lenght)`
Sets the current length of data in the block.
- **Returns**: `true` if successful, `false` if length exceeds capacity.
- **Complexity**: O(1)
- **Note**: API uses `lenght` spelling (consider `length` for future versions)

#### `u32 ledger_get_lenght(void *ptr)`
Gets the current data length.
- **Returns**: Current length in bytes.
- **Complexity**: O(1)

#### `u32 ledger_get_capacity(void *ptr)`
Gets the allocated capacity of the block.
- **Returns**: Capacity in bytes.
- **Complexity**: O(1)

#### `u32 ledger_get_block_size(void *ptr)`
Gets the total block size including metadata and alignment.
- **Returns**: Total block size in bytes.
- **Complexity**: O(1)

### Ledger Management

#### `bool ledger_merge_free_blocks(Ledger *ledger)`
Manually triggers merging of adjacent free blocks to reduce fragmentation.
- **Returns**: `true` if any blocks were merged.
- **Complexity**: O(n) single pass through all blocks
- **Note**: Automatically called every MAX_FREE_COUNT frees

#### `bool ledger_reset(Ledger *ledger)`
Resets all pages in the chain, clearing all allocations.
- **Returns**: `true` if successful.
- **Complexity**: O(m) where m is the number of pages

#### `bool ledger_delete(Ledger *ledger)`
Deletes the ledger and all child pages, freeing system memory.
- **Returns**: `true` if successful.
- **Complexity**: O(m) where m is the number of pages

### Inspection & Debugging

#### `void ledger_print(Ledger *ledger, bool content)`
Prints the state of a single ledger page.
- **content**: If `true`, prints hex dump of block contents.
- **Complexity**: O(n)

#### `void ledger_print_child(Ledger *ledger, bool content)`
Prints all pages in the ledger chain.
- **Complexity**: O(n × m) where m is the number of pages

#### `void ledger_dump(Ledger *ledger)`
Dumps complete ledger structure with detailed metadata and flags.
- **Complexity**: O(n × m)

## Advanced Usage Examples

### Using Flags for Memory Categories (ECS Pattern)
```c
#define TYPE_PLAYER     0x0100
#define TYPE_ENEMY      0x0200
#define TYPE_PROJECTILE 0x0300
#define TYPE_PARTICLE   0x0400

// Create categorized allocations
Entity *player = (Entity *)ledger_calloc_flag(game_ledger, sizeof(Entity), TYPE_PLAYER);
Entity *enemy1 = (Entity *)ledger_calloc_flag(game_ledger, sizeof(Entity), TYPE_ENEMY);
Entity *enemy2 = (Entity *)ledger_calloc_flag(game_ledger, sizeof(Entity), TYPE_ENEMY);

// Count enemies in memory - O(n) but no hash table needed
u32 enemy_count = ledger_count_flag(game_ledger, TYPE_ENEMY);
printf("Active enemies: %u\n", enemy_count);

// Update all enemies without external array
Entity *enemy = NULL;
while ((enemy = (Entity *)ledger_find_flag(game_ledger, enemy, TYPE_ENEMY))) {
    update_enemy(enemy);
}

// Batch cleanup: reset entire ledger instead of individual frees
ledger_reset(game_ledger); // O(m) instead of O(n) individual frees
```

### Dynamic String Management with Length Tracking
```c
Ledger *ledger = ledger_create(4096);

// Allocate string buffer
char *text = (char *)ledger_malloc(ledger, 100);
strcpy(text, "Initial text");
ledger_set_lenght(text, strlen(text));

// No need to call strlen() again - O(1) access
printf("String length: %u\n", ledger_get_lenght(text)); // Instant!

// Extend the string - smart reallocation
text = (char *)ledger_realloc(ledger, text, 200);
strcat(text, " - Extended!");
ledger_set_lenght(text, strlen(text));

// Check actual usage vs capacity
printf("Using %u / %u bytes (%.1f%% utilization)\n", 
       ledger_get_lenght(text), 
       ledger_get_capacity(text),
       (float)ledger_get_lenght(text) / ledger_get_capacity(text) * 100.0f);

ledger_delete(ledger);
```

### Memory Pool for Network Buffers
```c
#define FLAG_SEND_BUFFER 0x0001
#define FLAG_RECV_BUFFER 0x0002

Ledger *net_pool = ledger_create(1024 * 64); // 64KB pool

// Allocate buffers with categorization
void *send_buf = ledger_malloc_flag(net_pool, 1024, FLAG_SEND_BUFFER);
void *recv_buf = ledger_malloc_flag(net_pool, 1024, FLAG_RECV_BUFFER);

// After network operations, bulk cleanup
ledger_reset(net_pool); // Clear all buffers at once - no loops!
```

## Known Limitations & Future Improvements

### Current API Spelling
The current API uses `ledger_set_lenght()` and `ledger_get_lenght()` (note the spelling). Future versions may standardize to `length` for consistency with standard English spelling. For backward compatibility, consider adding aliases in your wrapper code if needed.

### Fragmentation in Long-Running Applications
While the automatic merge system (triggered every MAX_FREE_COUNT frees) handles most fragmentation, long-running applications with highly dynamic allocation patterns may benefit from periodic manual calls to `ledger_merge_free_blocks()` or `ledger_reset()` during safe points.

### Thread Safety
This allocator is **not thread-safe** by design. If you need concurrent access, wrap calls in mutexes or use separate ledgers per thread.

## Contributing
Contributions are welcome! If you have suggestions, bug reports, or feature requests, please open an issue or submit a pull request.

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes and commit them (`git commit -m 'Add new feature'`).
4. Push to the branch (`git push origin feature-branch`).
5. Open a pull request.

## License
This project is licensed under the MIT License. See the LICENSE file for details.