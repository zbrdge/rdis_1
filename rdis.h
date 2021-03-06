#ifndef _RDIS_H
#define _RDIS_H

#define RDMODE MODE_32

// NOTE: UGLY CASTS FROM DWORD* TO BYTE*

typedef struct basic_block {
	BYTE * addr;
	INSTRUCTION *inst;
	struct basic_block *prev, *next;
} BLOCK ;


typedef struct function {
	BLOCK *block;
	int edge;
	struct function *prev, *next;
} FUNCTION ;



typedef struct control_flow_graph {  // an adjacency list
	BLOCK *node;
	FUNCTION *neighbors;
	struct control_flow_graph *prev, *next;
} CFG ; 


// Prototypes.
void function_list_cleanup(
	FUNCTION *fl
);
FUNCTION * heuristic_dispatch( 
	char *filename, unsigned int entry
);
FUNCTION * allocate_function( 
);
FUNCTION * identify_functions(
	char *filename
);
BYTE *  start_addr_of(
	BLOCK *block 
);
BYTE *  get_target_of(
	INSTRUCTION *inst
);
BYTE *  get_region_start(
	void *f, int type
);
BYTE *  get_region_end(
	void *f, int type
);
BLOCK * get_block_of(
	INSTRUCTION *inst, DWORD *addr, FUNCTION *func
);
BLOCK * split_block(
	BLOCK *current, BYTE * addr
);
BLOCK * allocate_block(
);
BLOCK * disassemble(
	DWORD *addr, CFG *cfg, FUNCTION *func
);
int has_no_instructions(
	BLOCK *block
);
int block_add_instruction(
	BLOCK *block, INSTRUCTION *inst, BYTE * addr
);
int is_control_transfer(
	INSTRUCTION *inst
);
int is_branch(
	INSTRUCTION *inst
);
int connect_to(
	CFG *cfg,  BLOCK *block1,  BLOCK *block2
);
CFG * allocate_cfg(
);
unsigned int find_text_seg(
	char *filename;
);
INSTRUCTION * allocate_inst(
);



unsigned int find_text_seg( char *filename ) {
	// Why not use off_t for the entry point?
	// Would any file ever have an entry outside max off_t?
	return 0; // Later -- parse ELF/PE/Other Executable Types
}


FUNCTION * identify_functions( char *filename ) {

	FUNCTION *fl;

	unsigned int entry_point = find_text_seg(filename);

	fl = (FUNCTION *) heuristic_dispatch( filename, entry_point );

	// heuristic_local_calls( fl, end ); // search for calls in our text segment
	// heuristic_...
	// heuristic_elim_single( fl, end ); // only keep duplicates

	// Rewind
	while (fl->prev != NULL) 
		fl = fl->prev;

return fl;
}




BLOCK * disassemble( DWORD *addr, CFG *cfg, FUNCTION *func ) {
	BLOCK *block, *current;
        BYTE *target, *fstart, *fend;
	INSTRUCTION inst;
     

        fstart = get_region_start (func, 0);
        fend = get_region_end (func, 0);
	
	current = allocate_block ();
	current->addr = (BYTE *)addr;
 
        /* From the paper: */

	while ( addr < (DWORD *)fend ) {
		get_instruction (&inst, (BYTE *)addr, RDMODE);
	
		if ( (block=get_block_of(&inst, addr, func))!=NULL ) {
			
			if ( addr != (DWORD *)get_region_start(block, 1) ) {
				block = split_block(block, (BYTE *)addr);
				if (block == NULL) {
					perror("Address not actually in block.\n");
					return(NULL);
				}
			}
			
			if ( has_no_instructions(current) )
				return(block);
			
			else {
				connect_to(cfg, current, block);
				return(current);
			}
		} 
		
		else {
			block_add_instruction(current, &inst, (BYTE *)addr);
			
			if ( is_control_transfer(&inst) ) {
				target = get_target_of(&inst);
				
				if ((fstart <= target) && (target<fend)) {
					block = disassemble((DWORD *)target, cfg, func);
					connect_to(cfg, current, block);
				}
              
				if ( is_branch(&inst) ) {
					block = disassemble(addr+sizeof(INSTRUCTION), cfg, func);
					connect_to(cfg, current, block);
					return(current);
              			}
			} else
 				addr += sizeof(BYTE * );
      		}
	}

return current;
}




BYTE *  start_addr_of( BLOCK *block ) {

	BLOCK *ptr = block->prev;

	while ( ptr != NULL )
		ptr = ptr->prev;
		
return ptr->addr;
}





BYTE *  get_target_of( INSTRUCTION *inst ) {

	BYTE *immed;
	DWORD disp;

	if ( get_operand_type (&inst->op1) != OPERAND_TYPE_NONE ) { 
		get_operand_immediate ( &inst->op1, (DWORD *)immed );
		get_operand_displacement ( &inst->op1, &disp );
	}
   
return  immed += disp;
}




BYTE *  get_region_start( void *f, int type ) {
	BLOCK *bptr;
	FUNCTION *fptr = (FUNCTION *) f;

	if ( type == 0 ) {  // The region of code is a function
		while ( fptr->prev != NULL )
			fptr = fptr->prev;

		bptr = fptr->block;

		while ( bptr->prev != NULL ) 
			bptr = bptr->prev;

		return bptr->addr;

	} else { // The region of code is a block 

		bptr = (BLOCK *) f;

		while ( bptr->prev != NULL ) 
			bptr = bptr->prev;

		return bptr->addr;
	}
	
}





BYTE *  get_region_end( void *f, int type ) {
	BLOCK *bptr;
	FUNCTION *fptr = (FUNCTION *) f;

	if(!f)
	  return(NULL);
	

	if ( type == 0 ) {  // The region of code is a function
		while ( fptr->next != NULL )
			fptr = fptr->next;

		if (fptr->block != NULL)
			bptr = fptr->block;
		else
			return(NULL);

		while ( bptr->next != NULL ) 
			bptr = bptr->next;

		return bptr->addr;

	} else { // The region of code is a block 

		bptr = (BLOCK *) f;

		if (bptr!=NULL)
		while ( bptr->next != NULL ) 
			bptr = bptr->next;
		else
			return(NULL);

		return bptr->addr;
	}
}





BLOCK * get_block_of( INSTRUCTION *inst, DWORD *addr, FUNCTION *func ) { // get basic block by instruction or address

	BLOCK *bptr;
	FUNCTION *f = func;
	
	// Rewind
        while ( f->prev!=NULL )
		f = f->prev;
	bptr = f->block;
	
	if ( inst==NULL ) { // by address
	
		while ( f->next!=NULL ) {
			while ( bptr->next!=NULL ) {
				if ( bptr->addr == (BYTE *)addr )
					return bptr;
				else
					bptr = bptr->next;
			} 
		f = f->next;
		}
		
	} else if ( addr==NULL ) { // by instruction
	
		while ( f->next!=NULL ) {
			while ( bptr->next!=NULL ) {
				if ( bptr->inst == inst )
					return bptr;
				else
					bptr = bptr->next;
			} 
		f = f->next;
		}
	
	} else if ( (inst!=NULL) && (addr!=NULL) ) { // error
		return NULL;
	}
	
return NULL;
}


BLOCK * split_block( BLOCK *current, BYTE * addr ) {
	printf("split_block()\n");

	// Rewind
	while (current->prev != NULL)
		current = current->prev;

	// Find Addr in Block
	while (current->next != NULL) {
		if(current->addr != addr) {
			current = current->next;
			break;
		}
	}
	
	if (current->addr != addr) {
		perror("Error: Address not found in block.\n");	
		return NULL;
	}

	BLOCK *new_block = allocate_block();
   	new_block->addr = addr; 
	new_block->inst = current->inst;
	new_block->prev = NULL;
	new_block->next = current->next;
    
	current->prev->next = NULL;
    
	return(new_block);
}



int has_no_instructions ( BLOCK *block ) {
	BLOCK *bptr = block;
	// Rewind
	while ( bptr->prev != NULL )
		bptr = bptr->prev;
	
	while ( bptr->next != NULL ) {
		if ( bptr->inst != NULL )
			return 0;
		else
			bptr = bptr->next;
	}
return 1;
}



int block_add_instruction ( BLOCK *block, INSTRUCTION *inst, BYTE * addr ) {
	block->next = allocate_block();
	block->next->addr = addr;
	block->next->inst = inst;	
return 0;
}

int is_control_transfer ( INSTRUCTION *inst ) {
	switch ( inst->type ) {
		case INSTRUCTION_TYPE_JMP:
		case INSTRUCTION_TYPE_JMPC:
		case INSTRUCTION_TYPE_CALL:
			return 1;
		default:
			return 0;
	}
}

int is_branch ( INSTRUCTION *inst ) {
	switch ( inst->type ) {
		case INSTRUCTION_TYPE_JMP:
		case INSTRUCTION_TYPE_JMPC:
		case INSTRUCTION_TYPE_CALL:
			return 1;
		default:
			return 0;
	}
}

int connect_to (  CFG *cfg,  BLOCK *block1,  BLOCK *block2 ) {
	CFG *cptr, *head;
	cptr = cfg;


	if (cptr==NULL) {
		perror("Error, passed empty cfg.\n");
		return -1;
	}

	// Rewind
	while ( cptr->prev != NULL )
		cptr=cptr->prev;
	head = cptr;
	
	// Search for block1
	while ( cptr->node != block1 ) {
		if (cptr->next == NULL) { // block1 not found, allocate a node for it
			cptr->next = allocate_cfg();
			cptr->next->node = block1;
			cptr = cptr->next; 
			break;
		}
		cptr=cptr->next;
	}

	cptr->neighbors->next = allocate_function();
	cptr->neighbors->next->block = block2;		// Add block2 to block1's list of neighbor nodes
	
	// Rewind
	cptr = head;
	
	// Do the same for block2
	while ( cptr->node != block2 ) {
		if (cptr->next == NULL) {
			cptr->next = allocate_cfg();
			cptr->next->node = block2;
			cptr = cptr->next;
			break;
		}
		cptr=cptr->next;
	}
	
	cptr->neighbors->next = allocate_function();
	cptr->neighbors->next->block = block1;		// Add block1 to block2's list of neighbor nodes

return 0;
}


BLOCK * allocate_block() {
	BLOCK *b;
	if ( (b=malloc(sizeof(BLOCK))) == NULL ) {
		perror("Error allocating BLOCK structure..\n");
		return(NULL);
	} else {
		b->addr = NULL;
		b->prev = NULL;
		b->next = NULL;
		b->inst = NULL;
	}
return b;
}


FUNCTION * allocate_function() {
	FUNCTION *f;

	if ( (f=malloc(sizeof(FUNCTION))) == NULL ) {
		perror("Error allocating FUNCTION structure..\n");
		return(NULL);
	} else {
		f->block = NULL;
		f->edge = 0;
		f->prev = NULL;
		f->next = NULL;
	}

return f;
}


CFG * allocate_cfg() {
	CFG *c;

	if ( (c=malloc(sizeof(CFG))) == NULL ) {
		perror("Error allocating CFG structure..\n");
		return(NULL);
	} else {
		c->node = allocate_block();
		c->neighbors = allocate_function();
		c->prev = NULL;
		c->next = NULL;
	}
return c;
}



INSTRUCTION * allocate_inst() {
	INSTRUCTION * I;
	if ( (I = malloc(sizeof(INSTRUCTION))) == NULL) {
		perror("Error allocating INSTRUCTION structure..\n");
		return(NULL);
	}
	return I;
}



void function_list_cleanup( FUNCTION *fl ) {
	BLOCK *blk;
	while (fl->next != NULL) fl=fl->next;
	while (fl->prev != NULL) {
		blk = fl->block;
		while (blk->next != NULL) blk=blk->next;
		while (blk->prev != NULL) {
			if (blk->inst != NULL) free(blk->inst);
			blk=blk->prev;
		}
		if (blk->inst != NULL) free(blk->inst);
		fl = fl->prev;
		free(blk);
		free(fl->next);	
	}
	if (fl->block != NULL) {
		if (fl->block->inst != NULL)
			free(fl->block->inst);
		free(fl->block);
	}
	free(fl);
}

#endif
