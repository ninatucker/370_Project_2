/*
 * Instruction-level simulator for the LC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7

#define MAXSETS 256
#define MAXBLOCKSPERSET 256
#define MAXSIZEINWORDS 8192

typedef struct WordStruct {
    int tag;
    int data;
    int dirty;
    int lastUsed;
} WordType;

WordType cache[MAXSETS][MAXBLOCKSPERSET][MAXSIZEINWORDS];
int lru[MAXSETS][MAXBLOCKSPERSET];
int COUNT = 0;

int BLOCKSIZE = 0;
int NUMSETS = 0;
int BLOCKSPERSET = 0;

int SIZEINWORDS = 0;
int SIZEINBLOCKS = 0;
int SIBITS = 0;
int BLOCKOFFSETBITS = 0;
//SIZE: the total number of words in the cache is
//       blockSizeInWords * numberOfSets * blocksPerSet

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *);
void run(stateType);
int convertNum(int);
double log2 (double x);
int getSetIndex(int word);
int getBlockOff(int word);
int getTag(int address);
int load(int address);
void store(int address, int data);

enum actionType
        {cacheToProcessor, processorToCache, memoryToCache, cacheToMemory,
        cacheToNowhere};
/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *     cacheToProcessor: reading data from the cache to the processor
 *     processorToCache: writing data from the processor to the cache
 *     memoryToCache: reading data from the memory to the cache
 *     cacheToMemory: evicting cache data by writing it to the memory
 *     cacheToNowhere: evicting cache data by throwing it away
 */
void printAction(int address, int size, enum actionType type);


int
main(int argc, char *argv[])
{
    int i;
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 5) {
	printf("error: usage: missing arg val or missing file %s "
			"<machine-code file>\n", argv[0]);
	exit(1);
    }

    /* initialize memories and registers */
    for (i=0; i<NUMMEMORY; i++) {
	state.mem[i] = 0;
    }
    for (i=0; i<NUMREGS; i++) {
	state.reg[i] = 0;
    }

    state.pc=0;

    /* read machine-code file into instruction/data memory (starting at
	address 0) */

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
	printf("error: can't open file %s\n", argv[1]);
	perror("fopen");
	exit(1);
    }

    for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
	state.numMemory++) {
	if (state.numMemory >= NUMMEMORY) {
	    printf("exceeded memory size\n");
	    exit(1);
	}
	if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
	    printf("error in reading address %d\n", state.numMemory);
	    exit(1);
	}
	printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    printf("\n");
    

//SET GLOBALS FROM ARGS, ARGS PRESENT CHECKED ABOVE
    BLOCKSIZE = atoi(argv[2]);
    NUMSETS = atoi(argv[3]);
    BLOCKSPERSET = atoi(argv[4]);
    SIZEINWORDS = BLOCKSIZE * NUMSETS * BLOCKSPERSET;
    SIZEINBLOCKS = SIZEINWORDS / BLOCKSIZE; //CHECK THIS,
    SIBITS = log2(SIZEINBLOCKS/ BLOCKSPERSET);
    BLOCKOFFSETBITS = log2(BLOCKSIZE);

    /* run never returns */
    int s;
    int j;
    int k;
    for(s = 0; s < NUMSETS; s++){
    	for(j = 0; j < BLOCKSPERSET; j++){
    		for(k = 0; k < BLOCKSIZE; k++){
    			cache[s][j][k].dirty = 0;
    			cache[s][j][k].lastUsed = -1;
    			cache[s][j][k].tag = 0;
    		}
    	}
    }

    run(state);

    return(0);
}

void
run(stateType state)
{
    int arg0, arg1, arg2, addressField;
    int instructions=0;
    int opcode;
    int maxMem=-1;	/* highest memory address touched during run */

    for (; 1; instructions++) { /* infinite loop, exits when it executes halt */
	printState(&state);

	if (state.pc < 0 || state.pc >= NUMMEMORY) {
	    printf("pc went out of the memory range\n");
	    exit(1);
	}

	maxMem = (state.pc > maxMem)?state.pc:maxMem;

	/* this is to make the following code easier to read */
	opcode = state.mem[state.pc] >> 22;
	arg0 = (state.mem[state.pc] >> 19) & 0x7;
	arg1 = (state.mem[state.pc] >> 16) & 0x7;
	arg2 = state.mem[state.pc] & 0x7; /* only for add, nand */

	addressField = convertNum(state.mem[state.pc] & 0xFFFF); /* for beq,
								    lw, sw */
	state.pc++;
	if (opcode == ADD) {
	    state.reg[arg2] = state.reg[arg0] + state.reg[arg1];
	} else if (opcode == NAND) {
	    state.reg[arg2] = ~(state.reg[arg0] & state.reg[arg1]);
	} else if (opcode == LW) {
	    if (state.reg[arg0] + addressField < 0 ||
		    state.reg[arg0] + addressField >= NUMMEMORY) {
		printf("address out of bounds\n");
		exit(1);
	    }
	    state.reg[arg1] = state.mem[state.reg[arg0] + addressField];
	    if (state.reg[arg0] + addressField > maxMem) {
		maxMem = state.reg[arg0] + addressField;
	    }
	} else if (opcode == SW) {
	    if (state.reg[arg0] + addressField < 0 ||
		    state.reg[arg0] + addressField >= NUMMEMORY) {
		printf("address out of bounds\n");
		exit(1);
	    }
	    state.mem[state.reg[arg0] + addressField] = state.reg[arg1];
	    if (state.reg[arg0] + addressField > maxMem) {
		maxMem = state.reg[arg0] + addressField;
	    }
	} else if (opcode == BEQ) {
	    if (state.reg[arg0] == state.reg[arg1]) {
		state.pc += addressField;
	    }
	} else if (opcode == JALR) {
	    state.reg[arg1] = state.pc;
            if(arg0 != 0)
 		state.pc = state.reg[arg0];
	    else
		state.pc = 0;
	} else if (opcode == NOOP) {
	} else if (opcode == HALT) {
	    printf("machine halted\n");
	    printf("total of %d instructions executed\n", instructions+1);
	    printf("final state of machine:\n");
	    printState(&state);
	    exit(0);
	} else {
	    printf("error: illegal opcode 0x%x\n", opcode);
	    exit(1);
	}
        state.reg[0] = 0;
    }
}

int load(int address){
	int data;
	return data;
}

void store(int address,  int data){
	int si = getSetIndex(address);
	int bl = getBlockOff(address);
	int tag = getTag(address);
	int found = 0;
	int empty = 0;
	int leastI = 0; int leastJ = 0; int leastK = 0;
	int lastUsed = -1;
	int i, j, k;
	//for(i = si; i < bl; i++){
	i = si;
		for(j = 0; j < BLOCKSPERSET; j++){
			for(k = 0; k < BLOCKSIZE; k++){
				//if it is in the cache just mark dirty
				if(cache[i][j][k].tag == tag && cache[i][j][k].lastUsed != -1){
					cache[i][j][k].dirty = 1;
					cache[i][j][k].lastUsed = COUNT;
					found = 1;
				}
				else if(cache[i][j][k].lastUsed == -1){
					leastI = i;
					leastJ = j;
					leastK = k;
					empty = 1;
				}
				else if(cache[i][j][k].lastUsed < lastUsed){
					leastI = i;
					leastJ = j;
					leastK = k;
				}
			}
		}
	//}

	if(!found && empty){
		printAction(address - bl, BLOCKSIZE, memoryToCache);
		//SET ENTIRE BLOCK NEEDED HERE
		printAction(address, 1, processorToCache);
		cache[leastI][leastJ][leastK].tag = tag;
		cache[leastI][leastJ][leastK].lastUsed = COUNT;
		cache[leastI][leastJ][leastK].dirty = 1;
	}
	else if(!found && checkDirty(address)){
		cache[leastI][leastJ][leastK].tag = tag;
		cache[leastI][leastJ][leastK].dirty = 1;
		cache[leastI][leastJ][leastK].lastUsed = COUNT;
		printAction(address, 1, processorToCache);
		//need EVICT RANGE FUNCTION HERE AS WELL, AND RANGE SET BLOCK
	}
	else if(!found){//ie not dirty and not found
		printAction(address - bl, BLOCKSIZE, memoryToCache);
		//NOT CORRECT, NEED WAY TO SET ENTIRE BLOCK TO ADDRESS RANGE
		// also need EVICT function to remove entire block
	}



}

int checkDirty(int address){
	int si = getSetIndex(address);
	int bl = getBlockOff(address);
	int tag = getTag(address);
	int dirty = 0;
	int i, j, k;
	for(i = si; i < bl; i++){
		for(j = 0; j < BLOCKSPERSET; j++){
			for(k = 0; k < BLOCKSIZE; k++){
				//if it is in the cache just mark dirty
				if(cache[i][j][k].dirty){
					dirty = 1;
				}
			}
		}
	}
	if(dirty){
		printAction(address - bl, BLOCKSIZE, cacheToMemory);
	}
	return dirty;
}

double log2 (double x){
   return log(x)/log(2.0);
}

int getSetIndex(int word){
	return ((word >> BLOCKOFFSETBITS) & SIBITS);
}

int getBlockOff(int word){
	return(word & BLOCKOFFSETBITS);
}

void
printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
	    printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
    printf("end state\n");
}



int
convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Sun integer */
    if (num & (1<<15) ) {
	num -= (1<<16);
    }
    return(num);
}

int getTag(int address){
	return((address/BLOCKSIZE) /NUMSETS);
}

void
printAction(int address, int size, enum actionType type)
{
    printf("@@@ transferring word [%d-%d] ", address, address + size - 1);
    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    } else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    } else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    } else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    } else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}
