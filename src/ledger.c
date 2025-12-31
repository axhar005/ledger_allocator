/**
* Ledger Allocator for Memory Management.
* 
* This file implements an ledger allocator that allows efficient allocation and
* deallocation of memory blocks. The allocator uses a primary ledger and can
* create child ledgers in case of overflow. Allocated blocks are tracked using
* a metadata struct containing information about the block size, alloc size
* and status (free or used).
*/

#include "../include/ledger.h"
#include <stdio.h>
#include <string.h>

Ledger *
ledger_contains_this_ptr(Ledger *ledger, void *data_ptr) {
	Ledger *current_ledger = ledger;
	u8 *ptr = (u8 *)data_ptr;
	while (current_ledger) {
		if (ptr >= current_ledger->memory && ptr < (current_ledger->memory + current_ledger->size)) {
			return current_ledger;
		}
		current_ledger = current_ledger->child;
	}
	return NULL;
}

metadata *
ledger_get_block_metadata(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received.\n");
		return NULL;
	}
	return (metadata *)((u8 *)ptr - META_SIZE_ALIGNED);
}

u64
ledger_get_block_size(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_get_block_size function.\n");
		return 0;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	if (meta->block_size == 0) {
		fprintf(stderr, "Warning: Block size is zero.\n");
	}
	return meta->block_size;
}

bool
ledger_is_block_free(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_is_block_free function.\n");
		return false;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	return meta->block_used == 0;
}

void
ledger_set_block_used(void *ptr, bool used) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_set_block_used function.\n");
		return;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->block_used = used ? 1 : 0;
}

void
ledger_set_block_size(void *ptr, u64 new_size) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_set_block_size function.\n");
		return;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->block_size = new_size;
}

void
ledger_set_capacity(void *ptr, u32 size) {
	if (ptr == NULL) {
		fprintf(stderr,"Error: Null pointer received in ledger_set_capacity function.\n");
		return;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->capacity = size;
}

u64
ledger_get_capacity(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_get_capacity function.\n");
		return 0;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	return meta->capacity;
}

void
ledger_set_lenght(void *ptr, u32 new_lenght) {
	if (!ptr) return;
	metadata *meta = ledger_get_block_metadata(ptr);
	if (new_lenght <= meta->capacity) {
		meta->lenght = new_lenght;
	}
}

u32
ledger_get_lenght(void *ptr) {
	return ptr ? ledger_get_block_metadata(ptr)->lenght : 0;
}


void
ledger_merge_free_blocks(Ledger *ledger) {
	if (!ledger || ledger->offset == 0) return;

	u64 current_offset = 0;
	while (current_offset < ledger->offset) {
		u8 *block_ptr = ledger->memory + current_offset;
		void *data_ptr = block_ptr + META_SIZE_ALIGNED;
		u64 block_size = ledger_get_block_size(data_ptr);

		if (current_offset + block_size >= ledger->offset) {
			break;
		}

		u8 *next_block_ptr = block_ptr + block_size;
		void *next_data_ptr = next_block_ptr + META_SIZE_ALIGNED;

		if (ledger_is_block_free(data_ptr) && ledger_is_block_free(next_data_ptr)) {
			u64 next_block_size = ledger_get_block_size(next_data_ptr);
			u64 new_merged_size = block_size + next_block_size;
			
			ledger_set_block_size(data_ptr, new_merged_size);

			metadata *meta = ledger_get_block_metadata(data_ptr);
			meta->capacity = 0;
			meta->lenght = 0;
			
			memset(next_block_ptr, 0, META_SIZE_ALIGNED);
			
			continue;
		}
		current_offset += block_size;
	}
}

Ledger *
ledger_create(u64 size) {
	if (size == 0 || size >= MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: invalid ledger size.\n");
		return NULL;
	}

	Ledger *ledger = (Ledger *)malloc(sizeof(Ledger));
	if (!ledger) {
		fprintf(stderr, "Error: Failed to allocate ledger. Reason: %s\n", strerror(errno));
		return NULL;
	}

	size = LEDGER_ALIGN_UP(size);

	ledger->memory = (u8 *)calloc(1, size); 
	
	if (!ledger->memory) {
		fprintf(stderr, "Error: Failed to allocate ledger memory. Reason: %s\n", strerror(errno));
		free(ledger);
		return NULL;
	}

	uintptr_t addr = (uintptr_t)ledger->memory;
	if (addr % LEDGER_ALIGNMENT != 0) {
		fprintf(stderr, "Error: Memory alignment failure.\n");
		free(ledger->memory);
		free(ledger);
		return NULL;
	}

	ledger->size = size;
	ledger->offset = 0;
	ledger->space = size;
	ledger->child = NULL;
	ledger->free_count = 0;

	return ledger;
}

void *
ledger_find_free_block(Ledger *ledger, u64 size) {
	Ledger *current = ledger;

	while (current != NULL) {
		u64 offset = 0;
		while (offset < current->offset) {
			u8 *block_ptr = current->memory + offset;
			void *data_ptr = block_ptr + META_SIZE_ALIGNED;
			u64 block_size = ledger_get_block_size(data_ptr);

			if (ledger_is_block_free(data_ptr) && block_size >= size) {
				return block_ptr; 
			}

			offset += block_size;
		}
		current = current->child;
	}

	return NULL;
}

void *
ledger_alloc(Ledger *ledger, u64 size) {
	if (!ledger) {
		fprintf(stderr, "Error: invalid ledger ptr for aalloc.\n");
		return NULL;
	}
	if (size == 0) {
		fprintf(stderr, "Error: invalid size for aalloc.\n");
		return NULL;
	}

	u64 total_size = LEDGER_ALIGN_UP(size + META_SIZE_ALIGNED);

	if (total_size > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: Block size exceeds the maximum allowable size.\n");
		return NULL;
	}

	if (total_size > ledger->size) {
		if (ledger->child) {
			return ledger_alloc(ledger->child, size);
		} else {
			ledger->child = ledger_create(total_size);
			if (!ledger->child) return NULL;
			return ledger_alloc(ledger->child, size);
		}
	}

	void *free_block_ptr = ledger_find_free_block(ledger, total_size);
	if (free_block_ptr) {
		Ledger *target_ledger = ledger_contains_this_ptr(ledger, free_block_ptr);
		if (!target_ledger) return NULL;
		void *free_data_ptr = (u8 *)free_block_ptr + META_SIZE_ALIGNED;
		u64 free_block_size = ledger_get_block_size(free_data_ptr);

		if (free_block_size >= total_size + (META_SIZE_ALIGNED + LEDGER_ALIGNMENT)) {
			ledger_set_block_size(free_data_ptr, total_size);
			ledger_set_block_used(free_data_ptr, true);
			ledger_set_capacity(free_data_ptr, (u32)size);
			ledger_set_lenght(free_data_ptr, 0);

			u8 *next_block_ptr = (u8 *)free_block_ptr + total_size;
			void *next_data_ptr = next_block_ptr + META_SIZE_ALIGNED;
			ledger_set_block_size(next_data_ptr, (u32)(free_block_size - total_size));
			ledger_set_block_used(next_data_ptr, false);
			ledger_set_capacity(next_data_ptr, 0);
			ledger_set_lenght(next_data_ptr, 0);

			target_ledger->space -= total_size;
		} else {
			ledger_set_block_used(free_data_ptr, true);
			ledger_set_capacity(free_data_ptr, (u32)size);
			target_ledger->space -= free_block_size;
		}
		return free_data_ptr;
	}

	u64 current_offset = LEDGER_ALIGN_UP(ledger->offset);

	if (current_offset + total_size > ledger->size) {
		if (ledger->child) {
			return ledger_alloc(ledger->child, size);
		} else {
			ledger->child = ledger_create(ledger->size * 2);
			if (!ledger->child) return NULL;
			return ledger_alloc(ledger->child, size);
		}
	}

	u8 *block_ptr = ledger->memory + current_offset;
	void *data_ptr = block_ptr + META_SIZE_ALIGNED;
	
	ledger_set_block_size(data_ptr, (u32)total_size);
	ledger_set_block_used(data_ptr, true);
	ledger_set_capacity(data_ptr, (u32)size);
	ledger_set_lenght(data_ptr, 0);

	ledger->offset = current_offset + total_size;
	ledger->space = ledger->size - ledger->offset;

	return data_ptr;
}

void *
ledger_realloc(Ledger *ledger, void *ptr, u64 new_size) {
	if (!ledger) return NULL;
	
	if (!ptr) return ledger_alloc(ledger, new_size);
	
	if (new_size == 0) {
		ledger_free(ledger, ptr);
		return NULL;
	}

	Ledger *current_ledger = ledger_contains_this_ptr(ledger, ptr);
	if (!current_ledger) {
		fprintf(stderr, "Error: ptr does not belong to this ledger chain.\n");
		return NULL;
	}

	metadata *meta = ledger_get_block_metadata(ptr);
	u32 old_lenght = meta->lenght;
	u64 old_capacity = meta->capacity;
	u64 current_total_block_size = meta->block_size;
	u64 needed_total_size = LEDGER_ALIGN_UP(new_size + META_SIZE_ALIGNED);

	if (needed_total_size <= current_total_block_size) {
		ledger_set_capacity(ptr, (u32)new_size);
		if (meta->lenght > meta->capacity) {
			meta->lenght = meta->capacity;
		}
		return ptr;
	}

	u8 *block_end = (u8 *)meta + current_total_block_size;
	if (block_end == (current_ledger->memory + current_ledger->offset)) {
		u64 extra_needed = needed_total_size - current_total_block_size;
		if (current_ledger->offset + extra_needed <= current_ledger->size) {
			current_ledger->offset += extra_needed;
			current_ledger->space -= extra_needed;
			ledger_set_block_size(ptr, needed_total_size);
			ledger_set_capacity(ptr, (u32)new_size);
			return ptr;
		}
	}

	if (block_end < (current_ledger->memory + current_ledger->offset)) {
		metadata *next_meta = (metadata *)block_end;
		if (!next_meta->block_used && (current_total_block_size + next_meta->block_size) >= needed_total_size) {
			u64 combined_size = current_total_block_size + next_meta->block_size;
			
			ledger_set_block_size(ptr, combined_size);
			ledger_set_capacity(ptr, (u32)new_size);

			if (meta->lenght > meta->capacity) meta->lenght = meta->capacity;
			
			current_ledger->space -= next_meta->block_size;
			
			memset(next_meta, 0, sizeof(metadata));
			
			return ptr;
		}
	}

	void *new_ptr = ledger_alloc(ledger, new_size);
	if (new_ptr) {
		u64 copy_size = (old_capacity < new_size) ? old_capacity : new_size;
		memcpy(new_ptr, ptr, copy_size);
		metadata *new_meta = ledger_get_block_metadata(new_ptr);
		new_meta->lenght = (old_lenght < (u32)new_size) ? old_lenght : (u32)new_size;
		ledger_free(ledger, ptr);
	}
	
	return new_ptr;
}

void
ledger_free(Ledger *ledger, void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: invalid ptr for free.\n");
		return;
	}
	Ledger *current_ledger = ledger_contains_this_ptr(ledger, ptr);
	if (!current_ledger) {
		fprintf(stderr, "Error: Pointer does not belong to this ledger chain.\n");
		return;
	}
	u64 block_size = ledger_get_block_size(ptr);
	ledger_set_capacity(ptr, 0);
	ledger_set_block_used(ptr, false);
	ledger_set_lenght(ptr, 0);
	memset(ptr, 0, block_size - META_SIZE_ALIGNED);
	current_ledger->space += block_size;
	current_ledger->free_count++;
	if (current_ledger->free_count == MAX_FREE_COUNT) {
		ledger_merge_free_blocks(current_ledger);
		current_ledger->free_count = 0;
	}
}

void
ledger_delete(Ledger *ledger) {
	while (ledger != NULL) {
		Ledger *next = ledger->child;
		free(ledger->memory);
		free(ledger);
		ledger = next;
	}
}

void
ledger_reset(Ledger *ledger) {
	while (ledger != NULL) {
		Ledger *next = ledger->child;	
		ledger->offset = 0;
		ledger->space = ledger->size;
		ledger = next;
	}
}

void
ledger_print(Ledger *ledger, bool content) {
	u64 used_bytes = ledger->size - ledger->space; // Calcul correct
	f32 free_percent = (ledger->space * 100.0) / ledger->size;
	f32 used_percent = 100.0 - free_percent;

	printf("|-------------->>>\n");
	printf("| Ledger -> %p:\n", ledger);
	printf("| Size: %llu\n", ledger->size);
	printf("| Free: %llu byte Used: %llu byte\n", ledger->space, used_bytes);
	printf("| Free: %.4f%% Used: %.4f%%\n", free_percent, used_percent);
	if (content){
		u64 offset = 0;
		while (offset < ledger->offset) {
			void *block_ptr = (u8 *)ledger->memory + offset;
			void *data_ptr = (u8 *)block_ptr + META_SIZE_ALIGNED;
			u64 block_size = ledger_get_block_size(data_ptr);
			u64 capacity = ledger_get_capacity(data_ptr);
			bool free = ledger_is_block_free(data_ptr);
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

void ledger_print_child(Ledger *ledger, bool content) {
	Ledger *next = ledger;
	ledger_print(next, content);
	while (next->child){
		next = next->child;
		ledger_print(next, content);
	}
}