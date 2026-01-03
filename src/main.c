#include <stdio.h>
#include <string.h>
#include "../include/ledger.h"

int main() {
	printf("=== Ledger Allocator: Ledger Edition Demo ===\n\n");

	// 1. Initialize the Ledger with a small page (1KB)
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
		
		// Use helper functions to manage the "Ledger" entry
		ledger_set_lenght(entry1, (u32)strlen(entry1));
		
		printf("  Data: %s\n", entry1);
		printf("  Capacity: %u bytes\n", ledger_get_capacity(entry1));
		printf("  Length: %u bytes\n\n", ledger_get_lenght(entry1));
	}

	// 3. Smart Reallocation (In-place or Move)
	printf("[Step 2] Resizing an Entry (Realloc)\n");
	entry1 = (char *)ledger_realloc(nb, entry1, 256);
	strcat(entry1, " Adding more notes to the same page.");
	ledger_set_lenght(entry1, (u32)strlen(entry1));
	printf("  Updated Length: %u / New Capacity: %u\n\n", 
			ledger_get_lenght(entry1), ledger_get_capacity(entry1));

	// 4. Triggering the "Next Page" (Child Ledger)
	// We request 2KB, which is more than the remaining space in the 1KB initial page.
	printf("[Step 3] Turning the Page (Overflow to Child Ledger)\n");
	void *huge_entry = ledger_alloc(nb, 2048);
	if (huge_entry) {
		printf("  Successfully allocated 2KB. The Ledger automatically added a child page!\n\n");
	}

	// 5. Test d'allocations massives (Stress Test)
	printf("[Step 4] Massive Small Allocations Test\n");
	for (int i = 0; i < 15; i++) {
		// On alloue des blocs de 100 octets. 
		// Comme le Master fait 1KB, cela va rapidement déborder vers des Childs.
		void *small_ptr = ledger_alloc(nb, 100);
		if (small_ptr) {
			char buffer[50];
			sprintf(buffer, "Note #%d", i);
			memcpy(small_ptr, buffer, strlen(buffer));
			ledger_set_lenght(small_ptr, (u32)strlen(buffer));
		}
	}
	printf("  15 small blocks allocated. Checking chain growth...\n\n");

	// 6. Freeing and Memory Management
	printf("[Step 5] Freeing Blocks\n");
	void *temp = ledger_alloc(nb, 64);
	ledger_free(nb, temp); // This block is now marked as free and can be reused
	printf("  Block freed. If enough frees occur, adjacent blocks will merge.\n\n");

	// 6. Detailed Introspection
	printf("[Step 6] Final Ledger Inspection\n");
	// This prints the status of all blocks in all pages (Used/Free, Capacity, Length, Hex)

	// ledger_print_child(nb, false);
	ledger_dump(nb);

	// 7. Shred the Ledger
	printf("Closing the Ledger and freeing all system memory...\n");
	ledger_delete(nb);

	return 0;
}