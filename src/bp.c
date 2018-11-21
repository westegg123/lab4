/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 * Gushu Li and Reza Jokar
 */
#include "helper.h"
#include "pipe.h"
#include "bp.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

extern bp_t BP;
extern CPU_State CURRENT_STATE;
extern int branch_taken;

uint32_t get_8_pc_bits(uint32_t aPC) {
	return get_instruction_segment(2, 9, aPC);
}

uint32_t get_BTB_index(uint32_t aPC) {
	return get_instruction_segment(2, 11, aPC);
}

void update_GHR(int aIsConditional, int aBranchTaken) {
	if (aIsConditional == 0) {
		return;
	}

	printf("UPDATE GHR\n");
	if (aBranchTaken == 1) /*taken*/ {
		BP.gshare.GHR = ((BP.gshare.GHR << 1) | 1);
	} else if (aBranchTaken == 0) /*not taken*/ {
		BP.gshare.GHR = ((BP.gshare.GHR << 1));
	}
}

void update_PHT(uint32_t aPC, int aIsConditional) {
	if (aIsConditional == 0) {
		return;
	}

	printf("UPDATE PHT\n");
	uint32_t myPHTIndex = (get_8_pc_bits(aPC) ^ BP.gshare.GHR); 
	if (branch_taken == 1) {
		if (BP.gshare.PHT[myPHTIndex] < 3) {
			BP.gshare.PHT[myPHTIndex] += 1;
		}
	} else if (branch_taken == 0) {
		if (BP.gshare.PHT[myPHTIndex] > 0 ) {
			BP.gshare.PHT[myPHTIndex] -= 1;
		}
	}
}

void update_BTB(uint64_t aAddressTag, uint64_t aTargetBranch, int aIsConditional, int aIsValid) {
	printf("UPDATE BTB\n");
	uint32_t myBTBIndex = get_BTB_index(aAddressTag);
	BP.BTB[myBTBIndex].branch_target = aTargetBranch;
	BP.BTB[myBTBIndex].address_tag = aAddressTag;
	BP.BTB[myBTBIndex].unconditional = (!aIsConditional);
	BP.BTB[myBTBIndex].valid = aIsValid;
}

int should_take_branch(int aSaturatingCounter) {
	if (aSaturatingCounter == 3 || aSaturatingCounter == 2) {
		return 1;
	}
	return 0;
}

// void bp_predict(CPU_State *aCPU_State, bp_t *aBP) {
//     /* Predict next PC */
//     uint64_t myPCPrediction = aCPU_State->PC + 4;
//     uint32_t myBTB_index = get_BTB_index(aCPU_State);
//     gshare_t myGshare = aBP->gshare;
//     if (aBP->BTB[myBTB_index].branch_target != 0) {
//     	if ((aBP->BTB[myBTB_index].unconditional == 1) || should_take_branch(myGshare.PHT[myGshare.GHR ^ aCPU_State->PC])) {
//     		myPCPrediction = aBP->BTB[myBTB_index].branch_target;
//     	}
//     }
//     printf("PREDICTING: %x\n", myPCPrediction);
//     aCPU_State->PC = myPCPrediction;
// }

void bp_predict() {
    uint64_t myPCPrediction = CURRENT_STATE.PC + 4;
    uint32_t myBTB_index = get_BTB_index(CURRENT_STATE.PC);
    gshare_t myGshare = BP.gshare;
    if (BP.BTB[myBTB_index].branch_target != 0) {
    	if ((BP.BTB[myBTB_index].unconditional == 1) || should_take_branch(myGshare.PHT[myGshare.GHR ^ get_8_pc_bits(CURRENT_STATE.PC)])) {
    		myPCPrediction = BP.BTB[myBTB_index].branch_target;
    	}
    }
    printf("PREDICTING: %x\n", myPCPrediction);
    CURRENT_STATE.PC = myPCPrediction;
}

void bp_update(uint64_t aAddressTag, uint64_t aTargetBranch, int aIsConditional, int aIsValid, int aBranchTaken) {
    /* Update BTB */
	update_BTB(aAddressTag, aTargetBranch, aIsConditional, aIsValid);

    /* Update gshare directional predictor */
    update_PHT(aAddressTag, aIsConditional);

    /* Update global history register */
    update_GHR(aIsConditional, aBranchTaken);

}
