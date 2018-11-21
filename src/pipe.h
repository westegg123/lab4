/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include "helper.h"
#include <limits.h>


typedef struct CPU_State {
	/* register file state */
	int64_t REGS[ARM_REGS];
	int FLAG_N;        /* flag N */
	int FLAG_Z;        /* flag Z */

	/* program counter in fetch stage */
	uint64_t PC;
	
} CPU_State;

// PIPELINE REGISTER STRUCTS
typedef struct IF_ID_REGS {
	uint64_t PC, instruction;
} IF_ID_REGS;

typedef struct ID_EX_REGS {
	uint64_t PC, instruction, immediate, primary_data_holder, secondary_data_holder;
} ID_EX_REGS;

typedef struct EX_MEM_REGS {
	uint64_t PC, instruction, ALU_result, data_to_write;
} EX_MEM_REGS;

typedef struct MEM_WB_REGS {
	uint64_t instruction, fetched_data, ALU_result;
} MEM_WB_REGS;

typedef struct Pipeline_Regs {
	IF_ID_REGS IF_ID;
	ID_EX_REGS ID_EX;
	EX_MEM_REGS EX_MEM;
	MEM_WB_REGS MEM_WB;
} Pipeline_Regs;

// END PIPELINE REGISTER STRUCTS

int RUN_BIT;

/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif
