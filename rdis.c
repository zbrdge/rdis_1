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


   
unsigned char * read_file(int *len, char *name);

int main( int argc, char **argv ) {
	BYTE *data, *end;
	int format = FORMAT_ATT, size, i;
	BLOCK *blk, *bptr;
	FUNCTION *fl;
	char str[256];

	CFG *cfg = allocate_cfg();

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
		
	data = read_file(&size, argv[1]);
	end =  data + size;

	fl = identify_functions(data, end);
	disassemble(fl->block->addr, cfg, fl);

	while (fl->next != NULL) {
		blk = fl->block;
		while (blk->next != NULL) {
			get_instruction_string(blk->inst, format, (DWORD)(blk->addr - data), str, sizeof(str));
			printf("%08x:\t%s\n",blk->addr, str);
			blk = blk->next;
		}
		fl = fl->next;
	}
}


/*
 *    From das.c, libdasm sample:
 */
unsigned char * read_file(int *len, char *name) {
        char            *buf;
        FILE            *fp;
        int             c;
        struct stat     sstat;

        if ((fp = fopen(name, "r+b")) == NULL) {
                fprintf(stderr,"Error: unable to open file \"%s\"\n", name);
                exit(0);
        }

        /* Get file len */
        if ((c = stat(name, &sstat)) == -1) {
                fprintf(stderr,"Error: stat\n");
                exit (1);
        }
        *len = sstat.st_size;

        /* Allocate space for file */
        if (!(buf = (char *)malloc(*len))) {
                fprintf(stderr,"Error: malloc\n");
                exit (1);
        }

        /* Read file in allocated space */
        if ((c = fread(buf, 1, *len, fp)) != *len) {
                fprintf(stderr,"Error: fread\n");
                exit (1);
        }

        fclose(fp);

        return (buf);
}
