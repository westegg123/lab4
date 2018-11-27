/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "shell.h"
#include "cache.h"
#include "helper.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


// Number of sets, then associativity, then block size in bytes.
cache_t *cache_new(int aSets, int aWays, int aBlock) {
	cache_t *myCache = calloc(1, sizeof(cache_t));
	myCache->sets = aSets;
	myCache->ways = aWays;
	myCache->block = aBlock;
	myCache->cache_sets = calloc(aSets, sizeof(cache_set_t));

	cache_set_t *myCacheSets = myCache->cache_sets;
	for (int i = 0; i < aSets; i++) {
		myCacheSets[i].cache_blocks = calloc(aWays, sizeof(cache_block_t));
		for (int j = 0; j < aWays; j++) {
			myCacheSets[i].cache_blocks[j].written_data = calloc(8, sizeof(uint32_t));
			myCacheSets[i].cache_blocks[j].last_used_iteration = 0;
			myCacheSets[i].cache_blocks[j].tag = -1;
			myCacheSets[i].cache_blocks[j].dirty_bit = 0;
		}
	}
	return myCache;
}

cache_t *instruction_cache_new() {
	return cache_new(64, 4, 32);
}

cache_t *data_cache_new() {
	return cache_new(256, 8, 32);
}

int get_instruction_cache_block_offset(uint32_t aInstruction) {
	return (int)(get_instruction_segment(0, 4, aInstruction) / 4);
	//return (int)get_instruction_segment(0, 4, aInstruction);
}

int get_instruction_cache_set_index(uint32_t aInstruction) {
	return (int)get_instruction_segment(5, 10, aInstruction);
}

uint64_t get_instruction_cache_tag(uint32_t aInstruction) {
	return get_instruction_segment(11, 31, aInstruction);
}

int get_data_cache_block_offset(uint64_t aMemoryLoc) {
	return (int)get_instruction_segment(0, 4, aMemoryLoc);
}

int get_data_cache_set_index(uint64_t aMemoryLoc) {
	return (int)get_instruction_segment(5, 12, aMemoryLoc);
}

uint64_t get_data_cache_tag(uint64_t aMemoryLoc) {
	return get_instruction_segment(13, 63, aMemoryLoc);
}

// Get the Address that points to the that set and tag with offset 0 for instruction cache
uint64_t get_OriginAddr_IC(uint64_t aTag, int aSetIndex) {
	return (uint64_t)((aTag << 11) | (aSetIndex << 5));
}

// Get the Address that points to the that set and tag with offset 0 for Data Cache
uint64_t get_OriginAddr_DC(uint64_t aTag, int aSetIndex) {
	return (uint64_t)((aTag << 13) | ((uint64_t)(aSetIndex << 5)));
}

// Get the Data from a Cache Block
uint32_t get_specific_data_from_block(cache_block_t *aCacheBlock, int aBlockOffset) {
	return (uint32_t)((aCacheBlock->written_data)[aBlockOffset]);
}

void write_specific_data_to_block(cache_block_t *aCacheBlock, int aBlockOffset, uint32_t data) {
	(aCacheBlock->written_data)[aBlockOffset] = data;
}

// This function gives you the value of the new set
int get_next_set(int aSet) {
	if (aSet == 255) {
		aSet = 0;	
	} else {
		aSet += 1;
	}
	return aSet;
}


// Function that allows visualization of cache
void print_cache(cache_t *aCache) {
	int sets = aCache->sets;
	int ways = aCache->ways;
	int block = aCache->block;
	printf("NUM SETS: %d\n", sets);
	printf("NUM WAYS: %d\n", ways);
	for (int i = 0; i < sets; i++) {
		printf("CACHE SET %d:\n", i+1);
		for (int j = 0; j < ways; j++) {
			printf("ASSOCIATION %d:\n", j+1);
			// printf("LAST USED ITERATION OF BLOCK: %d\n", aCache->cache_sets[i].cache_blocks[j].last_used_iteration);
			for (int k = 0; k < 8; k++) {
			// for (int k = 0; k < 32; k++) {
				printf("VALUE AT BLOCK OFFSET %d: ", k+1);
				printf("%x\n", aCache->cache_sets[i].cache_blocks[j].written_data[k]);
			}
		}
		printf("\n\n");
	}
}

void print_block(cache_block_t *aBlock) {
	for (int k = 0; k < 8; k++) {
		printf("VALUE AT BLOCK OFFSET %d: ", k+1);
		printf("%x\n", aBlock->written_data[k]);
	}
}

cache_block_t *least_recently_used_block(cache_set_t *aCacheSet, int aWays) {
	cache_block_t *myLeastRecentlyUsedBlock = NULL;
	int myCurrentMin = INT_MAX;
	for (int i = 0; i < aWays; i++) {
		if (aCacheSet->cache_blocks[i].last_used_iteration < myCurrentMin) {
			myCurrentMin = aCacheSet->cache_blocks[i].last_used_iteration;
			myLeastRecentlyUsedBlock = &(aCacheSet->cache_blocks[i]);
		}
	}
	// printf("LEAST RECENTLY USED BLOCK WAS USED  ON ITERATION %d\n", myLeastRecentlyUsedBlock->last_used_iteration);
	return myLeastRecentlyUsedBlock;
}

void cache_destroy(cache_t *aCache) {
	// Maybe consider memset to 0, but I don't think we even need to worry about this function.
	int mySets = aCache->sets;
	int myWays = aCache->ways;

	for (int s = 0; s < mySets; s++) {
		cache_set_t myCacheSet = (aCache->cache_sets)[s]; 
		for (int w = 0; w < myWays; w++) {
			cache_block_t myCacheBlock = (myCacheSet.cache_blocks)[w];
			free(myCacheBlock.written_data);
		}
		free((aCache->cache_sets[s]).cache_blocks);
	}

	free(aCache->cache_sets);
	free(aCache);
}

/********************* FOR INSTRUCTION CACHES ***************************/
cache_block_t *get_tag_match(cache_set_t *aCacheSet, uint64_t aTag, int aAssociativity) {
	cache_block_t *myTagMatch = NULL;
	for (int i = 0; i < aAssociativity; i++) {
		if (aCacheSet->cache_blocks[i].tag == aTag) {
			myTagMatch = &(aCacheSet->cache_blocks[i]);
		}
	}

	// if (!myTagMatch) {
	// 	printf("NO TAG MATCH FOUND!\n");
	// }
	return myTagMatch;
}

// void load_cache_block(cache_block_t *aCacheBlock, uint32_t aAddr) {
// 	int offset = 0;
// 	for (int i = 0; i < 8; i++) {
// 		aCacheBlock->written_data[i] = (aAddr + offset);
// 		offset += 4;
// 	}
// }

void load_cache_block_IC(cache_block_t *aCacheBlock, uint64_t aTag, uint32_t aSet) {
	int offset = 0;
	uint64_t myOriginAddr = get_OriginAddr_IC(aTag, aSet);
	for (int i = 0; i < 8; i++) {
		(aCacheBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
		offset += 4;
	}
}

// int cache_update(cache_t *aInstructionCache, uint64_t addr) {
// 	int myBlockOffset = get_instruction_cache_block_offset(aAddr);
// 	int mySetIndex = get_instruction_cache_set_index(aAddr);
// 	uint32_t myTag = get_instruction_cache_tag(aAddr);

// 	cache_set_t *myCacheSet = &(aInstructionCache->cache_sets[mySetIndex]);
// 	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
// 	// If it is null, there has been no match, and we need to load in a block
// 	if (myCacheBlock == NULL) {
// 		printf("TAG MATCH IS NULL!\n");
// 		myCacheBlock = least_recently_used_block(myCacheSet, aInstructionCache->ways);
// 		load_cache_block(myCacheBlock, aAddr);
// 		myCacheBlock->last_used_iteration = stat_cycles;
// 		myCacheBlock->tag = myTag;
// 		return 1;
// 	} else {
// 		myCacheBlock->last_used_iteration = stat_cycles;
// 		return 0;
// 	}
// }


int cache_update(cache_t *aInstructionCache, uint64_t aAddr) {
	int myBlockOffset = get_instruction_cache_block_offset(aAddr);
	int mySetIndex = get_instruction_cache_set_index(aAddr);
	uint64_t myTag = get_instruction_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aInstructionCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
	// If it is null, there has been no match, and we need to load in a block
	if (myCacheBlock == NULL) {
		myCacheBlock = least_recently_used_block(myCacheSet, aInstructionCache->ways);
		load_cache_block_IC(myCacheBlock, myTag, mySetIndex);
		myCacheBlock->last_used_iteration = stat_cycles;
		myCacheBlock->tag = myTag;
		return 1;
	} else {
		myCacheBlock->last_used_iteration = stat_cycles;
		return 0;
	}
}

// checks if the data is in the isntruction cache
int check_data_in_cache_IC(cache_t *aDataCache, uint64_t aAddr) {
	int myBlockOffset = get_instruction_cache_block_offset(aAddr);
	int mySetIndex = get_instruction_cache_set_index(aAddr);
	uint64_t myTag = get_instruction_cache_tag(aAddr);

	//printf("This is the set: %x and Tag: %x and offset: %x\n", mySetIndex, myTag, myBlockOffset);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if (myCacheBlock == NULL) {
		return 1;
	} else {
		return 0;
	}
}

uint32_t get_instruct_from_IC(cache_t *aInstructionCache, uint64_t aAddr) {
	int myBlockOffset = get_instruction_cache_block_offset(aAddr);
	int mySetIndex = get_instruction_cache_set_index(aAddr);
	uint64_t myTag = get_instruction_cache_tag(aAddr);

	//printf("This is the set: %x and Tag: %x and offset: %x\n", mySetIndex, myTag, myBlockOffset);

	cache_set_t *myCacheSet = &(aInstructionCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);

	if (myCacheBlock == NULL) {
		cache_update(aInstructionCache, aAddr);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
	}

	myCacheBlock->last_used_iteration = stat_cycles;
	//print_cache(aInstructionCache);
	return get_specific_data_from_block(myCacheBlock, myBlockOffset);
}

/********************* FOR DATA CACHES ***************************/
// Write the Block back to memory to handle evictions
void write_back_block_to_mem(cache_block_t *aCacheBlock, uint64_t aOriginAddr) {
	int offset = 0;
	for (int i = 0; i < 8; i++) {
		mem_write_32((aOriginAddr + offset), (aCacheBlock->written_data)[i]);
		offset += 4;
	}	
}

// Used to get data from memory if cache does not have the data
// Loads a brand new block
cache_block_t *load_cache_block_DC(cache_set_t *aCacheSet, int aWays, uint64_t aAddr) {
	cache_block_t *myTargetBlock = least_recently_used_block(aCacheSet, aWays);
	uint64_t myTag = get_data_cache_tag(aAddr);	
	int offset = 0;

	uint64_t myOriginAddr = get_OriginAddr_DC(myTag, get_data_cache_set_index(aAddr));

	if (myTargetBlock->dirty_bit == 1) {
		write_back_block_to_mem(myTargetBlock, myOriginAddr);
	}

	for (int i = 0; i < 8; i++) {
		(myTargetBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
		offset += 4;
	}

	myTargetBlock->tag = myTag;
	myTargetBlock->last_used_iteration = stat_cycles;
	myTargetBlock->dirty_bit = 0;
	return myTargetBlock;
}

void update_data_cache(cache_t *aDataCache, uint64_t aAddr) {
	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if (myCacheBlock == NULL) {
		load_cache_block_DC(myCacheSet, aDataCache->ways, aAddr);
	} else {
		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

			if (myCacheBlock == NULL){
				load_cache_block_DC(myCacheSet, aDataCache->ways, get_OriginAddr_DC(myTag, mySetIndex));
			}
		}
	}
}

// checks if the data is in the data cache
int check_data_in_cache_DC(cache_t *aDataCache, uint64_t aAddr) {
	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if (myCacheBlock == NULL) {
		return 1;
	}

	if (myBlockOffset >= 29) {
		mySetIndex = get_next_set(mySetIndex);
		if (mySetIndex == 0) {
			myTag += 1;
		}

		myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

		if (myCacheBlock == NULL){
			return 2;
		}
	}
	return 0;
}

uint32_t get_specific_data_from_block_DC(cache_t *aDataCache, uint64_t aAddr) {
	uint32_t data = 0;
	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if ((myBlockOffset % 4) == 0) {
		myBlockOffset = myBlockOffset / 4;
		data = get_specific_data_from_block(myCacheBlock, myBlockOffset);
	} else {
		uint32_t append = 0;
		int remainder = myBlockOffset % 4;
		myBlockOffset = myBlockOffset / 4;
		data = get_instruction_segment((remainder*8), 31, get_specific_data_from_block(myCacheBlock, myBlockOffset));

		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);
			myBlockOffset = 0;
		} else {
			myBlockOffset += 1;
		}
		
		append = get_instruction_segment(0, remainder*8 - 1, get_specific_data_from_block(myCacheBlock, myBlockOffset));
		data = data | (append << 24);
	}

	return data;
}

uint32_t get_data_from_DC(cache_t *aDataCache, uint64_t aAddr) {
	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);
	
	if (check_data_in_cache_DC(aDataCache, aAddr) != 0) {
		printf("THIS FUNCTION SHOULD NEVER BE CALLED IF THERE IS A CACHE MISS\n");
		update_data_cache(aDataCache, aAddr);
	}

	myCacheBlock->last_used_iteration = stat_cycles;
	return get_specific_data_from_block_DC(aDataCache, aAddr);
}

void write_data_to_DC(cache_t *aDataCache, uint64_t aAddr, uint32_t data) {
	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if (check_data_in_cache_DC(aDataCache, aAddr) != 0) {
		printf("THIS FUNCTION SHOULD NEVER BE CALLED IF THERE IS A CACHE MISS\n");
		update_data_cache(aDataCache, aAddr);
	}
	write_specific_data_to_block(myCacheBlock, myBlockOffset, data);
	myCacheBlock->last_used_iteration = stat_cycles;
	myCacheBlock->dirty_bit = 1;
}

// THis function writes all of the elements of the data cache to the memory
void empty_data_cache(cache_t *aDataCache) {
	int mySets = aDataCache->sets;
	int myWays = aDataCache->ways;

	for (int s = 0; s < mySets; s++) {
		cache_set_t myCacheSet = (aDataCache->cache_sets)[s]; 
		for (int w = 0; w < myWays; w++) {
			cache_block_t myCacheBlock = (myCacheSet.cache_blocks)[w];
			if (myCacheBlock.dirty_bit == 1) {
				write_back_block_to_mem(&myCacheBlock, get_OriginAddr_DC(myCacheBlock.tag, s));
			}
		}
	}
}

/******** new general functions *********/

// void load_cache_block_IC(cache_block_t *aCacheBlock, uint64_t aTag, uint32_t aSet) {
// 	int offset = 0;
// 	uint64_t myOriginAddr = get_OriginAddr_IC(aTag, aSet);
// 	for (int i = 0; i < 8; i++) {
// 		(aCacheBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
// 		offset += 4;
// 	}
// 	myCacheBlock->last_used_iteration = stat_cycles;
// 	myCacheBlock->tag = myTag;
// }


// void load_cache_block_DC(cache_block_t *aCacheBlock, uint64_t aTag, uint32_t aSet) {
// 	int offset = 0;
// 	uint64_t myOriginAddr = get_OriginAddr_DC(aTag, aSet);
	
// 	if (myTargetBlock->dirty_bit == 1) {
// 		write_back_block_to_mem(myTargetBlock, myOriginAddr);
// 	}

// 	for (int i = 0; i < 8; i++) {
// 		(myTargetBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
// 		offset += 4;
// 	}

// 	for (int i = 0; i < 8; i++) {
// 		(aCacheBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
// 		offset += 4;
// 	}

// 	myTargetBlock->tag = myTag;
// 	myTargetBlock->last_used_iteration = stat_cycles;
// 	myTargetBlock->dirty_bit = 0;
// }


// This function just tells you what the data in the cache is
int check_data_in_cache(cache_t *aCache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;

	if (aCache->aSet == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
	} else if (aCache->aSet == 256) {
		myBlockOffset = get_data_cache_block_offset(aAddr);
		mySetIndex = get_data_cache_set_index(aAddr);
		myTag = get_data_cache_tag(aAddr);
	}

	cache_set_t *myCacheSet = &(aCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

	if (myCacheBlock == NULL) {
		return 1;
	}

	if (myBlockOffset >= 29) {
		mySetIndex = get_next_set(mySetIndex);
		if (mySetIndex == 0) {
			myTag += 1;
		}

		myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

		if (myCacheBlock == NULL){
			return 2;
		}
	}
	return 0;
}


void update_cache(cache_t *Cache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;
	int myCacheType = 0; // 1 for Instruction, 2 for Data

	if (aCache->aSet == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
		myCacheType = 1;
	} else if (aCache->aSet == 256) {
		myBlockOffset = get_data_cache_block_offset(aAddr);
		mySetIndex = get_data_cache_set_index(aAddr);
		myTag = get_data_cache_tag(aAddr);
		myCacheType = 2;
	}

	cache_set_t *myCacheSet = &(Cache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, Cache->ways);

	if (myCacheBlock == NULL) {
		myCacheBlock = least_recently_used_block(myCacheSet, Cache->ways);
		if (myCacheType == 1) {
			load_cache_block_IC(myCacheBlock, myTag, mySetIndex);
		} else if (myCacheType == 2) {
			load_cache_block_DC(myCacheBlock, myTag, mySetIndex);
		}
	} else {
		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(Cache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, Cache->ways);

			if (myCacheBlock == NULL){
				myCacheBlock = least_recently_used_block(myCacheSet, Cache->ways);
				if (myCacheType == 1) {
					load_cache_block_IC(myCacheBlock, myTag, mySetIndex);
				} else if (myCacheType == 2) {
					load_cache_block_DC(myCacheBlock, myTag, mySetIndex);
				}
			}
		}
	}
}

uint32_t read_cache(cache_t *Cache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;
	uint32_t data = 0;

	if (aCache->aSet == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
	} else if (aCache->aSet == 256) {
		myBlockOffset = get_data_cache_block_offset(aAddr);
		mySetIndex = get_data_cache_set_index(aAddr);
		myTag = get_data_cache_tag(aAddr);
	}

	cache_set_t *myCacheSet = &(Cache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, Cache->ways);

	if (myCacheBlock == NULL) {
		cache_update(Cache, aAddr);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
	}
	myCacheBlock->last_used_iteration = stat_cycles;

	if ((myBlockOffset % 4) == 0) {
		myBlockOffset = myBlockOffset / 4;
		data = get_specific_data_from_block(myCacheBlock, myBlockOffset);
	} else {
		uint32_t append = 0;
		int remainder = myBlockOffset % 4;
		myBlockOffset = myBlockOffset / 4;
		data = get_instruction_segment((remainder*8), 31, get_specific_data_from_block(myCacheBlock, myBlockOffset));

		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(Cache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, Cache->ways);
			myBlockOffset = 0;
		} else {
			myBlockOffset += 1;
		}
		
		append = get_instruction_segment(0, remainder*8 - 1, get_specific_data_from_block(myCacheBlock, myBlockOffset));
		myCacheBlock->last_used_iteration = stat_cycles;
		data = data | (append << 24);
	}
	return data;
}

void write_to_cache(cache_t *Cache, uint64_t aAddr, uint32_t aData) {
	if (aCache->aSet == 64) {
		return;
	}

	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aDataCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aDataCache->ways);

	if (myCacheBlock == NULL) {
		cache_update(Cache, aAddr);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
	}
	myCacheBlock->last_used_iteration = stat_cycles;
	myCacheBlock->dirty_bit = 1;
	
	if ((myBlockOffset % 4) == 0) {
		myBlockOffset = myBlockOffset / 4;
		write_specific_data_to_block(myCacheBlock, myBlockOffset, aData);
	} else {
		uint32_t myNewData = 0
		uint32_t append = 0
		int myRemainder = myBlockOffset % 4;
		myBlockOffset = myBlockOffset / 4;
		
		myNewData = get_instruction_segment(0, (myRemainder*8)-1, get_specific_data_from_block(myCacheBlock, myBlockOffset));
		append = get_instruction_segment(0, 31 - (myRemainder*8), aData);

		myNewData = myNewData | (append << (myRemainder*8));
		write_specific_data_to_block(myCacheBlock, myBlockOffset, myNewData);


		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(Cache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, Cache->ways);
			myBlockOffset = 0;
		} else {
			myBlockOffset += 1;
		}
		
		myNewData = get_instruction_segment(myRemainder*8, 31, get_specific_data_from_block(myCacheBlock, myBlockOffset));
		append = get_instruction_segment(myRemainder*8, 31, aData);

		myNewData = append | (myNewData << (myRemainder*8));
		write_specific_data_to_block(myCacheBlock, myBlockOffset, myNewData);

		myCacheBlock->dirty_bit = 1;
		myCacheBlock->last_used_iteration = stat_cycles;
	}
}
