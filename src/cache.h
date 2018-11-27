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

int get_instruction_cache_set_index(uint32_t aInstruction);
uint64_t get_instruction_cache_tag(uint32_t aInstruction);


// Origin address functions
uint64_t get_OriginAddr_IC(uint64_t aTag, int aSetIndex);
uint64_t get_OriginAddr_DC(uint64_t aTag, int aSetIndex);

// CACHING FUNCTIONS
void cache_update(cache_t *c, uint64_t aAddr);
int check_data_in_cache(cache_t *c, uint64_t aAddr);
uint32_t read_cache(cache_t *c, uint64_t aAddr);
void write_to_cache(cache_t *c, uint64_t aAddr, uint32_t data);



#endif
