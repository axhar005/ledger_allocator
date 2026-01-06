#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/ledger.h"

// Définition de flags pour le test (simulant un moteur de données)
#define TYPE_STRING  10
#define TYPE_INT     20
#define TYPE_SECRET  42

int main() {
    printf("=== Ledger Allocator: Professional Validation Demo ===\n\n");

    // 1. Initialisation
    // Création d'un Ledger Master de 1KB
    Ledger *my_ledger = ledger_create(1024); 
    if (!my_ledger) {
        fprintf(stderr, "CRITICAL: Failed to initialize ledger.\n");
        return 1;
    }
    printf("[OK] Ledger created (Master: 1KB)\n\n");

    // 2. Test Malloc & Metadata
    printf("[Step 1] Basic Malloc & Metadata Testing\n");
    char *entry1 = (char *)ledger_malloc(my_ledger, 128); 
    if (entry1) {
        strcpy(entry1, "Learning C at 42 is challenging but fun!");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        printf("  Data: %s\n", entry1);
        printf("  Capacity: %u | Length: %u\n", 
               ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 3. Test Realloc (Expansion & Split)
    printf("[Step 2] Resizing Entry (Realloc + Split Testing)\n");
    char *entry1_new = (char *)ledger_realloc(my_ledger, entry1, 256);
    if (entry1_new) {
        entry1 = entry1_new;
        strcat(entry1, " Testing expansion.");
        ledger_set_lenght(entry1, (u32)strlen(entry1));
        printf("  New Capacity: %u | New Length: %u\n", 
                ledger_get_capacity(entry1), ledger_get_lenght(entry1));
    }
    printf("\n");

    // 4. Test Calloc & Child Ledger (Overflow)
    printf("[Step 3] Overflow & Calloc Test (Automatic Chaining)\n");
    // On demande 2KB sur un Ledger de 1KB -> Force la création d'un Child
    int *numbers = (int *)ledger_calloc(my_ledger, 512 * sizeof(int)); 
    if (numbers) {
        printf("  [OK] 2KB allocated in Child page. Checking if zeroed: numbers[0] = %d\n", numbers[0]);
        assert(numbers[0] == 0);
    }
    printf("\n");

    // 5. Advanced Flags Validation (Filtering & Iteration)
    printf("[Step 4] Advanced Flags Validation (Search & Count)\n");
    
    // On marque nos blocs avec des types différents
    char *s1 = (char *)ledger_malloc_flag(my_ledger, 32, TYPE_STRING);
    strcpy(s1, "Entry Alpha");
    
    char *s2 = (char *)ledger_malloc_flag(my_ledger, 32, TYPE_STRING);
    strcpy(s2, "Entry Beta");
    
    int *n1 = (int *)ledger_calloc_flag(my_ledger, sizeof(int), TYPE_INT);
    *n1 = 1000;

    char *s3 = (char *)ledger_malloc_flag(my_ledger, 32, TYPE_STRING);
    strcpy(s3, "Entry Gamma");

    // Test de comptage
    u32 string_count = ledger_count_flag(my_ledger, TYPE_STRING);
    printf("  [Count] Expected: 3 | Found: %u strings\n", string_count);
    assert(string_count == 3);

    // Test de recherche itérative
    printf("  [Search] Iterating through all blocks of type STRING:\n");
    void *search_ptr = NULL;
    int count = 0;
    while ((search_ptr = ledger_find_flag(my_ledger, search_ptr, TYPE_STRING)) != NULL) {
        printf("    - Found block #%d at %p: \"%s\"\n", ++count, search_ptr, (char *)search_ptr);
    }
    assert(count == 3);
    printf("\n");

    // 6. Security (Double Free & Invalid)
    printf("[Step 5] Security Testing (Safety Guardrails)\n");
    void *test_ptr = ledger_malloc(my_ledger, 64);
    if (ledger_free(my_ledger, test_ptr)) 
        printf("  [OK] First free successful.\n");
    
    if (!ledger_free(my_ledger, test_ptr)) 
        printf("  [OK] Double-free prevented (Operation ignored).\n");
    
    int stack_var = 42;
    if (!ledger_free(my_ledger, &stack_var)) 
        printf("  [OK] Foreign pointer (stack) rejected safely.\n");
    printf("\n");

    // 7. Dirty Memory & Selective Memset
    printf("[Step 6] Dirty Memory vs Calloc Security\n");
    char *dirty = (char *)ledger_malloc(my_ledger, 100);
    if (dirty) {
        memset(dirty, 'X', 100); 
        ledger_free(my_ledger, dirty); // 'X' restent physiquement mais le flag est à 0
        printf("  [1] Created 100 bytes of 'X' and freed them.\n");
    }

    // On réalloue avec calloc au même endroit (normalement)
    char *clean = (char *)ledger_calloc(my_ledger, 50);
    bool is_zeroed = true;
    for (int i = 0; i < 50; i++) if (clean[i] != 0) is_zeroed = false;
    printf("  [2] Reallocated over 'X' with calloc. Is memory zeroed? %s\n", 
           is_zeroed ? "YES (Safe)" : "NO (Dirty)");
    assert(is_zeroed);
    printf("\n");

    // 8. Final Visual Dump
    printf("[Step 7] Final Ledger State (Introspection)\n");
    ledger_dump(my_ledger);

    // 9. Reset & Cleanup
    printf("[Step 8] Testing Reset & Delete\n");
    if (ledger_reset(my_ledger)) 
        printf("  [OK] Ledger reset successful (All memory cleared, metadata zeroed).\n");

    if (ledger_delete(my_ledger)) 
        printf("  [OK] Ledger and all child pages destroyed. Goodbye!\n");

    return 0;
}