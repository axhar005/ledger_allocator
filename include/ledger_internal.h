#ifndef LEDGER_INTERNAL_H
#define LEDGER_INTERNAL_H

#include "ledger.h"


/**
 * Splits a block into two if it is larger than the needed size.
 * @param ledger A pointer to the ledger.
 * @param data_ptr A pointer to the data portion of the memory block to split.
 * @param needed_total_size The size required for the first block after splitting.
 * @return true if the block was split, false otherwise.
 */
bool _ledger_split_block(Ledger *ledger, void *data_ptr, u32 needed_total_size);

/**
 * Allocates a block of memory of the specified size in the ledger.
 * @param ledger A pointer to the ledger.
 * @param size The size of the block to allocate in bytes.
 * @return A pointer to the allocated block.
 */
void* _ledger_alloc(Ledger *ledger, u32 size, u16 flag, bool clean);

/**
 * Checks if a given pointer belongs to the ledger or its child ledgers.
 * @param ledger A pointer to the ledger.
 * @param data_ptr A pointer to check.
 * @return A pointer to the ledger containing the pointer, or NULL if not found.
 */
Ledger* _ledger_contains_this_ptr(Ledger *ledger, void *data_ptr);

/**
 * Finds a free block of sufficient size in the ledger.
 * @param ledger A pointer to the ledger.
 * @param size The size of the block to find in bytes.
 * @return A pointer to the found free block, or NULL if no free block is found.
 */
void* _ledger_find_free_block(Ledger *ledger, u32 size);

/**
 * Merges adjacent free blocks in the ledger into a single larger block.
 * @param ledger A pointer to the ledger.
 * @return true if any blocks were merged, false otherwise.
 */
bool _ledger_merge_free_blocks(Ledger *ledger);

/**
 * Retrieves the metadata structure for a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @return A pointer to the metadata associated with the block.
 */
metadata *_ledger_get_block_metadata(void *ptr);

/**
 * Sets the metadata structure for a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @param meta A pointer to the metadata to set.
 * @return true if the operation was successful, false otherwise.
 */
bool _ledger_set_block_metadata(void *ptr, metadata *meta);

/**
 * Checks if a block of memory is free.
 * @param ptr A pointer to the data portion of the memory block to check.
 * @return true if the block is free, false otherwise.
 */
bool _ledger_is_block_free(void *ptr);

/**
 * Sets the used status of a block in the ledger.
 * @param ptr A pointer to the data portion of the memory block.
 * @param used A boolean indicating whether the block should be marked as used (true) or free (false).
 * @return true if the operation was successful, false otherwise.
 */
bool _ledger_set_block_used(void *ptr, bool used);

/**
 * Sets the size of a memory block while preserving its status.
 * @param ptr A pointer to the data portion of the memory block.
 * @param new_size The new size to assign to the block.
 * @return true if the operation was successful, false otherwise.
 */
bool _ledger_set_block_size(void *ptr, u32 new_size);

/**
 * Sets the data size of a memory block.
 * @param ptr A pointer to the data portion of the memory block.
 * @param size The new data size to assign.
 * @return true if the operation was successful, false otherwise.
 */
bool _ledger_set_capacity(void *ptr, u32 size);

#endif