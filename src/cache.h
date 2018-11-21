/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

typedef struct 
{
	uint32_t *written_data;
	uint32_t tag;
	int last_used_iteration;
} cache_block_t;

typedef struct 
{
	cache_block_t *cache_blocks;
} cache_set_t;

typedef struct
{
	cache_set_t *cache_sets;
	int sets;
	int ways;
	int block;
} cache_t;

cache_t *instruction_cache_new();
cache_t *data_cache_new();
void cache_destroy(cache_t *c);
void print_cache();
int cache_update(cache_t *c, /*uint64_t*/uint32_t addr);

#endif
