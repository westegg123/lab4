/*
 * CMSC 22200
 * Helper Functions
 *
 */

#include "shell.h"
#include "helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

// Helper functions that get bits start_bit to end_bit, inclusive
uint32_t get_instruction_segment(int start_bit, int end_bit, uint32_t instruction) {
	uint32_t segment_mask = 0;
	for (int i = start_bit; i <= end_bit; i++) {
		segment_mask |= 1 << i;
	}
	return (instruction & segment_mask) >> start_bit;
}

uint32_t get_memory_segment(int start_bit, int end_bit, uint32_t data) {
	uint32_t segment_mask = 0; 
	for (int i = start_bit; i <= end_bit; i++) {
		segment_mask |= 1 << i;
	}
	return (data & segment_mask) >> start_bit;
}


// Purpose: Sign extend the address
uint64_t sign_extend(uint64_t address, uint32_t size_add, uint32_t end_fill) {
	uint64_t offset = 0;
	uint64_t extend = 0xFFFFFFFFFFFFFFFF;
	if (address >> (size_add-1) == 1) {
		extend = extend << (size_add + end_fill);
		offset = extend | (address << end_fill);
	} else {
		offset = address << end_fill;
	}
	return offset;
}

/* Converts two 32 uint32 types to one uint64 types
 * bit31_0 - bits 0 to 31
 * bit63_32 - bits 32 to 63
 */
uint64_t convert_64(uint32_t bit31_0, uint32_t bit63_32) {
	uint64_t bit1 = bit31_0;
	uint64_t bit2 = bit63_32;
	bit2 = (bit2 << 32) + bit1;
	return bit2;
}

/* Gets 64 bits from the memmory
 * add - lowest address
 */
uint64_t mem_read_64(uint64_t add) {
	uint32_t bit31_0 = mem_read_32(add);
	uint32_t bit63_32 = mem_read_32(add + 4);
	return convert_64(bit31_0, bit63_32);
}

/* Write 64 bits from the memmory
 * add - lowest address
 */
void mem_write_64(uint64_t add, uint64_t value) {
	uint64_t bit31_0 = value & 0xFFFFFFFF;
	uint64_t bit63_32 = (value & 0xFFFFFFFF00000000) >> 32;
	
	uint32_t b1 = bit31_0;
	uint32_t b2 = bit63_32;

	mem_write_32(add, b1);
	mem_write_32((add + 4), b2);
}

parsed_instruction_holder get_opcode(uint32_t instruction) {
	parsed_instruction_holder HOLDER;
	uint32_t code = get_instruction_segment(21,31, instruction);
	if ((code == 0x458) || (code == 0x459) || (code == 0x558) || (code == 0x559) || (code == 0x450) || (code == 0x750)
		|| (code == 0x650) || (code == 0X650) || (code == 0x550) || 
		(code == 0x69B) || (code == 0x69A) || (code == 0x658) || (code == 0x659) || 
		(code == 0x758) || (code == 0x759) || (code == 0x6B0) || (code == 0x4D8)) {
		// R instructions
		HOLDER.format = 1;
		HOLDER.opcode = code;
	} else if ((code == 0x588) || (code == 0x589) || (code == 0x488) || (code == 0x489) || (code == 0x688) ||
		(code == 0x788) || (code == 0x789)) {
		// I instructions
		HOLDER.format = 2;
		HOLDER.opcode = code;
	} else if ((code == 0x7C2) || (code == 0x5C2) || (code == 0x1C2) || (code == 0x3C2) || 
		(code == 0x7C0) || (code == 0x1C0) || (code == 0x3C0) || (code == 0x5C0)) {
		// D instructions
		HOLDER.format = 3;
		HOLDER.opcode = code;
	} else if ((code >= 0x0A0) && (code <= 0x0BF)) {
		// B instructions
		HOLDER.format = 4;
		HOLDER.opcode = code;
	} else if (((code >= 0x5A8) && (code <= 0x5AF)) || 
		((code >= 0x5A0) && (code <= 0x5A7)) || 
		((code >= 0x2A0) && (code <= 0x2A7))) {
		// CB instructions
		HOLDER.format = 5;
		HOLDER.opcode = code;
	} else if ((code >= 0x694 && code <= 0x697)) {
		// IW (IM?) instructions
		HOLDER.format = 6;
		HOLDER.opcode = code;
	}
	return HOLDER;
}

parsed_instruction_holder fill_holder(parsed_instruction_holder HOLDER, uint32_t instruction) {
	// R
	if (HOLDER.format == 1) {
		HOLDER.Rm = get_instruction_segment(16, 20, instruction);
		HOLDER.shamt = get_instruction_segment(10, 15, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	}
	// I
	if (HOLDER.format == 2) {
		HOLDER.ALU_immediate = get_instruction_segment(10, 21, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	}
	// D
	if (HOLDER.format == 3) {
		HOLDER.DT_address = get_instruction_segment(12, 20, instruction);
		HOLDER.op = get_instruction_segment(10, 11, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rt = get_instruction_segment(0, 4, instruction);
	}
	// B
	if (HOLDER.format == 4) {
		HOLDER.BR_address = get_instruction_segment(0, 25, instruction);
	}
	// CB
	if (HOLDER.format == 5) {
		HOLDER.COND_BR_address = get_instruction_segment(5, 23, instruction);
		HOLDER.Rt = get_instruction_segment(0, 4, instruction);
	}
	// IW (IM?)
	if (HOLDER.format == 6) {
		HOLDER.MOV_immediate = get_instruction_segment(5, 20, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	}
	return HOLDER;
}

parsed_instruction_holder get_holder(uint32_t instruction) {
		parsed_instruction_holder HOLDER;
	uint32_t code = get_instruction_segment(21,31, instruction);
	if ((code == 0x458) || (code == 0x459) || (code == 0x558) || (code == 0x559) || 
		(code == 0x450) || (code == 0x750) || (code == 0x650) || (code == 0X650) || 
		(code == 0x550) || (code == 0x69B) || (code == 0x69A) || (code == 0x658) ||
		(code == 0x659) || (code == 0x758) || (code == 0x759) || (code == 0x6B0) || 
		(code == 0x4D8)) {
		// R instructions
		HOLDER.format = 1;
		HOLDER.opcode = code;

		HOLDER.Rm = get_instruction_segment(16, 20, instruction);
		HOLDER.shamt = get_instruction_segment(10, 15, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	} else if ((code == 0x588) || (code == 0x589) || (code == 0x488) || (code == 0x489) || (code == 0x688) ||
		(code == 0x788) || (code == 0x789)) {
		// I instructions
		HOLDER.format = 2;
		HOLDER.opcode = code;

		HOLDER.ALU_immediate = get_instruction_segment(10, 21, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	} else if ((code == 0x7C2) || (code == 0x5C2) || (code == 0x1C2) || (code == 0x3C2) || 
		(code == 0x7C0) || (code == 0x1C0) || (code == 0x3C0) || (code == 0x5C0)) {
		// D instructions
		HOLDER.format = 3;
		HOLDER.opcode = code;

		HOLDER.DT_address = get_instruction_segment(12, 20, instruction);
		HOLDER.op = get_instruction_segment(10, 11, instruction);
		HOLDER.Rn = get_instruction_segment(5, 9, instruction);
		HOLDER.Rt = get_instruction_segment(0, 4, instruction);
	} else if ((code >= 0x0A0) && (code <= 0x0BF)) {
		// B instructions
		HOLDER.format = 4;
		HOLDER.opcode = code;

		HOLDER.BR_address = get_instruction_segment(0, 25, instruction);
	} else if (((code >= 0x5A8) && (code <= 0x5AF)) || 
		((code >= 0x5A0) && (code <= 0x5A7)) || 
		((code >= 0x2A0) && (code <= 0x2A7))) {
		// CB instructions
		HOLDER.format = 5;
		HOLDER.opcode = code;

		HOLDER.COND_BR_address = get_instruction_segment(5, 23, instruction);
		HOLDER.Rt = get_instruction_segment(0, 4, instruction);
	} else if ((code >= 0x694 && code <= 0x697)) {
		// IW (IM?) instructions
		HOLDER.format = 6;
		HOLDER.opcode = code;
		
		HOLDER.MOV_immediate = get_instruction_segment(5, 20, instruction);
		HOLDER.Rd = get_instruction_segment(0, 4, instruction);
	}
	return HOLDER;
}

void print_instr(parsed_instruction_holder HOLDER) {
	printf("------------\n");
	if (HOLDER.format == 1) {
		printf("R instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	Rm: %x \n", HOLDER.Rm);
		printf("	shamt %x \n", HOLDER.shamt);
		printf("	Rn: %x \n", HOLDER.Rn);
		printf("	Rd: %x \n", HOLDER.Rd);
	} else if (HOLDER.format == 2) {
		printf("I instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	ALU immediate: %x \n", HOLDER.ALU_immediate);
		printf("	Rn: %d \n", HOLDER.Rn);
		printf("	Rd: %x \n", HOLDER.Rd);
	} else if (HOLDER.format == 3) {
		printf("D instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	DR address: %x \n", HOLDER.DT_address);
		printf("	Rn: %x \n", HOLDER.Rn);
		printf("	Rt: %x \n", HOLDER.Rt);
	} else if (HOLDER.format == 4) {
		printf("B instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	BR address: %x \n", HOLDER.BR_address);
	} else if (HOLDER.format == 5) {
		printf("CB instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	COND BR address: %x \n", HOLDER.COND_BR_address);
		printf("	Rt: %x \n", HOLDER.Rt);
	} else if (HOLDER.format == 6) {
		printf("IW instruction:\n");
		printf("	Opcode: %x \n", HOLDER.opcode);
		printf("	MOV immediate: %x \n", HOLDER.MOV_immediate);
		printf("	Rd: %x \n", HOLDER.Rd);
	} 
}



void print_operation(uint32_t instruction) {
	if (!instruction) {
		printf("NULL\n");
		return;
	}

	parsed_instruction_holder HOLDER = get_holder(instruction);
	// HLT
	if (instruction == 0xd4400000) {
		printf("HLT\n");
	} else if (HOLDER.opcode == 0x458 || HOLDER.opcode == 0x459) {
		printf("ADD: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode >= 0x488 && HOLDER.opcode <= 0x489) {
		printf("ADDI: Rd: %d, ALU: %x\n", HOLDER.Rd, HOLDER.ALU_immediate);
	} else if (HOLDER.opcode == 0x558 || HOLDER.opcode == 0x559) {
		printf("ADDS: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode >= 0x588 && HOLDER.opcode <= 0x589) {
		printf("ADDIS: Rd: %d, ALU: %x\n", HOLDER.Rd, HOLDER.ALU_immediate);
	} else if (HOLDER.opcode >= 0x5A8 && HOLDER.opcode <= 0x5AF) {
		printf("CBNZ: Rt: %d COND BR address: %x \n", HOLDER.Rt, HOLDER.COND_BR_address);
	} else if (HOLDER.opcode >= 0x5A0 && HOLDER.opcode <= 0x5A7) {
		printf("CBZ: Rt: %d COND BR address: %x \n", HOLDER.Rt, HOLDER.COND_BR_address);
	} else if (HOLDER.opcode == 0x450) {
		printf("AND: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x750) {
		printf("ANDS: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x650) {
		printf("EOR: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x550) {
		printf("ORR: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x7C2) {
		printf("LDUR 64 BITS: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x5C2) {
		printf("LDUR 32 BITS: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x1C2) {
		printf("LDURB: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x3C2) {
		printf("LDURH: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	}  else if (HOLDER.opcode == 0x69B) {
		if (get_instruction_segment(10,15, instruction) == 0x3f) {
			printf("LSL BUT LSR\n");
		} else {
			printf("LSL AND LSL\n");
		}
	} else if (HOLDER.opcode == 0x69A) {
		if (get_instruction_segment(10,15, instruction) != 0x3F) {
			printf("LSL BUT LSR\n");
		} else {
			printf("LSR AND LSR\n");
		}
	} else if (HOLDER.opcode >= 0x694 && HOLDER.opcode <= 0x697) {
		printf("MOVZ: MOV_immediate: %x, Rd: %d\n", HOLDER.MOV_immediate, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x7C0) {
		printf("STUR: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x1C0) {
		printf("STURB: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x3C0) {
		printf("STURH: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x5C0) {
		printf("STURW: DR address: %x, Rn: %d\n", HOLDER.DT_address, HOLDER.Rn);
	} else if (HOLDER.opcode == 0x658 || HOLDER.opcode == 0x659) {	
		printf("SUB: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x688 || HOLDER.opcode == 0x689) {
		printf("SUBI: Rd: %d, ALU: %x\n", HOLDER.Rd, HOLDER.ALU_immediate);
	} else if (HOLDER.opcode == 0x758 || HOLDER.opcode == 0x759) {
		printf("SUBS: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x788 || HOLDER.opcode == 0x789) {
		printf("SUBIS: Rd: %d, ALU: %x\n", HOLDER.Rd, HOLDER.ALU_immediate);
	} else if (HOLDER.opcode == 0x4D8) {
		printf("MUL: Rn: %d, Rd: %d\n", HOLDER.Rn, HOLDER.Rd);
	} else if (HOLDER.opcode == 0x6B0) {
		printf("BR: \n");
	} else if (HOLDER.opcode >= 0x0A0 && HOLDER.opcode <= 0x0BF) {
		printf("B\n");
	} else if (HOLDER.opcode >= 0x2A0 && HOLDER.opcode <= 0x2A7) {
		uint32_t cond = (HOLDER.Rt & 14) >> 1;
		if (cond == 0) {
			printf("HANDLING BEQ or BNE\n");
		} else if (cond == 5) {
			printf("HANDLING BGE or BLT\n");
		} else if (cond == 6) {
			printf("HANDLING BGT or BLE\n");
		}
	}

}