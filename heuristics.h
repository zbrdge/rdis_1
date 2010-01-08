#ifndef _RDIS_HEURISTICS_H
#define _RDIS_HEURISTICS_H


#include <pthread.h>


typedef struct thread_args {
	FUNCTION *fl;
	unsigned char *data;
	int flen;
	unsigned int entry;
} THREAD_ARGS;


/*
 *  Simplified heuristic prologue:
 *  ------------------------------
 *    Since the goal will ultimately be to read the entire 
 *    text segment of the binary into a FUNCTION/BLOCK
 *    linked list, simply allocate a new FUNCTION for each
 *    _potential_ prologue point: PUSH EBP and ENTER (more?).
 *    Later in other passes, read more instructions and
 *    use some simple linked list sorting routines to
 *    rearrange and destroy as appropriate.
 */
void *heuristic_prologue_pass1 ( THREAD_ARGS *thread_args ) {

	// Set up the thread arguments
	FUNCTION *fl = thread_args->fl;
	unsigned char *data = thread_args->data;
	int flen = thread_args->flen, push=0;
	unsigned int entry = thread_args->entry;

	short len = 0;
	unsigned int count = 0;

	for( ;count<flen; count+=len) { 

		fl->block = allocate_block();
		fl->block->inst = allocate_inst();

		len = get_instruction(fl->block->inst, data+count, RDMODE);
		
		if(fl->block->inst->type == INSTRUCTION_TYPE_PUSH) {
			// According to libdasm, INSTRUCTION_TYPE_PUSH includes
			// enter, pusha, and pushf, so for now, no need to test for others.
			if ((count+len)>=flen) break; // Did we just read the last PUSH? No more allocation.
			fl->next = allocate_function();
			fl->next->prev = fl;
			fl = fl->next;
		} else if (fl->block->inst->type != INSTRUCTION_TYPE_PUSH) {
			free(fl->block->inst);
			free(fl->block);
		}

	}

free(thread_args->data);
free(thread_args);
pthread_exit(NULL);
}


FUNCTION * heuristic_dispatch ( char *filename, unsigned int entry ) {

    FILE *fp;
    struct stat sstat;
    int i=0, sublen, NUM_THREADS=20;
    FUNCTION *fl[NUM_THREADS], *comp;
    pthread_t threads[NUM_THREADS];
    THREAD_ARGS *targs[NUM_THREADS];
    pthread_attr_t attr;


    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

     
    if ( (fp=fopen(filename, "r+b")) == NULL  ) {
	fprintf(stderr, "Error: fopen: heuristic_dispatch()\n");
	exit(1);
    }

    if ( stat(filename, &sstat)  == -1  ) {
	fprintf(stderr, "Error: stat: heuristic_dispatch()\n");
	exit(1);
    }

    // Position the file pointer at the actual entry point when you have it.
    //
    rewind(fp);


    // Partition the file into data to be read by each thread.
    // Note also that since we don't know where the last instruction
    // ends in each data block (we may be cutting it off), it may be 
    // a good idea to overlap data blocks somehow, and somehow
    // use the same block conflict resolution method from the paper.
    //
    if ( sstat.st_size > 100 )
	sublen = sstat.st_size / NUM_THREADS; 
    else { // small ammount of data to parse, use single thread
	NUM_THREADS=1;
	sublen = sstat.st_size;
    }


    for ( ; i < NUM_THREADS; i++ ) {

	fl[i] = allocate_function();

	if ( (targs[i]=malloc(sizeof(THREAD_ARGS))) == NULL ) {
		fprintf(stderr, "Error: malloc: heuristic_dispatch()\n");
		exit(1);
	}
	if ( (targs[i]->data=malloc(sublen)) == NULL ) {
		fprintf(stderr, "Error: malloc[2]: heuristic_dispatch()\n");
		exit(1);
	} 
	if ( (fread(targs[i]->data, sublen, 1, fp)) != 1 ) {
		fprintf(stderr, "Error: fread: heuristic_dispatch()\n");
		exit(1);
	}
	targs[i]->entry = 0;
	targs[i]->flen = sublen;
	targs[i]->fl = fl[i];

	if(pthread_create(&threads[i], &attr, (void *)heuristic_prologue_pass1, targs[i])) {
		fprintf(stderr, "Error: pthread_create: heuristic_dispatch()\n");
		exit(1);
	}
    } // clean up attr
    pthread_attr_destroy(&attr);

    
    fclose(fp);


    // Wait for every thread to finish and...
    //
    for ( i=0; i < NUM_THREADS; i++ ) {
	if(pthread_join(threads[i], NULL)) {
		fprintf(stderr, "Error: pthread_join: heuristic_dispatch()\n");
		exit(1);
	}
    }
    // ...link all their function lists.
    //
    for ( i=0; i < NUM_THREADS-1; i++ ) {
	comp = fl[i];
	while(comp->next != NULL) comp=comp->next;
	comp->next = fl[i+1];
	fl[i+1]->prev = comp;
    }


return(fl[0]);
}


void heuristic_local_calls( FUNCTION *fl ) {
}

void heuristic_elim_single( FUNCTION *fl ) {
}

#endif
