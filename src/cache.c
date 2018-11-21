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
}

int get_instruction_cache_set_index(uint32_t aInstruction) {
	return (int)get_instruction_segment(5, 10, aInstruction);
}

uint32_t get_instruction_cache_tag(uint32_t aInstruction) {
	return get_instruction_segment(11, 31, aInstruction);
}

int get_data_cache_block_offset(uint32_t aData) {
	return (int)get_instruction_segment(0, 4, aData);
}

int get_data_cache_set_index(uint32_t aData) {
	return (int)get_instruction_segment(5, 12, aData);
}

uint32_t get_data_cache_tag(uint32_t aData) {
	return get_instruction_segment(13, 31, aData);
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
				printf("VALUE AT BLOCK OFFSET %d: ", k+1);
				printf("%x\n", aCache->cache_sets[i].cache_blocks[j].written_data[k]);
			}
		}
		printf("\n\n");
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

cache_block_t *get_tag_match(cache_set_t *aCacheSet, uint32_t aTag, int aAssociativity) {
	cache_block_t *myTagMatch = NULL;
	for (int i = 0; i < aAssociativity; i++) {
		if (aCacheSet->cache_blocks[i].tag == aTag) {
			myTagMatch = &(aCacheSet->cache_blocks[i]);
		}
	}
	if (!myTagMatch) {
		printf("NO TAG MATCH FOUND! ");
	}
	return myTagMatch;
}

void load_cache_block(cache_block_t *aCacheBlock, uint32_t aAddr) {
	int offset = 0;
	for (int i = 0; i < 8; i++) {
		aCacheBlock->written_data[i] = (aAddr + offset);
		offset += 4;
	}
}

void cache_destroy(cache_t *aCache) {
	// Maybe consider memset to 0, but I don't think we even need to worry about this function.
	free(aCache);
}


int cache_update(cache_t *aInstructionCache, /*uint64_t addr */ uint32_t aAddr) {
	int myBlockOffset = get_instruction_cache_block_offset(aAddr);
	int mySetIndex = get_instruction_cache_set_index(aAddr);
	uint32_t myTag = get_instruction_cache_tag(aAddr);

	cache_set_t *myCacheSet = &(aInstructionCache->cache_sets[mySetIndex]);
	cache_block_t *myCacheBlock = get_tag_match(myCacheSet, myTag, aInstructionCache->ways);
	// If it is null, there has been no match, and we need to load in a block
	if (myCacheBlock == NULL) {
		printf("TAG MATCH IS NULL!\n");
		myCacheBlock = least_recently_used_block(myCacheSet, aInstructionCache->ways);
		load_cache_block(myCacheBlock, aAddr);
		myCacheBlock->last_used_iteration = stat_cycles;
		myCacheBlock->tag = myTag;
		return 1;
	} else {
		myCacheBlock->last_used_iteration = stat_cycles;
		return 0;
	}
}

