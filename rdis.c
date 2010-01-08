/*
 *    rdis:
 *    -----
 *    Recursive disassembler using libdasm and methods from the paper
 *    "Static Disassembly of Obfuscated Binaries" by Kruegel, et al, 2004,
 *    Which can be found at:
 *
 *        http://www.usenix.org/events/sec04/tech/kruegel.html
 *
 *    An up to date copy of libdasm can be found at:
 *
 *        http://code.google.com/p/libdasm/ 
 *
 *    Copyright (C) 2009 Zack Breckenridge. BSD License.
 *
 *    To Do: 
 *    ------
 *       - Fix memory handling of large linked list
 *       - Add more heuristics for function search
 *       - Add graphviz output
 *       - Add ability to parse object/executable files
 *	 - Add block conflict resolution (the most important thing in the paper)
 *       - Add better memory handling (free_block(), free_cfg(), etc..)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libdasm.h>
#include "rdis.h"
#include "heuristics.h"


   
int main( int argc, char **argv ) {

	int format = FORMAT_INTEL;
	BLOCK *blk, *bptr;
	FUNCTION *fl;
	char str[256];
	struct stat sstat;


	/****************************************************************/ 
	if ( argc<2 ) {
		printf("Usage: %s <file> [-a|-i]\n"
			"  file    file to be disassembled (required)\n"
			"  -a      format: ATT (optional, default)\n"
			"  -i      format: INTEL (optional)\n",
		argv[0]);
		exit (1);
	}
	
	if (argc > 2)
		if (argv[2][0] == '-') {
			switch(argv[2][1]) {
				case 'a':
					format = FORMAT_ATT;
					break;
				case 'i':
					format = FORMAT_INTEL;
					break;
			}
		}
	
	/****************************************************************/ 

	fl = identify_functions( (char *)argv[1] );

	/*disassemble(fl->block->addr, cfg, fl);*/

	while (fl->prev != NULL) fl=fl->prev;

	while (1) {
		blk = fl->block;

		while (1) {
			get_instruction_string(blk->inst, format, (DWORD) 0, str, sizeof(str));
			printf("%08x:\t%s\t%x\n", (unsigned char)blk->addr, str, blk->inst->opcode);
			if (blk->next == NULL) break;
			else blk = blk->next;
		}
		if (fl->next == NULL) break; 
		else fl = fl->next;
	}

	function_list_cleanup( fl );
}
