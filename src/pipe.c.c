/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */
//#include "sim.c"
#include "pipe.h"
#include "shell.h"
#include "helper.h"
#include "bp.h"
//#include "bp.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


// /*
//  * CMSC 22200
//  *
//  * Group: Andreas Komissopoulos and James Kon  
//  * ARM pipeline timing simulator
//  */

// #include "pipe.h"
// #include "shell.h"
// #include "helper.h"
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <assert.h>


/* global pipeline and branch prediction state */
int branch_taken = -1;
CPU_State CURRENT_STATE;
Pipeline_Regs CURRENT_REGS, START_REGS;
//bp_t *BP = malloc(sizeof(bp_t));
bp_t BP;
/* FLAGS */
int FETCH_MORE = 1;
int BUBBLE = 0;
int BRANCH_STALL = 0;
/* Notes on forwarding:
 * For bubbling, need to implement a control for each function
 * For forwarding - need to forward in the ID stage of each dependent instruction
 * For dependencies of two, need to check forwarding issues 
 * Need to pay attention to STUR and CBZ instruction
 */


/************************************ CONSTANTS ************************************/
/* 
 * NOT INCLUDED: CBNZ, CBZ, MOVZ, B, B.COND
 * 
 */
#define HLT 0xd4400000 // entire instr, not code
#define ADD 0x458 //need to add one to handle both cases
#define ADDI 0x488 //need to add one to handle both cases
#define ADDS 0x558 //need to add one to handle both cases
#define ADDIS 0x588 //need to add one to handle both cases
#define AND 0x450
#define ANDS 0x750
#define EOR 0x650
#define ORR 0x550
#define LDUR_64 0x7C2
#define LDUR_32 0x5C2
#define LDURB 0x1C2
#define LDURH 0x3C2
#define LSL	0x69B //IMMEDIATE VERSION
#define LSR 0x69A //IMMEDIATE VERSION
#define STUR 0x7C0
#define STURB 0x1C0
#define STURH 0x3C0
#define STURW 0x5C0
#define SUB 0x658 //need to add one to handle both cases
#define SUBI 0x688 //need to add one to handle both cases
#define SUBS 0x758 //need to add one to handle both cases
#define SUBIS 0x788
#define MUL 0x4D8
#define BR 0x6B0

#define CONDITIONAL 1
#define UNCONDITIONAL 0

#define VALID 1
#define INVALID 0
/************************************ END OF CONSTANTS ************************************/



/************************************ HELPERS ************************************/

void reset_bubble() {
	BUBBLE = 0;
}

void clear_IF_ID_REGS() {
	CURRENT_REGS.IF_ID.PC = 0;
	CURRENT_REGS.IF_ID.instruction = 0;
}

void clear_ID_EX_REGS() {
	CURRENT_REGS.ID_EX.PC = 0;
	CURRENT_REGS.ID_EX.instruction = 0;
	CURRENT_REGS.ID_EX.immediate = 0;
	CURRENT_REGS.ID_EX.primary_data_holder = 0;
	CURRENT_REGS.ID_EX.secondary_data_holder = 0;
}

void clear_EX_MEM_REGS() {
	CURRENT_REGS.EX_MEM.PC = 0;
	CURRENT_REGS.EX_MEM.instruction = 0;
	CURRENT_REGS.EX_MEM.ALU_result = 0;
	CURRENT_REGS.EX_MEM.data_to_write = 0;
}

void clear_MEM_WB_REGS() {
	CURRENT_REGS.MEM_WB.instruction = 0;
	CURRENT_REGS.MEM_WB.fetched_data = 0;
	CURRENT_REGS.MEM_WB.ALU_result = 0;
}

// Equation for regWrite flag
int get_regWrite(uint32_t opcode) {
	return (opcode < 0x0A0 || opcode > 0x0BF) && (opcode != STUR) && 
		(opcode != STURB) && (opcode != STURH) && (opcode != STURW);
}

int get_memRead(uint32_t opcode) {
	return (opcode == LDUR_64) || (opcode == LDUR_32) ||
		(opcode == LDURB) || (opcode == LDURH);
}

int hazard_detection_unit(uint32_t depend_instruct, uint32_t ind_instruct) {
	if (ind_instruct == 0) {
		return 0;
	}

	parsed_instruction_holder depend_holder = get_holder(depend_instruct);
	parsed_instruction_holder ind_holder = get_holder(ind_instruct);

	if (depend_holder.opcode >= 0x694 && depend_holder.opcode <= 0x697) {
		return 0;
	}

	if (get_memRead(ind_holder.opcode)) {
		if (ind_holder.Rt == depend_holder.Rn) {
			if (depend_holder.format != 4 && depend_holder.format != 5 &&
				depend_holder.format != 6) {
				return 1;
			}
		}

		if (depend_holder.format == 1) {	
			if (depend_holder.opcode == LSL || depend_holder.opcode == LSR 
				|| depend_holder.opcode == BR) {
				return 0;
			}
			if (ind_holder.Rt == depend_holder.Rm) {
				return 2;
			}
		// SPECIAL FOR STUR, CBZ, CBNZ
		} else if (depend_holder.opcode == STUR || depend_holder.opcode == STURH ||
			depend_holder.opcode == STURB || depend_holder.opcode == STURW ||
			(depend_holder.opcode >= 0x5A0 && depend_holder.opcode <= 0x5AF)) {

			if (ind_holder.Rt == depend_holder.Rt) {
				return 2;
			}
		}
	}
	return 0;
}

int forward(uint32_t depend_instruct, uint32_t ind_instruct) {
	parsed_instruction_holder depend_holder = get_holder(depend_instruct);
	parsed_instruction_holder ind_holder = get_holder(ind_instruct);

	if (get_memRead(ind_instruct)) {
		return 0;
	}

	if (depend_holder.opcode >= 0x694 && depend_holder.opcode <= 0x697) {
		return 0;
	}

	if ((ind_holder.Rd != 31) && get_regWrite(ind_holder.opcode)) {
		uint32_t ind_target = ind_holder.Rd;
		if (ind_holder.format == 3) {
			ind_target = ind_holder.Rt;
		} else if (ind_holder.format == 5 || ind_holder.format == 4) {
			return 0;
		}

		if (ind_target == depend_holder.Rn) {
			if (depend_holder.format != 4 && depend_holder.format != 5 &&
				depend_holder.format != 6) {
				return 1;
			}
		}

		if (depend_holder.format == 1) {	
			if (depend_holder.opcode == LSL || depend_holder.opcode == LSR ||
				depend_holder.opcode == BR) {
				return 0;
			} 
			if (ind_target == depend_holder.Rm) {
				return 2;
			}
		} else if (depend_holder.opcode == STUR || depend_holder.opcode == STURH ||
			depend_holder.opcode == STURB || depend_holder.opcode == STURW ||
			(depend_holder.opcode >= 0x5A0 && depend_holder.opcode <= 0x5AF)) {
			
			if (ind_target == depend_holder.Rt) {
				return 2;
			}
		}
	}
	return 0;
}

/******************************* R EXECUTION INSTRUCTIONS HANLDERS *******************************/

void handle_add() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.secondary_data_holder;
}

void handle_adds() {
	handle_add();
}

void handle_and() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder & CURRENT_REGS.ID_EX.secondary_data_holder;	
}

void handle_ands() {
	handle_and();
}

void handle_eor() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder ^ CURRENT_REGS.ID_EX.secondary_data_holder;	
}

void handle_orr() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder | CURRENT_REGS.ID_EX.secondary_data_holder;
}

void handle_lsl() {
	CURRENT_REGS.EX_MEM.ALU_result = ((uint64_t)CURRENT_REGS.ID_EX.primary_data_holder) << (0x3F - CURRENT_REGS.ID_EX.secondary_data_holder);
}

void handle_lsr() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder >> CURRENT_REGS.ID_EX.secondary_data_holder;
}

void handle_sub() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder - CURRENT_REGS.ID_EX.secondary_data_holder;		
}

void handle_subs() {
	handle_sub();
}

void handle_br() {
	if (CURRENT_STATE.PC != CURRENT_REGS.ID_EX.primary_data_holder) {
		CURRENT_STATE.PC = CURRENT_REGS.ID_EX.primary_data_holder;
		clear_IF_ID_REGS();
		clear_ID_EX_REGS();
		BUBBLE = 1;
	}
}

void handle_mul() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder * CURRENT_REGS.ID_EX.secondary_data_holder;		
}


/******************************* I EXECUTION INSTRUCTIONS HANLDERS *******************************/
void handle_addi() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate; 
}	

void handle_addis() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
}

void handle_subi() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder - CURRENT_REGS.ID_EX.immediate;
}

void handle_subis() {
	CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder - CURRENT_REGS.ID_EX.immediate;
}


/******************************* CB EXECUTION INSTRUCTIONS HANLDERS *******************************/


void handle_bcond(parsed_instruction_holder HOLDER) {
	uint32_t cond = (HOLDER.Rt & 14) >> 1;
	int flag_N = CURRENT_STATE.FLAG_N;
	int flag_Z = CURRENT_STATE.FLAG_Z;
	int result = 0; 

	parsed_instruction_holder MEM_instruct = get_holder(START_REGS.EX_MEM.instruction);
	if (MEM_instruct.opcode == ADDS || MEM_instruct.opcode == (ADDS + 1) ||
		MEM_instruct.opcode == ANDS || MEM_instruct.opcode == SUBS || 
		MEM_instruct.opcode == (SUBS + 1) || MEM_instruct.opcode == ADDIS || 
		MEM_instruct.opcode == (ADDIS + 1) || MEM_instruct.opcode == SUBIS || 
		MEM_instruct.opcode == (SUBIS + 1)) {

		flag_Z = (START_REGS.EX_MEM.ALU_result == 0) ? 1 : 0;
		flag_N = (START_REGS.EX_MEM.ALU_result < 0) ? 1 : 0;
	}

	if (cond == 0) {
		// EQ or NE
		//printf("HANDLING BEQ or BNE\n");
		//printf("flag Z:	%x\n", flag_Z);
		if (flag_Z == 1) {
			result = 1;	
		}
	} else if (cond == 5) {
		// BGE or BLT
		// printf("HANDLING BGE or BLT\n");
		if (flag_N == 0) {
			result = 1;
		}
	} else if (cond == 6) {
		// BGT or BLE
		// printf("HANDLING BGT or BLE\n");
		if ((flag_N == 0) && (flag_Z == 0)) {
			result = 1;
		}
	}

	if ((HOLDER.Rt & 1) == 1 && ((HOLDER.Rt & 15) != 15)) {
		result = !result;
	}

	if (result == 1) {
		//printf("BRANCHING\n");
		CURRENT_STATE.PC = CURRENT_REGS.ID_EX.PC + CURRENT_REGS.ID_EX.immediate;
		BRANCH_STALL = 1;
		clear_IF_ID_REGS();
		clear_ID_EX_REGS();
	}
}


void handle_cbnz() {
	if (CURRENT_REGS.ID_EX.secondary_data_holder != 0) {
		CURRENT_STATE.PC = CURRENT_REGS.ID_EX.PC + CURRENT_REGS.ID_EX.immediate;
		BRANCH_STALL = 1;
		clear_IF_ID_REGS();
		clear_ID_EX_REGS();
	}
}


void handle_cbz() {
	if (CURRENT_REGS.ID_EX.secondary_data_holder == 0) {
		CURRENT_STATE.PC = CURRENT_REGS.ID_EX.PC + CURRENT_REGS.ID_EX.immediate;
		BRANCH_STALL = 1;
		clear_IF_ID_REGS();
		clear_ID_EX_REGS();
	}
}

/************************************ END OF HELPERS ************************************/

void pipe_init() {
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;
}

void pipe_cycle() {
	//printf("--------CYCLE START (%lx)-----\n", CURRENT_STATE.PC);
	START_REGS = CURRENT_REGS;
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
	reset_bubble();
//	printf("--------CYCLE END-------\n\n");
}

void pipe_stage_wb() {
	printf("Write BACK -----------> ");
	print_operation(CURRENT_REGS.MEM_WB.instruction);
	if (CURRENT_REGS.MEM_WB.instruction == 0) {
		//printf("Write Back Stage Skipped\n");
		return;
	} else if (CURRENT_REGS.MEM_WB.instruction == HLT) {
		//printf("FINITO\n");
		//printf("WRITING INSTRUCTION: %lx\n", CURRENT_REGS.MEM_WB.instruction);
		stat_inst_retire++;
		RUN_BIT = 0;
		return;
	}
	parsed_instruction_holder INSTRUCTION_HOLDER = get_holder(CURRENT_REGS.MEM_WB.instruction);
	int WRITE_TO = -1;

	if (INSTRUCTION_HOLDER.format == 1) {
		WRITE_TO = INSTRUCTION_HOLDER.Rd;
		if (INSTRUCTION_HOLDER.opcode == ADDS || INSTRUCTION_HOLDER.opcode == (ADDS + 1) ||
			INSTRUCTION_HOLDER.opcode == ANDS || 
			INSTRUCTION_HOLDER.opcode == SUBS || INSTRUCTION_HOLDER.opcode == (SUBS + 1)) {
			CURRENT_STATE.FLAG_N = ((long)CURRENT_REGS.MEM_WB.ALU_result < 0) ? 1 : 0;
			CURRENT_STATE.FLAG_Z = (CURRENT_REGS.MEM_WB.ALU_result == 0) ? 1 : 0;
		}

		if (INSTRUCTION_HOLDER.opcode == BR) {
			WRITE_TO = -1;
		}

	} else if (INSTRUCTION_HOLDER.format == 2) {
		WRITE_TO = INSTRUCTION_HOLDER.Rd;
		if (INSTRUCTION_HOLDER.opcode == ADDIS || INSTRUCTION_HOLDER.opcode == (ADDIS + 1) ||
			INSTRUCTION_HOLDER.opcode == SUBIS || INSTRUCTION_HOLDER.opcode == (SUBIS + 1)) {
			CURRENT_STATE.FLAG_N = ((long)CURRENT_REGS.MEM_WB.ALU_result < 0) ? 1 : 0;
			CURRENT_STATE.FLAG_Z = (CURRENT_REGS.MEM_WB.ALU_result == 0) ? 1 : 0;
		}

	} else if (INSTRUCTION_HOLDER.format == 3) {
		if ((INSTRUCTION_HOLDER.opcode == LDUR_64) || (INSTRUCTION_HOLDER.opcode == LDUR_32) ||
			(INSTRUCTION_HOLDER.opcode == LDURH) || (INSTRUCTION_HOLDER.opcode == LDURB)) {
			WRITE_TO = INSTRUCTION_HOLDER.Rt;
		}
	} else if (INSTRUCTION_HOLDER.format == 4 || INSTRUCTION_HOLDER.format == 5) {
		//printf("SOMETHING WEIRD HAPPENING - CB SHOULDNT WRITE BACK\n");
	} else if (INSTRUCTION_HOLDER.format == 6) {
		WRITE_TO = INSTRUCTION_HOLDER.Rd;
	}

	if ((WRITE_TO != -1) && (WRITE_TO != 31)) {
		if (INSTRUCTION_HOLDER.format != 3) { 
			CURRENT_STATE.REGS[WRITE_TO] = CURRENT_REGS.MEM_WB.ALU_result;
		} else {
			CURRENT_STATE.REGS[WRITE_TO] = CURRENT_REGS.MEM_WB.fetched_data;
		}
	}
	stat_inst_retire++;
}

void pipe_stage_mem() {
	printf("Memory -----------> ");
	print_operation(CURRENT_REGS.EX_MEM.instruction);

	if (CURRENT_REGS.EX_MEM.instruction == 0) {
		//printf("Memmeory Stage Skipped\n");
		clear_MEM_WB_REGS();
		return;
	} else if ( CURRENT_REGS.EX_MEM.instruction == HLT) {
		clear_MEM_WB_REGS();
		CURRENT_REGS.MEM_WB.instruction = CURRENT_REGS.EX_MEM.instruction;
		return;
	}

	clear_MEM_WB_REGS();
	parsed_instruction_holder INSTRUCTION_HOLDER = get_holder(CURRENT_REGS.EX_MEM.instruction);
	
	if (INSTRUCTION_HOLDER.format == 1) {
		//printf ("SOMETHING WEIRD HAPPENING - R INSTR SHOULDNT GO TO MEM\n");
	} else if (INSTRUCTION_HOLDER.format == 2) {
		//printf ("SOMETHING WEIRD HAPPENING - I INSTR SHOULDNT GO TO MEM\n");
	} else if (INSTRUCTION_HOLDER.format == 3) {
		// load
		if (INSTRUCTION_HOLDER.opcode == 0x7C2 || INSTRUCTION_HOLDER.opcode == 0x5C2) {
			CURRENT_REGS.MEM_WB.fetched_data = mem_read_64(CURRENT_REGS.EX_MEM.ALU_result);
		} else if (INSTRUCTION_HOLDER.opcode == 0x1C2 ) {
			CURRENT_REGS.MEM_WB.fetched_data = get_memory_segment(0,7,mem_read_32(CURRENT_REGS.EX_MEM.ALU_result));
		} else if (INSTRUCTION_HOLDER.opcode == 0x3C2) {
			CURRENT_REGS.MEM_WB.fetched_data = get_memory_segment(0,15,mem_read_32(CURRENT_REGS.EX_MEM.ALU_result));
		} else /* store */{
			if (INSTRUCTION_HOLDER.opcode != STUR) {
				mem_write_32(CURRENT_REGS.EX_MEM.ALU_result, CURRENT_REGS.EX_MEM.data_to_write);
			} else {
				mem_write_64(CURRENT_REGS.EX_MEM.ALU_result, CURRENT_REGS.EX_MEM.data_to_write);
			}
		}
	} else if ((INSTRUCTION_HOLDER.format == 4) || (INSTRUCTION_HOLDER.format == 5) || 
		(INSTRUCTION_HOLDER.format == 6)) {

	}
	CURRENT_REGS.MEM_WB.ALU_result = CURRENT_REGS.EX_MEM.ALU_result;
	CURRENT_REGS.MEM_WB.instruction = CURRENT_REGS.EX_MEM.instruction;
}

// R INSTR EXECUTE STAGE
void pipe_stage_execute() {
	printf("Execute -----------> ");
	print_operation(CURRENT_REGS.ID_EX.instruction);

	printf("PC OF INSTRUCTION TO EXECUTE: %x. PC OF NEXT INSTRUCTION: %x\n", CURRENT_REGS.ID_EX.PC, CURRENT_REGS.IF_ID.PC);

	if (CURRENT_REGS.ID_EX.instruction == 0) {
		//printf("Execute Skipped\n");
		clear_EX_MEM_REGS();
		return;
	} else if (CURRENT_REGS.ID_EX.instruction == HLT) {
		clear_EX_MEM_REGS();
		CURRENT_REGS.EX_MEM.instruction = CURRENT_REGS.ID_EX.instruction;
		return;
	}

	BUBBLE = (hazard_detection_unit(CURRENT_REGS.ID_EX.instruction, CURRENT_REGS.EX_MEM.instruction) != 0) ? 1 : 0;
	if (BUBBLE != 0) {
		clear_EX_MEM_REGS();
		return;
	}

	parsed_instruction_holder HOLDER = get_holder(CURRENT_REGS.ID_EX.instruction);	

	//check if there is immediate dependicies (EX/MEM to ID/EX), then check dependicies between (MEM/WB and ID/EX)
	int MEM_forward = forward(CURRENT_REGS.ID_EX.instruction, CURRENT_REGS.EX_MEM.instruction);
	int WB_forward = forward(CURRENT_REGS.ID_EX.instruction, START_REGS.MEM_WB.instruction);

	//printf("This is Mem forward: %u\n", MEM_forward);
	//printf("This is WB forward: %u\n", WB_forward);

	//printf("MEM - Instruction 1: %lx <----- Instruction 2: %lx\n", CURRENT_REGS.EX_MEM.instruction, CURRENT_REGS.ID_EX.instruction);
	if (MEM_forward == 1) {
		CURRENT_REGS.ID_EX.primary_data_holder = CURRENT_REGS.EX_MEM.ALU_result;
	} else if (MEM_forward == 2) {
		CURRENT_REGS.ID_EX.secondary_data_holder = CURRENT_REGS.EX_MEM.ALU_result;
	}

	if ((WB_forward != 0) && (MEM_forward != WB_forward)) {
		if (WB_forward == 1) {
			CURRENT_REGS.ID_EX.primary_data_holder = START_REGS.MEM_WB.ALU_result;
		} else {
			CURRENT_REGS.ID_EX.secondary_data_holder = START_REGS.MEM_WB.ALU_result;
		}
	}

	// if (CURRENT_REGS.EX_MEM.instruction == 0) {
	if (get_memRead(get_holder(START_REGS.MEM_WB.instruction).opcode)) {
		int bubble_result = hazard_detection_unit(CURRENT_REGS.ID_EX.instruction, START_REGS.MEM_WB.instruction);
		if (bubble_result == 1) {
			CURRENT_REGS.ID_EX.primary_data_holder = START_REGS.MEM_WB.fetched_data;
		} else if (bubble_result == 2) {
			CURRENT_REGS.ID_EX.secondary_data_holder = START_REGS.MEM_WB.fetched_data;
		}	
	}


	clear_EX_MEM_REGS();
	CURRENT_REGS.EX_MEM.instruction = CURRENT_REGS.ID_EX.instruction;
	if (HOLDER.format == 1) {
		if (HOLDER.opcode == 0x458 || HOLDER.opcode == 0x459) {
			handle_add();
		} else if (HOLDER.opcode == 0x558 || HOLDER.opcode == 0x559) {
			handle_adds();
		} else if (HOLDER.opcode == 0x450) {
			handle_and();
		} else if (HOLDER.opcode == 0x750) {
			handle_ands();
		} else if (HOLDER.opcode == 0x650) {
			handle_eor();
		} else if (HOLDER.opcode == 0x550) {
			handle_orr();
		} else if (HOLDER.opcode == 0x69B) {
			if (get_instruction_segment(10,15, CURRENT_REGS.ID_EX.instruction) == 0x3F) {
				CURRENT_REGS.ID_EX.secondary_data_holder = get_instruction_segment(16,21, CURRENT_REGS.ID_EX.instruction);
				handle_lsr();
		} 	else {
				CURRENT_REGS.ID_EX.secondary_data_holder = get_instruction_segment(10,15, CURRENT_REGS.ID_EX.instruction);
				handle_lsl();
			}
		} else if (HOLDER.opcode == 0x69A) {
			if (get_instruction_segment(10,15, CURRENT_REGS.ID_EX.instruction) != 0x3F) {
				CURRENT_REGS.ID_EX.secondary_data_holder = get_instruction_segment(10,15, CURRENT_REGS.ID_EX.instruction);
				handle_lsl();
		} 	else {
				CURRENT_REGS.ID_EX.secondary_data_holder = get_instruction_segment(16,21, CURRENT_REGS.ID_EX.instruction);
				handle_lsr();
			}	
		} else if (HOLDER.opcode == 0x658 || HOLDER.opcode == 0x659) {
			handle_sub();
		} else if (HOLDER.opcode == 0x758 || HOLDER.opcode == 0x759) {
			handle_subs();
		} else if (HOLDER.opcode == 0x6B0) {
			handle_br();
		} else if (HOLDER.opcode == 0x4D8) {
			handle_mul();
		}
	} else if (HOLDER.format == 2) {
		if (HOLDER.opcode == ADDI || HOLDER.opcode == (ADDI + 1)) {
			handle_addi();
		} else if (HOLDER.opcode == ADDIS || HOLDER.opcode == (ADDIS + 1)) {
			handle_addis();
		} else if (HOLDER.opcode == SUBI || HOLDER.opcode == (SUBI + 1)) {
			handle_subi();
		} else if (HOLDER.opcode == SUBIS || HOLDER.opcode == (SUBIS + 1)) {
			handle_subis();
		}
	} else if (HOLDER.format == 3) {
		if (HOLDER.opcode == 0x7C2) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
		} else if (HOLDER.opcode == 0x1C2) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
		} else if (HOLDER.opcode == 0x3C2) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
		} else if (HOLDER.opcode == 0x7C0) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_STATE.REGS[HOLDER.Rn] + CURRENT_REGS.ID_EX.immediate;
			CURRENT_REGS.EX_MEM.data_to_write = CURRENT_REGS.ID_EX.secondary_data_holder;
		} else if (HOLDER.opcode == 0x1C0) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
			CURRENT_REGS.EX_MEM.data_to_write = get_memory_segment(0,7, CURRENT_REGS.ID_EX.secondary_data_holder);
		} else if (HOLDER.opcode == 0x3C0) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
			CURRENT_REGS.EX_MEM.data_to_write = get_memory_segment(0,15, CURRENT_REGS.ID_EX.secondary_data_holder);
		} else if (HOLDER.opcode == 0x5C0) {
			CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.primary_data_holder + CURRENT_REGS.ID_EX.immediate;
			CURRENT_REGS.EX_MEM.data_to_write = get_memory_segment(0,31, CURRENT_REGS.ID_EX.secondary_data_holder);
		}
	} else if (HOLDER.format == 4) {
		if (CURRENT_STATE.PC != (CURRENT_REGS.ID_EX.PC + CURRENT_REGS.ID_EX.immediate)) {
			uint32_t myExecuteInstructionPC = CURRENT_REGS.ID_EX.PC;
			uint32_t myPredictedNextInstructionPC = CURRENT_REGS.IF_ID.PC;
			uint32_t myActualNextInstructionPC = CURRENT_REGS.ID_EX.PC + CURRENT_REGS.ID_EX.immediate;

			if (myActualNextInstructionPC != myPredictedNextInstructionPC) {
				printf("UNCONDITIONAL BRANCH: PREDICTION INCORRECT.\n");
				BRANCH_STALL = 1;
				clear_IF_ID_REGS();
				clear_ID_EX_REGS();
			}
			CURRENT_STATE.PC = myActualNextInstructionPC;
			bp_update(myExecuteInstructionPC, myActualNextInstructionPC, UNCONDITIONAL, VALID, -5);
		}
	} else if (HOLDER.format == 5) {
		if (HOLDER.opcode >= 0x5A8 && HOLDER.opcode <= 0x5AF) {
			handle_cbnz();
		} else if (HOLDER.opcode >= 0x5A0 && HOLDER.opcode <= 0x5A7) {
			handle_cbz();
		} else if (HOLDER.opcode >= 0x2A0 && HOLDER.opcode <= 0x2A7) {
			handle_bcond(HOLDER);
		}
	} else if (HOLDER.format == 6) {
		CURRENT_REGS.EX_MEM.ALU_result = CURRENT_REGS.ID_EX.immediate;
	}
}


void pipe_stage_decode() {
	printf("Decode -----------> ");
	print_operation(CURRENT_REGS.IF_ID.instruction);

	if (BUBBLE != 0) {
		return;
	}

	if (CURRENT_REGS.IF_ID.instruction == 0) {
		//printf("SKIPPING DECODE\n");
		clear_ID_EX_REGS();
		return;
	} else if (CURRENT_REGS.IF_ID.instruction == HLT) {
		clear_ID_EX_REGS();
		CURRENT_REGS.ID_EX.instruction = CURRENT_REGS.IF_ID.instruction;
		FETCH_MORE = 0;
		CURRENT_STATE.PC = CURRENT_REGS.IF_ID.PC + 8;
		clear_IF_ID_REGS();
		return;
	}

	parsed_instruction_holder INSTRUCTION_HOLDER = get_holder(CURRENT_REGS.IF_ID.instruction);

	clear_ID_EX_REGS();
	if (INSTRUCTION_HOLDER.format == 1) { // R
		CURRENT_REGS.ID_EX.primary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rn];
		CURRENT_REGS.ID_EX.secondary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rm];
		//printf("This is the secondary data holder: %lx\n", CURRENT_REGS.ID_EX.secondary_data_holder);
		if (INSTRUCTION_HOLDER.opcode == 0x69B) {
			CURRENT_REGS.ID_EX.secondary_data_holder = INSTRUCTION_HOLDER.shamt;
		} else if (INSTRUCTION_HOLDER.opcode == 0x69A) {
			CURRENT_REGS.ID_EX.secondary_data_holder = 
				get_instruction_segment(16,21, CURRENT_REGS.IF_ID.instruction);
		}

		if (INSTRUCTION_HOLDER.opcode == BR) {
			BRANCH_STALL = 1;
		}
	} else if (INSTRUCTION_HOLDER.format == 2) { // I
	 	CURRENT_REGS.ID_EX.primary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rn];
	 	CURRENT_REGS.ID_EX.immediate = INSTRUCTION_HOLDER.ALU_immediate;
		
	} else if (INSTRUCTION_HOLDER.format == 3) { // D
		CURRENT_REGS.ID_EX.primary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rn];
		CURRENT_REGS.ID_EX.immediate = INSTRUCTION_HOLDER.DT_address;

		if (INSTRUCTION_HOLDER.opcode == STUR || INSTRUCTION_HOLDER.opcode == STURH ||
			INSTRUCTION_HOLDER.opcode == STURW || INSTRUCTION_HOLDER.opcode == STURB) {
			CURRENT_REGS.ID_EX.secondary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rt];
		}

	} else if (INSTRUCTION_HOLDER.format == 4) { // B
		CURRENT_REGS.ID_EX.immediate = sign_extend(INSTRUCTION_HOLDER.BR_address, 26, 2);
		BRANCH_STALL = 1;
	} else if (INSTRUCTION_HOLDER.format == 5) { // CB
		if ((INSTRUCTION_HOLDER.opcode >= 0x5A8 && INSTRUCTION_HOLDER.opcode <= 0x5AF) || 
			(INSTRUCTION_HOLDER.opcode >= 0x5A0 && INSTRUCTION_HOLDER.opcode <= 0x5A7)) {
			CURRENT_REGS.ID_EX.secondary_data_holder = CURRENT_STATE.REGS[INSTRUCTION_HOLDER.Rt];
		}

 		CURRENT_REGS.ID_EX.immediate = sign_extend(INSTRUCTION_HOLDER.COND_BR_address, 19, 2);
 		BRANCH_STALL = 1;
	} else if (INSTRUCTION_HOLDER.format == 6) { // IM/IW
		CURRENT_REGS.ID_EX.immediate = INSTRUCTION_HOLDER.MOV_immediate;
	}
	
	CURRENT_REGS.ID_EX.instruction = CURRENT_REGS.IF_ID.instruction;
	CURRENT_REGS.ID_EX.PC = CURRENT_REGS.IF_ID.PC;
}

void pipe_stage_fetch() {
	printf("Fetch -----------> ");
	if (BRANCH_STALL == 1) {
		//printf("STALLING\n");
		clear_IF_ID_REGS();
		BRANCH_STALL = 0;
		return;
	} else if (BUBBLE != 0) {
		//printf("BUBBLE\n");
		return;
	}


	if (FETCH_MORE != 0) {
		clear_IF_ID_REGS();
		CURRENT_REGS.IF_ID.instruction = mem_read_32(CURRENT_STATE.PC);
		print_operation(CURRENT_REGS.IF_ID.instruction);
		CURRENT_REGS.IF_ID.PC = CURRENT_STATE.PC;
		bp_predict();
		//CURRENT_STATE.PC += 4;
	} 
}