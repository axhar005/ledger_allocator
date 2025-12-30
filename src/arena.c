/**
* Arena Allocator for Memory Management.
* 
* This file implements an arena allocator that allows efficient allocation and
* deallocation of memory blocks. The allocator uses a primary arena and can
* create child arenas in case of overflow. Allocated blocks are tracked using
* a metadata struct containing information about the block size, alloc size
* and status (free or used).
*/

#include "../include/arena.h"
#include <stdio.h>
#include <string.h>

Arena *
arena_contains_this_ptr(Arena *arena, void *data_ptr) {
	Arena *current_arena = arena;
	u8 *ptr = (u8 *)data_ptr;
	while (current_arena) {
		if (ptr >= current_arena->memory && ptr < (current_arena->memory + current_arena->size)) {
			return current_arena;
		}
		current_arena = current_arena->child;
	}
	return NULL;
}

metadata *
arena_get_block_metadata(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received.\n");
		return NULL;
	}
	return (metadata *)((u8 *)ptr - META_SIZE_ALIGNED);
}

u64
arena_get_block_size(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in arena_get_block_size function.\n");
		return 0;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	if (meta->block_size == 0) {
		fprintf(stderr, "Warning: Block size is zero.\n");
	}
	return meta->block_size;
}

bool
arena_is_block_free(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in arena_is_block_free function.\n");
		return false;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	return meta->block_used == 0;
}

void
arena_set_block_used(void *ptr, bool used) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in arena_set_block_used function.\n");
		return;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	meta->block_used = used ? 1 : 0;
}

void
arena_set_block_size(void *ptr, u64 new_size) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in arena_set_block_size function.\n");
		return;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	meta->block_size = new_size;
}

void
arena_set_capacity(void *ptr, u32 size) {
	if (ptr == NULL) {
		fprintf(stderr,"Error: Null pointer received in arena_set_capacity function.\n");
		return;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	meta->capacity = size;
}

u64
arena_get_capacity(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in arena_get_capacity function.\n");
		return 0;
	}
	metadata *meta = arena_get_block_metadata(ptr);
	return meta->capacity;
}

void
arena_set_lenght(void *ptr, u32 new_lenght) {
	if (!ptr) return;
	metadata *meta = arena_get_block_metadata(ptr);
	if (new_lenght <= meta->capacity) {
		meta->lenght = new_lenght;
	}
}

u32
arena_get_lenght(void *ptr) {
	return ptr ? arena_get_block_metadata(ptr)->lenght : 0;
}


void
arena_merge_free_blocks(Arena *arena) {
	if (!arena || arena->offset == 0) return;

	u64 current_offset = 0;
	while (current_offset < arena->offset) {
		u8 *block_ptr = arena->memory + current_offset;
		void *data_ptr = block_ptr + META_SIZE_ALIGNED;
		u64 block_size = arena_get_block_size(data_ptr);

		if (current_offset + block_size >= arena->offset) {
			break;
		}

		u8 *next_block_ptr = block_ptr + block_size;
		void *next_data_ptr = next_block_ptr + META_SIZE_ALIGNED;

		if (arena_is_block_free(data_ptr) && arena_is_block_free(next_data_ptr)) {
			u64 next_block_size = arena_get_block_size(next_data_ptr);
			u64 new_merged_size = block_size + next_block_size;
			
			arena_set_block_size(data_ptr, new_merged_size);

			metadata *meta = arena_get_block_metadata(data_ptr);
			meta->capacity = 0;
			meta->lenght = 0;
			
			memset(next_block_ptr, 0, META_SIZE_ALIGNED);
			
			continue;
		}
		current_offset += block_size;
	}
}

Arena *
arena_create(u64 size) {
	if (size == 0 || size >= MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: invalid arena size.\n");
		return NULL;
	}

	Arena *arena = (Arena *)malloc(sizeof(Arena));
	if (!arena) {
		fprintf(stderr, "Error: Failed to allocate arena. Reason: %s\n", strerror(errno));
		return NULL;
	}

	size = ARENA_ALIGN_UP(size);

	arena->memory = (u8 *)calloc(1, size); 
	
	if (!arena->memory) {
		fprintf(stderr, "Error: Failed to allocate arena memory. Reason: %s\n", strerror(errno));
		free(arena);
		return NULL;
	}

	uintptr_t addr = (uintptr_t)arena->memory;
	if (addr % ARENA_ALIGNMENT != 0) {
		fprintf(stderr, "Error: Memory alignment failure.\n");
		free(arena->memory);
		free(arena);
		return NULL;
	}

	arena->size = size;
	arena->offset = 0;
	arena->space = size;
	arena->child = NULL;
	arena->free_count = 0;

	return arena;
}

void *
arena_find_free_block(Arena *arena, u64 size) {
	Arena *current = arena;

	while (current != NULL) {
		u64 offset = 0;
		while (offset < current->offset) {
			u8 *block_ptr = current->memory + offset;
			void *data_ptr = block_ptr + META_SIZE_ALIGNED;
			u64 block_size = arena_get_block_size(data_ptr);

			if (arena_is_block_free(data_ptr) && block_size >= size) {
				return block_ptr; 
			}

			offset += block_size;
		}
		current = current->child;
	}

	return NULL;
}

void *
arena_alloc(Arena *arena, u64 size) {
	if (!arena) {
		fprintf(stderr, "Error: invalid arena ptr for aalloc.\n");
		return NULL;
	}
	if (size == 0) {
		fprintf(stderr, "Error: invalid size for aalloc.\n");
		return NULL;
	}

	u64 total_size = ARENA_ALIGN_UP(size + META_SIZE_ALIGNED);

	if (total_size > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: Block size exceeds the maximum allowable size.\n");
		return NULL;
	}

	if (total_size > arena->size) {
		if (arena->child) {
			return arena_alloc(arena->child, size);
		} else {
			arena->child = arena_create(total_size);
			if (!arena->child) return NULL;
			return arena_alloc(arena->child, size);
		}
	}

	void *free_block_ptr = arena_find_free_block(arena, total_size);
	if (free_block_ptr) {
		Arena *target_arena = arena_contains_this_ptr(arena, free_block_ptr);
		if (!target_arena) return NULL;
		void *free_data_ptr = (u8 *)free_block_ptr + META_SIZE_ALIGNED;
		u64 free_block_size = arena_get_block_size(free_data_ptr);

		if (free_block_size >= total_size + (META_SIZE_ALIGNED + ARENA_ALIGNMENT)) {
			arena_set_block_size(free_data_ptr, total_size);
			arena_set_block_used(free_data_ptr, true);
			arena_set_capacity(free_data_ptr, (u32)size);
			arena_set_lenght(free_data_ptr, 0);

			u8 *next_block_ptr = (u8 *)free_block_ptr + total_size;
			void *next_data_ptr = next_block_ptr + META_SIZE_ALIGNED;
			arena_set_block_size(next_data_ptr, (u32)(free_block_size - total_size));
			arena_set_block_used(next_data_ptr, false);
			arena_set_capacity(next_data_ptr, 0);
			arena_set_lenght(next_data_ptr, 0);

			target_arena->space -= total_size;
		} else {
			arena_set_block_used(free_data_ptr, true);
			arena_set_capacity(free_data_ptr, (u32)size);
			target_arena->space -= free_block_size;
		}
		return free_data_ptr;
	}

	u64 current_offset = ARENA_ALIGN_UP(arena->offset);

	if (current_offset + total_size > arena->size) {
		if (arena->child) {
			return arena_alloc(arena->child, size);
		} else {
			arena->child = arena_create(arena->size * 2);
			if (!arena->child) return NULL;
			return arena_alloc(arena->child, size);
		}
	}

	u8 *block_ptr = arena->memory + current_offset;
	void *data_ptr = block_ptr + META_SIZE_ALIGNED;
	
	arena_set_block_size(data_ptr, (u32)total_size);
	arena_set_block_used(data_ptr, true);
	arena_set_capacity(data_ptr, (u32)size);
	arena_set_lenght(data_ptr, 0);

	arena->offset = current_offset + total_size;
	arena->space = arena->size - arena->offset;

	return data_ptr;
}

void *
arena_realloc(Arena *arena, void *ptr, u64 new_size) {
	if (!arena) return NULL;
	
	if (!ptr) return arena_alloc(arena, new_size);
	
	if (new_size == 0) {
		arena_free(arena, ptr);
		return NULL;
	}

	Arena *current_arena = arena_contains_this_ptr(arena, ptr);
	if (!current_arena) {
		fprintf(stderr, "Error: ptr does not belong to this arena chain.\n");
		return NULL;
	}

	metadata *meta = arena_get_block_metadata(ptr);
	u32 old_lenght = meta->lenght;
	u64 old_capacity = meta->capacity;
	u64 current_total_block_size = meta->block_size;
	u64 needed_total_size = ARENA_ALIGN_UP(new_size + META_SIZE_ALIGNED);

	if (needed_total_size <= current_total_block_size) {
		arena_set_capacity(ptr, (u32)new_size);
		if (meta->lenght > meta->capacity) {
			meta->lenght = meta->capacity;
		}
		return ptr;
	}

	u8 *block_end = (u8 *)meta + current_total_block_size;
	if (block_end == (current_arena->memory + current_arena->offset)) {
		u64 extra_needed = needed_total_size - current_total_block_size;
		if (current_arena->offset + extra_needed <= current_arena->size) {
			current_arena->offset += extra_needed;
			current_arena->space -= extra_needed;
			arena_set_block_size(ptr, needed_total_size);
			arena_set_capacity(ptr, (u32)new_size);
			return ptr;
		}
	}

	if (block_end < (current_arena->memory + current_arena->offset)) {
		metadata *next_meta = (metadata *)block_end;
		if (!next_meta->block_used && (current_total_block_size + next_meta->block_size) >= needed_total_size) {
			u64 combined_size = current_total_block_size + next_meta->block_size;
			
			arena_set_block_size(ptr, combined_size);
			arena_set_capacity(ptr, (u32)new_size);

			if (meta->lenght > meta->capacity) meta->lenght = meta->capacity;
			
			current_arena->space -= next_meta->block_size;
			
			memset(next_meta, 0, sizeof(metadata));
			
			return ptr;
		}
	}

	void *new_ptr = arena_alloc(arena, new_size);
	if (new_ptr) {
		u64 copy_size = (old_capacity < new_size) ? old_capacity : new_size;
		memcpy(new_ptr, ptr, copy_size);
		metadata *new_meta = arena_get_block_metadata(new_ptr);
		new_meta->lenght = (old_lenght < (u32)new_size) ? old_lenght : (u32)new_size;
		arena_free(arena, ptr);
	}
	
	return new_ptr;
}

void
arena_free(Arena *arena, void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: invalid ptr for free.\n");
		return;
	}
	Arena *current_arena = arena_contains_this_ptr(arena, ptr);
	if (!current_arena) {
		fprintf(stderr, "Error: Pointer does not belong to this arena chain.\n");
		return;
	}
	u64 block_size = arena_get_block_size(ptr);
	arena_set_capacity(ptr, 0);
	arena_set_block_used(ptr, false);
	arena_set_lenght(ptr, 0);
	memset(ptr, 0, block_size - META_SIZE_ALIGNED);
	current_arena->space += block_size;
	current_arena->free_count++;
	if (current_arena->free_count == MAX_FREE_COUNT) {
		arena_merge_free_blocks(current_arena);
		current_arena->free_count = 0;
	}
}

void
arena_delete(Arena *arena) {
	while (arena != NULL) {
		Arena *next = arena->child;
		free(arena->memory);
		free(arena);
		arena = next;
	}
}

void
arena_reset(Arena *arena) {
	while (arena != NULL) {
		Arena *next = arena->child;	
		arena->offset = 0;
		arena->space = arena->size;
		arena = next;
	}
}

void
arena_print(Arena *arena, bool content) {
	u64 used_bytes = arena->size - arena->space; // Calcul correct
    f32 free_percent = (arena->space * 100.0) / arena->size;
    f32 used_percent = 100.0 - free_percent;

    printf("|-------------->>>\n");
    printf("| Arena -> %p:\n", arena);
    printf("| Size: %llu\n", arena->size);
    printf("| Free: %llu byte Used: %llu byte\n", arena->space, used_bytes);
    printf("| Free: %.4f%% Used: %.4f%%\n", free_percent, used_percent);
	if (content){
		u64 offset = 0;
		while (offset < arena->offset) {
			void *block_ptr = (u8 *)arena->memory + offset;
			void *data_ptr = (u8 *)block_ptr + META_SIZE_ALIGNED;
			u64 block_size = arena_get_block_size(data_ptr);
			u64 capacity = arena_get_capacity(data_ptr);
			bool free = arena_is_block_free(data_ptr);
			printf("| Block at %p: capacity = %llu, block_size = %llu, block_status = %s, content = ", block_ptr, capacity, block_size, free ? "free" : "used");
			for (u64 i = 0; i < block_size - META_SIZE_ALIGNED; i++) {
				printf("%02x ", ((u8 *)data_ptr)[i]);
			}
			printf("\n");
			offset += block_size;
		}
	}
	printf("|--------------<<<\n\n");
}

void arena_print_child(Arena *arena, bool content) {
	Arena *next = arena;
	arena_print(next, content);
	while (next->child){
		next = next->child;
		arena_print(next, content);
	}
}