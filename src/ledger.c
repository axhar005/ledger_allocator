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

bool
ledger_set_block_metadata(void *ptr, metadata *meta) {
	if (ptr == NULL || meta == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_set_metadata function.\n");
		return false;
	}
	metadata *current_meta = ledger_get_block_metadata(ptr);
	if (current_meta == NULL) {
		return false;
	}
	*current_meta = *meta;
	return true;
}

metadata *
ledger_get_block_metadata(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received.\n");
		return NULL;
	}
	return (metadata *)((u8 *)ptr - META_SIZE_ALIGNED);
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

bool
ledger_set_block_used(void *ptr, bool used) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_set_block_used function.\n");
		return false;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->block_used = used ? 1 : 0;
	return true;
}

bool
ledger_set_block_size(void *ptr, u32 new_size) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_set_block_size function.\n");
		return false;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->block_size = new_size;
	return true;
}

u32
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
ledger_set_capacity(void *ptr, u32 size) {
	if (ptr == NULL) {
		fprintf(stderr,"Error: Null pointer received in ledger_set_capacity function.\n");
		return false;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->capacity = size;
	return true;
}

u32
ledger_get_capacity(void *ptr) {
	if (ptr == NULL) {
		fprintf(stderr, "Error: Null pointer received in ledger_get_capacity function.\n");
		return 0;
	}
	metadata *meta = ledger_get_block_metadata(ptr);
	return meta->capacity;
}

bool
ledger_set_lenght(void *ptr, u32 new_lenght) {
	if (!ptr) return false;
	metadata *meta = ledger_get_block_metadata(ptr);
	if (new_lenght <= meta->capacity) {
		meta->lenght = new_lenght;
	}else{
		fprintf(stderr, "Error: New lenght exceeds block capacity.\n");
		return false;
	}
	return true;
}

u32
ledger_get_lenght(void *ptr) {
	return ptr ? ledger_get_block_metadata(ptr)->lenght : 0;
}

bool
ledger_set_flag(void *ptr, u16 flag) {
	if (!ptr) return false;
	metadata *meta = ledger_get_block_metadata(ptr);
	meta->flag = flag;
	return true;
}

u16
ledger_get_flag(void *ptr) {
	return ptr ? ledger_get_block_metadata(ptr)->flag : 0;
}

void *
ledger_find_flag(Ledger *ledger, void *start_after, u16 flag) {
	if (!ledger) return NULL;

	Ledger *current_page = ledger;
	u64 offset = 0;

	if (start_after) {
		current_page = ledger_contains_this_ptr(ledger, start_after);
		if (!current_page) return NULL;
		
		metadata *meta = ledger_get_block_metadata(start_after);
		offset = ((u8*)start_after - (u8*)current_page->memory - META_SIZE_ALIGNED) + meta->block_size;
	}

	while (current_page) {
		while (offset < current_page->offset) {
			void *data_ptr = current_page->memory + offset + META_SIZE_ALIGNED;
			metadata *meta = ledger_get_block_metadata(data_ptr);

			if (meta->block_used && meta->flag == flag) {
				return data_ptr;
			}
			offset += meta->block_size;
		}
		current_page = current_page->child;
		offset = 0;
	}
	return NULL;
}

u32
ledger_count_flag(Ledger *ledger, u16 flag) {
	u32 count = 0;
	void *entry = NULL;

	while ((entry = ledger_find_flag(ledger, entry, flag))) {
		count++;
	}
	return count;
}

bool
ledger_merge_free_blocks(Ledger *ledger) {
	if (!ledger || ledger->offset == 0) return false;

	bool merged_any = false;
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
			
			merged_any = true;

			continue;
		}
		current_offset += block_size;
	}
	return merged_any;
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

bool
ledger_split_block(Ledger *ledger, void *data_ptr, u32 needed_total_size) {
	if (!ledger || !data_ptr) {
		fprintf(stderr, "Error: Invalid ledger or data_ptr for block split.\n");
		return false;
	}

	if(needed_total_size == 0 || needed_total_size > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: Invalid needed_total_size for block split.\n");
		return false;
	}

	if (!ledger_contains_this_ptr(ledger, data_ptr)) {
		fprintf(stderr, "Error: data_ptr does not belong to the provided ledger.\n");
		return false;
	}

	u32 current_block_size = ledger_get_block_size(data_ptr);
	
	u32 min_split_size = needed_total_size + META_SIZE_ALIGNED + LEDGER_ALIGNMENT;

	if (current_block_size >= min_split_size) {
		u32 remaining_size = current_block_size - needed_total_size;

		ledger_set_block_size(data_ptr, needed_total_size);

		metadata *current_meta = ledger_get_block_metadata(data_ptr);
		u8 *next_block_ptr = (u8 *)current_meta + needed_total_size;
		void *next_data_ptr = next_block_ptr + META_SIZE_ALIGNED;

		ledger_set_block_size(next_data_ptr, remaining_size);
		ledger_set_block_used(next_data_ptr, false);
		ledger_set_capacity(next_data_ptr, 0);
		ledger_set_lenght(next_data_ptr, 0);
		ledger_set_flag(next_data_ptr, 0);

		return true;
	}

	return false;
}

void *
ledger_find_free_block(Ledger *ledger, u32 size) {
	Ledger *current = ledger;

	while (current != NULL) {
		u32 offset = 0;
		while (offset < current->offset) {
			u8 *block_ptr = current->memory + offset;
			void *data_ptr = block_ptr + META_SIZE_ALIGNED;
			u32 block_size = ledger_get_block_size(data_ptr);

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
_ledger_alloc(Ledger *ledger, u32 size, u16 flag, bool clean) {
	if (!ledger) {
		fprintf(stderr, "Error: invalid ledger ptr for aalloc.\n");
		return NULL;
	}
	if (size == 0) {
		fprintf(stderr, "Error: invalid size for aalloc.\n");
		return NULL;
	}

	u32 total_size = LEDGER_ALIGN_UP(size + META_SIZE_ALIGNED);

	if (total_size > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Error: Block size exceeds the maximum allowable size.\n");
		return NULL;
	}

	// child ledger allocation
	if (total_size > ledger->size) {
		if (ledger->child) {
			return _ledger_alloc(ledger->child, size, flag , clean);
		} else {
			ledger->child = ledger_create(total_size);
			if (!ledger->child) return NULL;
			return _ledger_alloc(ledger->child, size, flag, clean);
		}
	}

	void *free_block_ptr = ledger_find_free_block(ledger, total_size);
	if (free_block_ptr) {
		Ledger *target_ledger = ledger_contains_this_ptr(ledger, free_block_ptr);
		if (!target_ledger) return NULL;

		void *free_data_ptr = (u8 *)free_block_ptr + META_SIZE_ALIGNED;
		u32 free_block_size = ledger_get_block_size(free_data_ptr);

		ledger_set_block_used(free_data_ptr, true);
		ledger_set_capacity(free_data_ptr, (u32)size);
		ledger_set_lenght(free_data_ptr, 0);
		ledger_set_flag(free_data_ptr, flag);

		if (ledger_split_block(target_ledger, free_data_ptr, total_size))
			target_ledger->space -= total_size;
		else
			target_ledger->space -= free_block_size;

		if (clean)
			memset(free_data_ptr, 0, free_block_size - META_SIZE_ALIGNED);

		return free_data_ptr;
	}

	u32 current_offset = LEDGER_ALIGN_UP(ledger->offset);

	if (current_offset + total_size > ledger->size) {
		if (ledger->child) {
			return _ledger_alloc(ledger->child, size, flag, clean);
		} else {
			ledger->child = ledger_create(ledger->size * 2);
			if (!ledger->child) return NULL;
			return _ledger_alloc(ledger->child, size, flag, clean);
		}
	}
	
	u8 *block_ptr = ledger->memory + current_offset;
	void *data_ptr = block_ptr + META_SIZE_ALIGNED;
	
	ledger_set_block_size(data_ptr, (u32)total_size);
	ledger_set_block_used(data_ptr, true);
	ledger_set_capacity(data_ptr, (u32)size);
	ledger_set_lenght(data_ptr, 0);
	ledger_set_flag(data_ptr, flag);

	if (clean)
		memset(data_ptr, 0, size);

	ledger->offset = current_offset + total_size;
	ledger->space = ledger->size - ledger->offset;

	return data_ptr;
}

void *
ledger_malloc(Ledger *ledger, u32 size) {
	return _ledger_alloc(ledger, size, LEDGER_FLAG_NONE, false);
}

void *
ledger_calloc(Ledger *ledger, u32 size) {
	return _ledger_alloc(ledger, size, LEDGER_FLAG_NONE, true);
}

void *
ledger_malloc_flag(Ledger *ledger, u32 size, u16 flag) {
	return _ledger_alloc(ledger, size, flag, false);
}

void *
ledger_calloc_flag(Ledger *ledger, u32 size, u16 flag) {
	return _ledger_alloc(ledger, size, flag, true);
}

void *
ledger_realloc(Ledger *ledger, void *ptr, u32 new_size) {
	if (!ledger) return NULL;
	
	if (!ptr) return NULL; // TO-DO mayby return ledger_alloc(ledger, new_size);
	
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
	u32 old_capacity = meta->capacity;
	u32 current_total_block_size = meta->block_size;
	u32 needed_total_size = LEDGER_ALIGN_UP(new_size + META_SIZE_ALIGNED);

	if (needed_total_size <= current_total_block_size) {
		ledger_set_capacity(ptr, (u32)new_size);
		if (meta->lenght > meta->capacity) {
			meta->lenght = meta->capacity;
		}
		return ptr;
	}

	u8 *block_end = (u8 *)meta + current_total_block_size;
	if (block_end == (current_ledger->memory + current_ledger->offset)) {
		u32 extra_needed = needed_total_size - current_total_block_size;
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
			
			u32 combined_size = current_total_block_size + next_meta->block_size;
			
			ledger_set_block_size(ptr, combined_size);

			if (ledger_split_block(current_ledger, ptr, needed_total_size)) {
				current_ledger->space -= (needed_total_size - current_total_block_size);
			} else {
				current_ledger->space -= next_meta->block_size;
			}

			u32 block_growth = ledger_get_block_size(ptr) - current_total_block_size;
			memset(block_end, 0, block_growth);
			ledger_set_capacity(ptr, (u32)new_size);

			if (meta->lenght > meta->capacity) meta->lenght = meta->capacity;
			
			return ptr;
		}
	}

	u16 current_flag = ledger_get_flag(ptr);
	void *new_ptr = _ledger_alloc(ledger, new_size, current_flag, false);
	if (new_ptr) {
		u32 copy_size = (old_capacity < new_size) ? old_capacity : new_size;
		memcpy(new_ptr, ptr, copy_size);
		metadata *new_meta = ledger_get_block_metadata(new_ptr);
		new_meta->lenght = (old_lenght < (u32)new_size) ? old_lenght : (u32)new_size;
		ledger_free(ledger, ptr);
	}
	
	return new_ptr;
}

bool
ledger_free(Ledger *ledger, void *ptr) {
	if (!ledger) {
		fprintf(stderr, "Error: invalid ledger ptr for free.\n");
		return false;
	}

	if (ptr == NULL) {
		return true;
	}

	Ledger *current_ledger = ledger_contains_this_ptr(ledger, ptr);
	if (!current_ledger) {
		fprintf(stderr, "Error: Pointer does not belong to this ledger chain.\n");
		return false;
	}

	if (ledger_is_block_free(ptr)) {
		fprintf(stderr, "Warning: Double free detected at %p. Operation ignored.\n", ptr);
		return false; 
	}

	u32 block_size = ledger_get_block_size(ptr);
	ledger_set_capacity(ptr, 0);
	ledger_set_block_used(ptr, false);
	ledger_set_lenght(ptr, 0);
	ledger_set_flag(ptr, 0);
	current_ledger->space += block_size;
	current_ledger->free_count++;
	if (current_ledger->free_count == MAX_FREE_COUNT) {
		ledger_merge_free_blocks(current_ledger);
		current_ledger->free_count = 0;
	}
	return true;
}

bool
ledger_delete(Ledger *ledger) {
	if (!ledger) return false;

	while (ledger != NULL) {
		Ledger *next = ledger->child;
		free(ledger->memory);
		free(ledger);
		ledger = next;
	}
	return true;
}

bool
ledger_reset(Ledger *ledger) {
	if (!ledger) return false;

	Ledger *current = ledger;
	while (current != NULL) {
		current->offset = 0;
		current->space = current->size;
		current->free_count = 0;

		if (current->memory)
			memset(current->memory, 0, current->size);

		current = current->child;
	}

	return true;
}

void
ledger_print(Ledger *ledger, bool content) {
	u64 used_bytes = ledger->size - ledger->space;
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

void
ledger_print_child(Ledger *ledger, bool content) {
	Ledger *next = ledger;
	ledger_print(next, content);
	while (next->child){
		next = next->child;
		ledger_print(next, content);
	}
}

void
ledger_dump(Ledger *ledger) {
	if (!ledger) return;

	Ledger *current = ledger;
	int depth = 0;

	while (current) {
		if (depth == 0) {
			printf("\n==================== LEDGER: MASTER (@%p) ====================\n", (void*)current);
		} else {
			printf("\n==================== LEDGER: CHILD %d (@%p) ====================\n", depth, (void*)current);
		}
		
		// Colonne FLAG en format décimal
		printf(" STAT |  CAP  |  B_SZ |  LEN  |  FLAG | ADDRESS          | DATA (Hex Preview)\n");
		printf("------+-------+-------+-------+-------+------------------+-------------------------\n");

		u64 current_offset = 0;
		while (current_offset < current->offset) {
			u8 *block_base = current->memory + current_offset;
			void *data_ptr = block_base + META_SIZE_ALIGNED;
			
			metadata *meta = ledger_get_block_metadata(data_ptr);
			if (!meta || meta->block_size == 0) break;

			bool is_free = (meta->block_used == 0);

			// %5u permet de garder l'alignement pour des valeurs allant jusqu'à 65535
			printf("  [%c] | %5u | %5u | %5u | %5u | %p | ", 
				is_free ? 'F' : 'U', 
				meta->capacity, 
				meta->block_size, 
				meta->lenght,
				meta->flag,
				data_ptr);

			u8 *d = (u8 *)data_ptr;
			u32 data_limit = meta->block_size - (u32)META_SIZE_ALIGNED;
			for (u32 i = 0; i < 12; i++) {
				if (i < data_limit) {
					printf("%02x ", d[i]);
				} else {
					printf("   ");
				}
			}
			printf("\n");

			current_offset += meta->block_size;
		}

		f32 usage_pct = (f32)(current->size - current->space) / (f32)current->size * 100.0f;
		printf("------+-------+-------+-------+-------+------------------+-------------------------\n");
		printf(" PAGE USAGE: %6.2f%% | SIZE: %7llu | SPACE: %7llu\n", 
			usage_pct, current->size, current->space);

		if (current->child) {
			printf("\n          ||\n          \\/ (Next Linked Page)\n");
		}
		
		current = current->child;
		depth++;
	}
	printf("=====================================================================\n\n");
}