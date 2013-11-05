/*
 * pipeline.c
 *
 *  Created on: Nov 4, 2013
 *      Author: jcrusen
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000


#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for Project 3 */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
} IDEXType;

typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
	int instr;
	int writeData;
} MEMWBType;

typedef struct WBENDStruct {
	int instr;
	int writeData;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
} stateType;

void printState(stateType *);

void initRegs(stateType *);
int getArg0(int);
int getArg1(int);
int getDest(int);
int getOff(int);
int convertNum(int);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);
void printInstruction(int instr);

int
main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read in the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {

        if (sscanf(line, "%d", state.instrMem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        state.dataMem[state.numMemory] = state.instrMem[state.numMemory];
        printf("memory[%d]=%d\n", state.numMemory, state.instrMem[state.numMemory]);
    }


    // begin my code
    stateType newState;
    initRegs(&state);
    state.pc = 0;






	while (1) {

	printState(&state);

	/* check for halt */
	if (opcode(state.MEMWB.instr) == HALT) {
		printf("machine halted\n");
		printf("total of %d cycles executed\n", state.cycles);
		exit(0);
	}

	newState = state;
	newState.cycles++;

	/* --------------------- IF stage --------------------- */

	/* --------------------- ID stage --------------------- */

	/* --------------------- EX stage --------------------- */

	/* --------------------- MEM stage --------------------- */

	/* --------------------- WB stage --------------------- */

	state = newState; /* this is the last statement before end of the loop.
			It marks the end of the cycle and updates the
			current state with the values calculated in this
			cycle */
	}

//    while(!halt){
//    	printState(&state);
//    	int mem = state.mem[state.pc];
//
//    	if(mem >> 22 == 6){ // halt
//    		halt = 1;
//    		state.pc++;
//    	}
//    	else if (mem >> 22 == 7){
//    		state.pc++;
//    	}
//    	else if(mem >> 22 == 0){ // add
//    		state.reg[getDest(mem)] = state.reg[getArg0(mem)] +
//    				state.reg[getArg1(mem)];
//    		state.pc++;
//    	}
//    	else if (mem >> 22 == 1){ // nand
//    		state.reg[getDest(mem)] = ~(state.reg[getArg0(mem)] &
//    				state.reg[getArg1(mem)]);
//    		state.pc++;
//    	}
//    	else if (mem >> 22 == 2){ //lw
//    		state.reg[getArg1(mem)] = state.mem[state.reg[getArg0(mem)] +
//    				getOff(mem)];
//    		state.pc++;
//    	}
//    	else if (mem >> 22 == 3){ //sw
//    		state.mem[state.reg[getArg0(mem)] + getOff(mem)] =
//    				state.reg[getArg1(mem)];
//    		state.pc++;
//    	}
//    	else if (mem >> 22 == 4){  //beq
//    		if (state.reg[getArg0(mem)] == state.reg[getArg1(mem)])
//    			state.pc = state.pc + 1 + getOff(mem);
//    		else
//    			state.pc++;
//    	}
//    	else if(mem >> 22 == 5){ //jalr
//    		state.reg[getArg1(mem)] = state.pc + 1;
//    		state.pc = state.reg[getArg0(mem)];
//    	}
//
//    	count++;
//    }
//    printState(&state);


    return(0);
}

int getArg0(int mem){
	mem = mem >> 19 & 7;
	return mem;
}

int getArg1(int mem){
	mem = mem >> 16 & 7;
	return mem; //convertNum(mem);
}

int getDest(int mem){
	mem = mem & 7;
	return mem; //convertNum(mem);
}

int getOff(int mem){
	mem = mem & 0xFFFF;
	mem = convertNum(mem);
	return mem;
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

void initRegs(stateType *statePtr){
	//set all registers to 0
	int i;
	for(i = 0; i < 8; i++){
		statePtr->reg[i] = 0;
	}
}

void
printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);

    printf("\tdata memory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
	    printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
    printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
    printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int
field0(int instruction)
{
	return( (instruction>>19) & 0x7);
}

int
field1(int instruction)
{
	return( (instruction>>16) & 0x7);
}

int
field2(int instruction)
{
	return(instruction & 0xFFFF);
}

int
opcode(int instruction)
{
	return(instruction>>22);
}

void
printInstruction(int instr)
{

	char opcodeString[10];

	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	} else if (opcode(instr) == NAND) {
		strcpy(opcodeString, "nand");
	} else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	} else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	} else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	} else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	} else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
    }
    printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
		field2(instr));
}
