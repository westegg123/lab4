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
	uint32_t written_data;
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

cache_t *cache_new(int sets, int ways, int block);
void cache_destroy(cache_t *c);
int cache_update(cache_t *c, uint64_t addr);

#endif
