/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 */

#ifndef _BP_H_
#define _BP_H_
#define BTB_SIZE 1024
#define PHT_SIZE 256
#define CONDITIONAL 1
#define UNCONDITIONAL 0
#define VALID 1
#define INVALID 0
#include <inttypes.h>

typedef struct 
{
	unsigned char GHR; /* 8 bits */
	int PHT[PHT_SIZE]; /* each value will be in range [0,3], representing one of the 4 possible states */
} gshare_t;

typedef struct 
{
	uint64_t address_tag;
	int valid;
	int conditional;
	uint64_t branch_target;
} BTB_entry_t;

typedef struct
{
    gshare_t gshare; /* gshare */
    BTB_entry_t BTB[BTB_SIZE]; /* BTB */
} bp_t;
 
void bp_predict(/*......*/);
void bp_update(/*......*/);
void evaluate_prediction();
uint32_t get_BTB_index();
uint32_t get_8_pc_bits();
int should_take_branch();

/* Printing Functions */
void print_BTB(bp_t BP);
void print_GShare(bp_t BP);
void print_bp(bp_t BP);
#endif
