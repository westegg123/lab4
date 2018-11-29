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
	return (int)get_instruction_segment(0, 4, aInstruction);
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

// Write the Block back to memory to handle evictions
void write_back_block_to_mem(cache_block_t *aCacheBlock, uint64_t aOriginAddr) {
	int offset = 0;
	for (int i = 0; i < 8; i++) {
		mem_write_32((aOriginAddr + offset), (aCacheBlock->written_data)[i]);
		offset += 4;
	}	
	aCacheBlock->last_used_iteration = 0;
	aCacheBlock->tag = 0;
}

/*
 * This function handles loading in an instruction block into the cache
 */
void load_cache_block_IC(cache_block_t *aCacheBlock, uint64_t aTag, uint32_t aSet) {
	int offset = 0;
	uint64_t myOriginAddr = get_OriginAddr_IC(aTag, aSet);
	for (int i = 0; i < 8; i++) {
		(aCacheBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
		offset += 4;
	}
	aCacheBlock->last_used_iteration = stat_cycles;
	aCacheBlock->tag = aTag;
}


/*
 * This function handles loading in an data block into the cache. It checks if the block needs
 * to be written in memory.
 */
void load_cache_block_DC(cache_block_t *aCacheBlock, uint64_t aTag, uint32_t aSet) {
	int offset = 0;
	uint64_t myOriginAddr = get_OriginAddr_DC(aTag, aSet);
	
	if (aCacheBlock->dirty_bit == 1) {
		write_back_block_to_mem(aCacheBlock, myOriginAddr);
	}

	for (int i = 0; i < 8; i++) {
		(aCacheBlock->written_data)[i] = mem_read_32(myOriginAddr + offset);
		offset += 4;
	}

	aCacheBlock->tag = aTag;
	aCacheBlock->last_used_iteration = stat_cycles;
	aCacheBlock->dirty_bit = 0;
}


// This function just tells you what the data in the cache is
int check_data_in_cache(cache_t *aCache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;

	if (aCache->sets == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
	} else if (aCache->sets == 256) {
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

		myCacheSet = &(aCache->cache_sets[mySetIndex]);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

		if (myCacheBlock == NULL){
			return 2;
		}
	}
	return 0;
}


/*
 * This function updates the cache with the missing block.
 */
void cache_update(cache_t *aCache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;
	int myCacheType = 0; // 1 for Instruction, 2 for Data
	
	if (aCache->sets == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
		myCacheType = 1;
	} else if (aCache->sets == 256) {
		myBlockOffset = get_data_cache_block_offset(aAddr);
		mySetIndex = get_data_cache_set_index(aAddr);
		myTag = get_data_cache_tag(aAddr);
		myCacheType = 2;
	}

	cache_set_t *myCacheSet = &(aCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

	// if (aCache->sets == 256) {
	// 	printf("In cache Update\n");
	// 	printf("This is Tag: %lx, Set: %d, Offset: %d\n", myTag, mySetIndex, myBlockOffset);
	// }

	if (myCacheBlock == NULL) {
		myCacheBlock = least_recently_used_block(myCacheSet, aCache->ways);
		if (myCacheType == 1) {
			// printf("loading cache block IC\n");
			load_cache_block_IC(myCacheBlock, myTag, mySetIndex);
		} else if (myCacheType == 2) {
			// printf("loading cache block DC\n");
			load_cache_block_DC(myCacheBlock, myTag, mySetIndex);
		}
	} //else {
	if (myBlockOffset >= 29) {
		mySetIndex = get_next_set(mySetIndex);
		if (mySetIndex == 0) {
			myTag += 1;
		}

		myCacheSet = &(aCache->cache_sets[mySetIndex]);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

		if (myCacheBlock == NULL){
			myCacheBlock = least_recently_used_block(myCacheSet, aCache->ways);
			if (myCacheType == 1) {
				load_cache_block_IC(myCacheBlock, myTag, mySetIndex);
			} else if (myCacheType == 2) {
				load_cache_block_DC(myCacheBlock, myTag, mySetIndex);
			}
		}
	}
	//}
}


/*
 * This function reads the cache and returns the data from the cache
 */
uint32_t read_cache(cache_t *aCache, uint64_t aAddr) {
	int myBlockOffset = 0;
	int mySetIndex = 0;
	uint64_t myTag = 0;
	uint32_t data = 0;

	if (aCache->sets == 64) {
		myBlockOffset = get_instruction_cache_block_offset(aAddr);
		mySetIndex = get_instruction_cache_set_index(aAddr);
		myTag = get_instruction_cache_tag(aAddr);
	} else if (aCache->sets == 256) {
		myBlockOffset = get_data_cache_block_offset(aAddr);
		mySetIndex = get_data_cache_set_index(aAddr);
		myTag = get_data_cache_tag(aAddr);
	}

	cache_set_t *myCacheSet = &(aCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

	if (myCacheBlock == NULL) {
		cache_update(aCache, aAddr);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);
	}
	myCacheBlock->last_used_iteration = stat_cycles;

	// if (aCache->sets == 256) {
	// 	printf("In read cache\n");
	// 	printf("This is Tag: %lx, Set: %d, Offset: %d\n", myTag, mySetIndex, myBlockOffset);
	// }

	if ((myBlockOffset % 4) == 0) {
		myBlockOffset = myBlockOffset / 4;
		data = get_specific_data_from_block(myCacheBlock, myBlockOffset);
	} else {
		// printf("The Block Offset is not divisible 4\n");
		
		uint32_t append = 0;
		int remainder = myBlockOffset % 4;
		int myBlockIndex = myBlockOffset / 4;
		
		data = get_instruction_segment((remainder*8), 31, get_specific_data_from_block(myCacheBlock, myBlockIndex));
		// printf("Data from first block: %x\n", data);

		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}

			myCacheSet = &(aCache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);
			myBlockIndex = 0;
		} else {
			myBlockIndex += 1;
		}
		
		append = get_instruction_segment(0, (remainder*8) - 1, get_specific_data_from_block(myCacheBlock, myBlockIndex));
		myCacheBlock->last_used_iteration = stat_cycles;

		//printf("This is the appended data: %x\n", append);
		data = data | (append << (32-remainder*8));
		//printf("This is the combined data: %x\n", data);
	}
	return data;
}


/*
 * This function allows you to write to the cache. It can be only be used for the data cache
 */
void write_to_cache(cache_t *aCache, uint64_t aAddr, uint32_t aData) {
	if (aCache->sets == 64) {
		return;
	}

	int myBlockOffset = get_data_cache_block_offset(aAddr);
	int mySetIndex = get_data_cache_set_index(aAddr);
	uint64_t myTag = get_data_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);

	if (myCacheBlock == NULL) {
		cache_update(aCache, aAddr);
		myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);
	}

	myCacheBlock->last_used_iteration = stat_cycles;
	myCacheBlock->dirty_bit = 1;
	
	// printf("In Write to cache (%d)\n", stat_cycles+1);
	// printf("This is Tag: %lx, Set: %d, Offset: %d\n", myTag, mySetIndex, myBlockOffset);

	if ((myBlockOffset % 4) == 0) {
		myBlockOffset = myBlockOffset / 4;
		write_specific_data_to_block(myCacheBlock, myBlockOffset, aData);
	} else {
		// printf("The Block Offset is not divisible 4\n");
		// printf("DATA: %x\n", aData);
		uint32_t myNewData = 0;
		uint32_t myCacheData = 0;
		uint32_t myData = 0;
		int myRemainder = myBlockOffset % 4;
		int myBlockIndex = myBlockOffset / 4;
		

		print_block(myCacheBlock);
		myCacheData = get_instruction_segment(0, (myRemainder*8)-1, get_specific_data_from_block(myCacheBlock, myBlockIndex));
		myData = get_instruction_segment(0, ((4-myRemainder)*8) - 1, aData);

		myNewData = myCacheData | (myData << ((myRemainder)*8));
		//printf("This is the data being written: %x\n", myNewData);
		write_specific_data_to_block(myCacheBlock, myBlockIndex, myNewData);

		if (myBlockOffset >= 29) {
			mySetIndex = get_next_set(mySetIndex);
			if (mySetIndex == 0) {
				myTag += 1;
			}
			
			//printf("This the new Tag: %lx, new Set: %d, Offset: %d\n", myTag, mySetIndex, myBlockOffset);

			myCacheSet = &(aCache->cache_sets[mySetIndex]);
			myCacheBlock = get_tag_match(myCacheSet, myTag, aCache->ways);
			myBlockIndex = 0;
		} else {
			myBlockIndex += 1;
		}

		myCacheData = get_instruction_segment(myRemainder*8, 31, get_specific_data_from_block(myCacheBlock, myBlockIndex));
		myData = get_instruction_segment((4-myRemainder)*8, 31, aData);
	
		myNewData = myData | (myCacheData << (myRemainder*8));
		// printf("This is the data being written: %x\n", myNewData);
		// printf("\n\n");

		write_specific_data_to_block(myCacheBlock, myBlockIndex, myNewData);

		myCacheBlock->dirty_bit = 1;
		myCacheBlock->last_used_iteration = stat_cycles;
	}
}
