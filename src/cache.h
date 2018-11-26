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
	uint32_t *written_data; // Array of 32 bit data
	uint64_t tag;
	int last_used_iteration;
	int dirty_bit;
} cache_block_t;

typedef struct 
{
	cache_block_t *cache_blocks; // Array of Cache Blocks - handles associativity
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
int get_data_cache_set_index(uint64_t aData);
uint64_t get_data_cache_tag(uint64_t aData);
uint64_t get_OriginAddr_DC(uint64_t aTag, int aSetIndex);


// INSTRUCTION CACHE FUNCTIONS
int cache_update(cache_t *c, uint64_t aAddr);
int check_data_in_cache_IC(cache_t *c, uint64_t aAddr);
uint32_t get_instruct_from_IC(cache_t *c, uint64_t aAddr);

// DATA CACHE FUNCTIONS
void update_data_cache(cache_t *c, uint64_t aAddr);
int check_data_in_cache_DC(cache_t *c, uint64_t aAddr);
uint32_t get_data_from_DC(cache_t *c, uint64_t aAddr);
void write_data_to_DC(cache_t *c, uint64_t aAddr, uint32_t data);
void empty_data_cache(cache_t* c);


#endif
