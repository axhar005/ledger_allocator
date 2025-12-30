#include "../include/arena.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
	Arena *ar = arena_create(2048);
	assert(ar != NULL);
	printf("--- Edge Case Tests ---\n");

	// Test 1: Downsizing Realloc (Réduction de taille)
	printf("\n--- Test A: Downsizing Realloc ---\n");
	char *d1 = (char *)arena_alloc(ar, 500);
	strcpy(d1, "Keep Me");
	void *old_ptr = d1;
	
	// On réduit de 500 à 50 octets
	d1 = (char *)arena_realloc(ar, d1, 50);
	printf("Downsized Realloc: %s, Same Ptr: %s\n", d1, (old_ptr == d1) ? "YES" : "NO");
	assert(strcmp(d1, "Keep Me") == 0);
	assert(arena_get_data_size(d1) == 50);

	// Test 2: Fragmentation & First Fit
	printf("\n--- Test B: Fragmentation Recovery ---\n");
	void *p[5];
	for(int i = 0; i < 5; i++) p[i] = arena_alloc(ar, 100);
	
	// On libère les indices 1 et 3 (crée des trous)
	printf("Freeing p[1] and p[3] to create holes...\n");
	arena_free(ar, p[1]);
	arena_free(ar, p[3]);
	
	// On essaie d'allouer dans un trou
	void *hole_fill = arena_alloc(ar, 80);
	printf("Hole at p[1] was %p, new allocation at %p\n", p[1], hole_fill);
	assert(hole_fill == p[1]); // Devrait réutiliser exactement le même pointeur

	// Test 3: Realloc NULL (doit se comporter comme alloc)
	printf("\n--- Test C: Realloc NULL ---\n");
	void *r_null = arena_realloc(ar, NULL, 120);
	assert(r_null != NULL);
	printf("Realloc(NULL) successful at %p\n", r_null);
	(void)r_null;

	// Test 4: Free NULL (ne doit pas crash)
	printf("\n--- Test D: Free NULL ---\n");
	arena_free(ar, NULL); 
	printf("Free(NULL) did not crash.\n");

	printf("\n--- Final Integrity Check ---\n");
	arena_print(ar, false);

	arena_delete(ar);
	printf("All Edge Cases Passed.\n");
	return 0;
}