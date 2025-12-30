#include "../include/arena.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
    Arena *ar = arena_create(4096);
    printf("=== HARDCORE STRESS TESTS ===\n\n");

    // --- TEST 8: LA FUSION TRIPLE (Triple Merge) ---
    // On veut créer : [USED] [FREE] [FREE] [FREE] [USED]
    // Puis fusionner les 3 FREE en un seul bloc géant.
    printf("TEST 8: Triple Free Block Merge\n");
    void *start = arena_alloc(ar, 64);
    void *m1 = arena_alloc(ar, 64);
    void *m2 = arena_alloc(ar, 64);
    void *m3 = arena_alloc(ar, 64);
    void *end = arena_alloc(ar, 64);
    (void)start; (void)end;

    arena_free(ar, m1);
    arena_free(ar, m2);
    arena_free(ar, m3);

    // On force la fusion
    arena_merge_free_blocks(ar);

    // Si la fusion a marché, on doit pouvoir allouer un bloc de ~200 octets
    // qui commence exactement à l'adresse de m1.
    void *big = arena_alloc(ar, 180);
    printf("  Triple Merge Result: %s\n", (big == m1) ? "SUCCESS" : "FAILED");
    assert(big == m1);
    printf("  Result: Pass\n\n");


    // --- TEST 9: REALLOC VERS LE BAS ET REMONTÉE (Elasticity) ---
    printf("TEST 9: Elasticity (Down then Up)\n");
    arena_reset(ar);
    char *elastic = arena_alloc(ar, 500);
    strcpy(elastic, "ElasticMemory");
    arena_set_lenght(elastic, (u32)strlen(elastic));

    // On réduit drastiquement
    elastic = arena_realloc(ar, elastic, 10);
    assert(arena_get_lenght(elastic) == 10); // Tronqué
    
    // On remonte à la taille originale (doit rester In-Place car le block_size n'a pas changé)
    void *addr_before = elastic;
    elastic = arena_realloc(ar, elastic, 450);
    assert(elastic == addr_before);
    printf("  Elasticity In-Place: Success\n");
    printf("  Result: Pass\n\n");


    // --- TEST 10: ALIGNMENT STRESS TEST ---
    printf("TEST 10: Alignment Integrity\n");
    // On alloue des tailles impaires pour voir si l'alignement survit
    void *a1 = arena_alloc(ar, 7);
    void *a2 = arena_alloc(ar, 13);
    void *a3 = arena_alloc(ar, 1);
    
    // Chaque adresse de DATA doit être divisible par 8 (ou 16 selon ton ARENA_ALIGNMENT)
    assert((uintptr_t)a1 % ARENA_ALIGNMENT == 0);
    assert((uintptr_t)a2 % ARENA_ALIGNMENT == 0);
    assert((uintptr_t)a3 % ARENA_ALIGNMENT == 0);
    printf("  Alignment (7, 13, 1 bytes): All aligned to %d bytes.\n", ARENA_ALIGNMENT);
    printf("  Result: Pass\n\n");


    // --- TEST 11: REALLOC NULL & SIZE 0 (Standard C behavior) ---
    printf("TEST 11: Standard C Realloc Behavior\n");
    // realloc(NULL, size) == malloc(size)
    void *r1 = arena_realloc(ar, NULL, 100);
    assert(r1 != NULL);
    
    // realloc(ptr, 0) == free(ptr)
    void *r2 = arena_realloc(ar, r1, 0);
    assert(r2 == NULL);
    // On vérifie si r1 est bien marqué free
    assert(arena_is_block_free(r1) == true);
    printf("  Realloc(NULL) and Realloc(0) behavior: Correct.\n");
    printf("  Result: Pass\n\n");


// --- TEST 12: Multi-level Chaining ---
    printf("TEST 12: Multi-level Chaining\n");
    arena_reset(ar);
    
    // On sature l'arène 1 (4096)
    void *c1 = arena_alloc(ar, 3500); 
    
    // On sature l'arène 2 (8192)
    // On demande un bloc très gros pour forcer l'arène 2 à déborder
    void *c2 = arena_alloc(ar, 7000); 
    
    // On force la création de l'arène 3
    void *c3 = arena_alloc(ar, 7000);

    assert(ar->child != NULL);
    assert(ar->child->child != NULL);
    (void)c1; (void)c2; (void)c3;
    printf("  Chain: Parent -> Child -> Grand-Child: Created.\n");
    printf("  Result: Pass\n\n");

    // --- TEST 13: LENGHT VS CAPACITY BORDERLINE ---
    printf("TEST 13: Length/Capacity Borderline\n");
    char *border = arena_alloc(ar, 10);
    arena_set_lenght(border, 10); // OK
    assert(arena_get_lenght(border) == 10);
    
    arena_set_lenght(border, 11); // Doit échouer (rester à 10) car > capacity
    assert(arena_get_lenght(border) == 10);
    printf("  Safety: Length cannot exceed capacity. Verified.\n");
    printf("  Result: Pass\n\n");

    printf("\n=== ALL HARDCORE TESTS PASSED ===\n");
    arena_delete(ar);
    return 0;
}