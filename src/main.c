#include <stdio.h>
#include <string.h>
#include "../include/arena.h"

int main() {
    printf("=== Arena Allocator: Notebook Edition Demo ===\n\n");

    // 1. Initialize the Notebook with a small page (1KB)
    // We use a small size to easily demonstrate the "Page Turning" (Child creation)
    Arena *nb = arena_create(1024); 
    if (!nb) {
        fprintf(stderr, "Failed to initialize arena.\n");
        return 1;
    }

    // 2. Basic Allocation & Data Manipulation
    printf("[Step 1] Basic Allocation\n");
    char *entry1 = (char *)arena_alloc(nb, 128);
    if (entry1) {
        strcpy(entry1, "Learning C at 42 is challenging but fun!");
        
        // Use helper functions to manage the "Notebook" entry
        arena_set_lenght(entry1, (u32)strlen(entry1));
        
        printf("  Data: %s\n", entry1);
        printf("  Capacity: %llu bytes\n", arena_get_capacity(entry1));
        printf("  Length: %u bytes\n\n", arena_get_lenght(entry1));
    }

    // 3. Smart Reallocation (In-place or Move)
    printf("[Step 2] Resizing an Entry (Realloc)\n");
    entry1 = (char *)arena_realloc(nb, entry1, 256);
    strcat(entry1, " Adding more notes to the same page.");
    arena_set_lenght(entry1, (u32)strlen(entry1));
    printf("  Updated Length: %u / New Capacity: %llu\n\n", 
            arena_get_lenght(entry1), arena_get_capacity(entry1));

    // 4. Triggering the "Next Page" (Child Arena)
    // We request 2KB, which is more than the remaining space in the 1KB initial page.
    printf("[Step 3] Turning the Page (Overflow to Child Arena)\n");
    void *huge_entry = arena_alloc(nb, 2048);
    if (huge_entry) {
        printf("  Successfully allocated 2KB. The Notebook automatically added a child page!\n\n");
    }

    // 5. Freeing and Memory Management
    printf("[Step 4] Freeing Blocks\n");
    void *temp = arena_alloc(nb, 64);
    arena_free(nb, temp); // This block is now marked as free and can be reused
    printf("  Block freed. If enough frees occur, adjacent blocks will merge.\n\n");

    // 6. Detailed Introspection
    printf("[Step 5] Final Notebook Inspection\n");
    // This prints the status of all blocks in all pages (Used/Free, Capacity, Length, Hex)
    arena_print_child(nb, false); 

    // 7. Shred the Notebook
    printf("Closing the notebook and freeing all system memory...\n");
    arena_delete(nb);

    return 0;
}