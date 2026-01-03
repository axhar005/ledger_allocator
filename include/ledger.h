#ifndef LEDGER_H
# define LEDGER_H

# include <stdint.h>
# include <stddef.h>
# include <stdlib.h>
# include <stdalign.h>
# include <assert.h>
# include <stdbool.h>
# include <errno.h>
# include <limits.h>

# define LEDGER_SIZE 1024 * 1024 * 1															// 1 MB

# define LEDGER_ALIGNMENT 16																		// 8 or 16 bytes

# define LEDGER_ALIGN_UP(size) (((size) + ((LEDGER_ALIGNMENT) - 1)) & ~((LEDGER_ALIGNMENT) - 1))	// Align size up to the nearest multiple of LEDGER_ALIGNMENT

# define META_SIZE_ALIGNED LEDGER_ALIGN_UP(sizeof(metadata)) 									// Aligned size of metadata

# define MAX_FREE_COUNT 10																		// Max free count before ledger_merge_free_blocks()

# define MAX_BLOCK_SIZE UINT32_MAX																// Max unsigned int 32 bits size


// Data types
typedef uint8_t		u8;
typedef uint16_t 	u16;
typedef uint32_t 	u32;
typedef uint64_t 	u64;

typedef int8_t		i8;
typedef int16_t 	i16;
typedef int32_t 	i32;
typedef int64_t 	i64;

typedef float		f32;
typedef double		f64;

// Structure of the ledger
typedef struct ledger_t {
	u8 				*memory;	// Pointer to the allocated memory for the ledger
	u64 			space;		// Remaining free space in the ledger
	u64 			size;		// Total size of the ledger
	u64 			offset;		// Current offset in the ledger
	u8				free_count;	// Counts frees and merges blocks when MAX_FREE_COUNT is reached
	struct ledger_t	*child;		// Pointer to a child ledger in case of overflow
} Ledger;

// Metadata structure for each allocated block
typedef struct {
	u32 block_used; 			// Block status indicator: 1 if the block is used, 0 if free
	u32 capacity; 				// Size of the data actually allocated by the user
	u32 block_size; 			// Total size of the block, including the alignment
	u32 lenght; 					// Current length of data stored in the block
} metadata;

/**
 * Creates a new ledger with the specified size.
 * @param size The size of the ledger in bytes.
 * @return A pointer to the created ledger.
 */
Ledger* ledger_create(u64 size);

/**
 * Allocates a block of memory of the specified size in the ledger.
 * @param ledger A pointer to the ledger.
 * @param size The size of the block to allocate in bytes.
 * @return A pointer to the allocated block.
 */
void* ledger_alloc(Ledger *ledger, u32 size);

/**
 * Reallocates a previously allocated block to a new size.
 * @param ledger A pointer to the ledger.
 * @param ptr A pointer to the previously allocated block.
 * @param new_size The new size for the block in bytes.
 * @return A pointer to the reallocated block.
 */
void* ledger_realloc(Ledger *ledger, void *ptr, u32 new_size);

/**
 * Frees a previously allocated block of memory.
 * @param ptr A pointer to the block to free.
 */
void ledger_free(Ledger *ledger, void *ptr);

/**
 * Resets the ledger, freeing all allocations made.
 * @param ledger A pointer to the ledger.
 */
void ledger_reset(Ledger *ledger);

/**
 * Deletes an ledger and frees all associated memory.
 * @param ledger A pointer to the ledger to delete.
 */
void ledger_delete(Ledger *ledger);

/**
 * Checks if a given pointer belongs to the ledger or its child ledgers.
 * @param ledger A pointer to the ledger.
 * @param data_ptr A pointer to check.
 * @return A pointer to the ledger containing the pointer, or NULL if not found.
 */
Ledger *ledger_contains_this_ptr(Ledger *ledger, void *data_ptr);

/**
 * Finds a free block of sufficient size in the ledger.
 * @param ledger A pointer to the ledger.
 * @param size The size of the block to find in bytes.
 * @return A pointer to the found free block, or NULL if no free block is found.
 */
void* ledger_find_free_block(Ledger *ledger, u32 size);

/**
 * Merges adjacent free blocks in the ledger into a single larger block.
 * @param ledger A pointer to the ledger.
 */
void ledger_merge_free_blocks(Ledger *ledger);

/**
 * Retrieves the metadata structure for a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @return A pointer to the metadata associated with the block.
 */
metadata *ledger_get_block_metadata(void *ptr);

/**
 * Checks if a block of memory is free.
 * @param ptr A pointer to the data portion of the memory block to check.
 * @return true if the block is free, false otherwise.
 */
bool ledger_is_block_free(void *ptr);

/**
 * Sets the used status of a block in the ledger.
 * @param ptr A pointer to the data portion of the memory block.
 * @param used A boolean indicating whether the block should be marked as used (true) or free (false).
 */
void ledger_set_block_used(void *ptr, bool used);

/**
 * Sets the size of a memory block while preserving its status.
 * @param ptr A pointer to the data portion of the memory block.
 * @param new_size The new size to assign to the block.
 */
void ledger_set_block_size(void *ptr, u32 new_size);

/**
 * Gets the size of an allocated block of memory.
 * @param ptr A pointer to the data portion of the memory block
 * @return The size of the block in bytes.
 */
u32 ledger_get_block_size(void *ptr);

/**
 * Sets the data size of a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @param size The new data size to assign.
 */
void ledger_set_capacity(void *ptr, u32 size);

/**
 * Retrieves the data size of a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @return The size of the data portion as an unsigned 64-bit integer.
 */
u32 ledger_get_capacity(void *ptr);

/**
 * Sets the current length of data stored in the block.
 * @param ptr A pointer to the data portion of the memory block.
 * @param lenght The new length to assign.
 */
void ledger_set_lenght(void *ptr, u32 lenght);

/**
 * Retrieves the current length of data stored in the block.
 * @param ptr A pointer to the data portion of the memory block.
 * @return The current length of data as an unsigned 32-bit integer.
 */
u32 ledger_get_lenght(void *ptr);

/**
 * Prints the current state of the ledger, including the proportion of free and used memory.
 * @param ledger A pointer to the ledger.
 * @param content An indicator to print or not the content of the blocks.
 */
void ledger_print(Ledger *ledger, bool content);

/**
 * Prints the current state of the ledger and child, including the proportion of free and used memory.
 * @param ledger A pointer to the ledger.
 * @param content An indicator to print or not the content of the blocks.
 */
void ledger_print_child(Ledger *ledger, bool content);

/**
 * Dumps the entire ledger structure, including all blocks and their metadata.
 * @param ledger A pointer to the ledger.
 */
void ledger_dump(Ledger *ledger);

#endif