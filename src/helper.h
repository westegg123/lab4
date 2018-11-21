#ifndef _SIM_HELPER_H_
#define _SIM_HELPER_H_
#include <inttypes.h>

typedef struct parsed_instruction_holder {
	int format;
	uint32_t opcode, Rm, shamt, Rn, Rd, Rt, ALU_immediate, DT_address, op, BR_address, COND_BR_address, MOV_immediate;
} parsed_instruction_holder;

uint32_t get_instruction_segment(int start_bit, int end_bits, uint32_t instruction);
uint32_t get_memory_segment(int start_bit, int end_bit, uint32_t data);

uint64_t sign_extend(uint64_t address, uint32_t size_add, uint32_t end_fill);

uint64_t convert_64(uint32_t bit31_0, uint32_t bit63_32);
uint64_t mem_read_64(uint64_t add);
void mem_write_64(uint64_t add, uint64_t value);


// DECODE THE INSTRUCTION FUNCTIONS
parsed_instruction_holder get_opcode(uint32_t instruction);
parsed_instruction_holder fill_holder(parsed_instruction_holder stuff, uint32_t instruction);
parsed_instruction_holder get_holder(uint32_t instruction);

// PRINT HOLDER
void print_instr(parsed_instruction_holder HOLDER);
void print_operation(uint32_t instruction);

#endif