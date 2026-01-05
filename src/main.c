#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/ledger.h"

int main() {
    printf("=== Ledger Allocator: Professional Validation Demo ===\n\n");

    // 1. Initialisation
    Ledger *nb = ledger_create(1024); 
    if (!nb) {
        fprintf(stderr, "CRITICAL: Failed to initialize ledger.\n");
        return 1;
    }
    printf("[OK] Ledger created (Master: 1KB)\n\n");

    // 2. Test Malloc & Setters
    printf("[Step 1] Basic Malloc & Metadata Testing\n");
    char *entry1 = (char *)ledger_malloc(nb, 128); 
    if (entry1) {
        strcpy(entry1, "Learning C at 42 is challenging but fun!");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        printf("  Data: %s\n", entry1);
        printf("  Capacity: %u | Length: %u\n", ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 3. Test Realloc (Expansion & Split)
    printf("[Step 2] Resizing Entry (Realloc + Split Testing)\n");
    char *entry1_new = (char *)ledger_realloc(nb, entry1, 256);
    if (entry1_new) {
        entry1 = entry1_new;
        strcat(entry1, " Testing expansion.");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        printf("  New Capacity: %u | New Length: %u\n", 
                ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 4. Test Calloc & Child Ledger (Overflow)
    printf("[Step 3] Overflow & Calloc Test (Child Page)\n");
    // calloc doit mettre à zéro automatiquement
    int *numbers = (int *)ledger_calloc(nb, 512 * sizeof(int)); // 2KB -> Force un Child
    if (numbers) {
        printf("  [OK] 2KB allocated in Child. Checking if zeroed: numbers[0] = %d\n", numbers[0]);
        assert(numbers[0] == 0);
    }
    printf("\n");

    // 5. Massive Small Allocations (Stress Test)
    printf("[Step 4] Massive Small Allocations (Chain Growth)\n");
    for (int i = 0; i < 15; i++) {
        void *small_ptr = ledger_malloc(nb, 80);
        if (small_ptr) {
            char buffer[20];
            sprintf(buffer, "ID:%d", i);
            memcpy(small_ptr, buffer, strlen(buffer));
            ledger_set_lenght(small_ptr, (u32)strlen(buffer));
        }
    }
    printf("  15 small blocks allocated across the chain.\n\n");

    // 6. Security (Double Free & Invalid)
    printf("[Step 5] Security Testing (Boolean returns)\n");
    void *test_ptr = ledger_malloc(nb, 64);
    if (ledger_free(nb, test_ptr)) printf("  [OK] First free successful.\n");
    if (!ledger_free(nb, test_ptr)) printf("  [OK] Double free prevented (returned false).\n");
    
    int stack_var = 42;
    if (!ledger_free(nb, &stack_var)) printf("  [OK] Foreign pointer rejected.\n");
    printf("\n");

    // 7. Dirty Memory & Selective Memset (Surgery Test)
    printf("[Step 6] Selective Memset & Dirty Memory Test\n");
    char *dirty = (char *)ledger_malloc(nb, 100);
    if (dirty) {
        memset(dirty, 'X', 100); 
        ledger_free(nb, dirty); // 'X' restent en mémoire
        printf("  [1] Created 100 bytes of 'X' and freed them.\n");
    }

    char *pre = (char *)ledger_malloc(nb, 50);
    printf("  [2] Reallocating 50 -> 70 bytes (Eating 20 bytes of 'X')...\n");
    pre = (char *)ledger_realloc(nb, pre, 70);

    bool is_clean = true;
    for (int i = 50; i < 70; i++) if (pre[i] != 0) is_clean = false;

    metadata *meta_pre = _ledger_get_block_metadata(pre);
    u8 *next_data_ptr = (u8 *)meta_pre + meta_pre->block_size + META_SIZE_ALIGNED;

    printf("  [Result] Extended zone (50-70) is %s\n", is_clean ? "CLEAN (0x00)" : "DIRTY");
    printf("  [Result] Residue after split is %s\n", (*next_data_ptr == 'X') ? "STILL DIRTY (Correct)" : "CLEANED");
    printf("\n");

    // 8. Flags Testing
    printf("[Step 7] Custom Flags Validation\n");
    u16 secret = 0x4242;
    void *f_ptr = ledger_malloc_flag(nb, 32, secret);
    if (f_ptr) {
        printf("  Set Flag: 0x%X | Retrieved: 0x%X\n", secret, ledger_get_flag(f_ptr));
        assert(ledger_get_flag(f_ptr) == secret);
    }
    printf("\n");

    // 9. Final Visual & Cleanup
    printf("[Step 8] Final Ledger State\n");
    ledger_dump(nb);

    printf("[Step 9] Testing Reset & Delete\n");
    if (ledger_reset(nb)) printf("  [OK] Reset successful.\n");
    if (ledger_delete(nb)) printf("  [OK] Ledger destroyed.\n");

    return 0;
}