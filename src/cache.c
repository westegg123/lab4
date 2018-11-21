/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include "helper.h"
#include <stdlib.h>
#include <stdio.h>


// Number of sets, then associativity, then block size in bytes.
cache_t *cache_new(int sets, int ways, int block) {
	cache_t *myCache = calloc(1, sizeof(cache_t));
	myCache->sets = sets;
	myCache->ways = ways;
	myCache->block = block;
	myCache->cache_sets = calloc(sets, sizeof(cache_set_t));
	for (int i = 0; i < sets; i++) {
		myCache->cache_sets[i].cache_blocks = calloc(8, sizeof(cache_block_t));
	}
	return myCache;
}

cache_t *instruction_cache_new(int sets, int ways, int block) {
	return cache_new(64, 4, 32);
}

cache_t *data_cache_new(int sets, int ways, int block) {
	return cache_new(256, 8, 32);
}

void cache_destroy(cache_t *c) {
	// Maybe consider memset to 0, but I don't think we even need to worry about this function.
	free(c);
}


int cache_update(cache_t *c, uint64_t addr) {

}

