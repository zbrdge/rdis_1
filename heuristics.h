#ifndef _RECDASM_HEURISTICS_H
#define _RECDASM_HEURISTICS_H

/*
 * Search for common function prologues:
 *   a) push %ebp; mov %esp,%ebp; sub $n, %esp
 *   b) enter $n, $0
 *
 * In this function, the starting address is passed in the FUNCTION
 * structure: the first addr in the first block.
 * 
 */
void heuristic_prologue( FUNCTION *fl, BYTE *end ) {
	BLOCK *blk;
	int instret;


	while (fl->block->addr < end) {

		blk=fl->block;

		if (!(instret = get_instruction(blk->inst, blk->addr, RDMODE))) {
			perror ("Unable to get instruction at address.");
			exit;
		}

		blk->next = allocate_block(blk->addr + instret);
		blk = blk->next;

		if ( (blk->inst->type == INSTRUCTION_TYPE_PUSH)  && (blk->inst->op1.reg == REGISTER_EBP) ) {

			if (!(instret = get_instruction(blk->inst, blk->addr, RDMODE))) {
				perror ("Unable to get instruction at address.");
				exit;
			}
			
			blk->next = allocate_block(blk->addr + instret);
			blk = blk->next;

			printf("PUSH.\n");
			
		if ( (blk->inst->type == INSTRUCTION_TYPE_MOV) && (blk->inst->op2.reg == REGISTER_ESP) && (blk->inst->op1.reg == REGISTER_EBP)) {

				if (!(instret = get_instruction(blk->inst, blk->addr, RDMODE))) {
					perror ("Unable to get instruction at address.");
					exit;
				}

				blk->next = allocate_block(blk->addr + instret);
				blk = blk->next;
				
				printf("MOV\n");

		if ( (blk->inst->type == INSTRUCTION_TYPE_SUB) && (blk->inst->op2.reg == REGISTER_ESP)) {
		
				fl->next = allocate_function(blk->addr + instret);
				fl = fl->next;
				continue;
			
				printf("PROLOGUE.\n");

		} else  fl->block->addr = blk->addr;  // SUB
		} else  fl->block->addr = blk->addr;  // MOV
		}

			/***** LOOK for ENTER
			if ( inst.type == INSTRUCTION_TYPE_ENTER ) {
			if ((inst.op1.type==OPERAND_TYPE_IMMEDIATE)&&(inst.op2.type==OPERAND_TYPE_IMMEDIATE)) {
		
				fl->next = allocate_function(blk->addr + instret);
				fl = fl->next;
				continue;

			} }

			***** AT some POINT ***/

		else { // Not the start of a basic block
				if (fl->next == NULL) { 
					fl->next = allocate_function(blk->addr + instret);
					fl->next->block = allocate_block(blk->addr + instret);
				}
				fl = fl->next;
				continue; 
					
		}		
	}

}

void heuristic_local_calls( FUNCTION *fl ) {
}

void heuristic_elim_single( FUNCTION *fl ) {
}

#endif
