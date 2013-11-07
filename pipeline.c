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
//int getDest(int);
int getOff(int);
int convertNum(int);
int field0(int instruction);
int field1(int instruction);
int field2(int instruction);
int opcode(int instruction);
void printInstruction(int instr);
int checkDataHaz(const stateType *state, stateType *newState);
int lwStall(const stateType *state);
int getDest(int instr);

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


    // Initialize
    stateType newState;
    initRegs(&state);
    state.pc = 0;
    state.cycles = 0;
    state.IFID.instr = NOOPINSTRUCTION;
    state.IDEX.instr = NOOPINSTRUCTION;
    state.EXMEM.instr = NOOPINSTRUCTION;
    state.MEMWB.instr = NOOPINSTRUCTION;
    state.WBEND.instr = NOOPINSTRUCTION;
    // end Initialize

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

	if(!lwStall(&state)){
	//if(state.EXMEM.branchTarget == 0){
		newState.IFID.pcPlus1 = state.pc + 1;
		newState.pc = state.pc + 1;
	//}
	//else {
	//	newState.IFID.pcPlus1 = state.EXMEM.branchTarget;
	//	newState.pc = state.EXMEM.branchTarget;
	//}

	//if(opcode(state.instrMem[state.pc]) >= 0 //probably remove ifelse
		//	&& opcode(state.instrMem[state.pc]) <= 7)
	newState.IFID.instr = state.instrMem[state.pc];//just leave this line
	//else
		//newState.IFID.instr = NOOPINSTRUCTION;
	//newState.IFID.pcPlus1 = state.pc + 1;

	}
	else{
		newState.IFID.instr = NOOPINSTRUCTION;
	}


	/* --------------------- ID stage --------------------- */

	//if(!lwStall(&state)){
	newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
	newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
	newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
	newState.IDEX.offset = convertNum(field2(state.IFID.instr));
	newState.IDEX.instr = state.IFID.instr;
	//}
	//else{
	//	newState.IDEX.instr = NOOPINSTRUCTION;
//	}


	/* --------------------- EX stage --------------------- */
	//if(opcode(state.IDEX.instr) == BEQ
	//		&& state.IDEX.readRegA == state.IDEX.readRegB)
	//	newState.EXMEM.branchTarget = state.IDEX.offset + state.IDEX.pcPlus1;
	//else
	//	newState.EXMEM.branchTarget = 0;

	if(opcode(state.IDEX.instr) == ADD)
		newState.EXMEM.aluResult = checkDataHaz(&state, &newState);
	else if(opcode(state.IDEX.instr) == NAND)
		newState.EXMEM.aluResult = checkDataHaz(&state, &newState);
	else if(opcode(state.IDEX.instr) == LW || opcode(state.IDEX.instr) == SW)
		newState.EXMEM.aluResult = checkDataHaz(&state, &newState);
	else if(opcode(state.IDEX.instr) == BEQ)
		newState.EXMEM.aluResult = checkDataHaz(&state, &newState);

	newState.EXMEM.readRegB = state.IDEX.readRegB;
	newState.EXMEM.instr = state.IDEX.instr;


	/* --------------------- MEM stage --------------------- */
	newState.MEMWB.instr = state.EXMEM.instr;
	if(opcode(state.EXMEM.instr) == LW)
		newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
	else if(opcode(state.EXMEM.instr) == SW)
		newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;
	else
		newState.MEMWB.writeData = state.EXMEM.aluResult;

	if(opcode(state.EXMEM.instr) == BEQ && state.EXMEM.aluResult == 0){
		newState.pc = state.EXMEM.branchTarget;
		newState.IFID.instr = NOOPINSTRUCTION;
		newState.IDEX.instr = NOOPINSTRUCTION;
		newState.EXMEM.instr = NOOPINSTRUCTION;
		newState.EXMEM.branchTarget = 0;
	}


	/* --------------------- WB stage --------------------- */
	if(opcode(state.MEMWB.instr) == ADD || opcode(state.MEMWB.instr) == NAND)
		newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
	else if(opcode(state.MEMWB.instr) == LW)
		newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;

	newState.WBEND.instr = state.MEMWB.instr;	//FOWARD TO WBEND
	newState.WBEND.writeData = state.MEMWB.writeData;


	/* --------------------- OTHER --------------------- */
	state = newState; /* this is the last statement before end of the loop.
			It marks the end of the cycle and updates the
			current state with the values calculated in this
			cycle */
	}

    return(0);
}

int checkDataHaz(const stateType *state, stateType *newState){
//only call this for BEQ add nand lw sw, sets aluresult
	int cur = state->IDEX.instr;
	int exmem = state->EXMEM.instr;
	int memwb = state->MEMWB.instr;
	int wbend = state->WBEND.instr;

	if(opcode(cur) == ADD || opcode(cur) == NAND || opcode(cur) == BEQ){
		int regA = state->IDEX.readRegA;
		int regB = state->IDEX.readRegB;

		if(field0(cur) == getDest(exmem))
			regA = state->EXMEM.aluResult;
		else if(field0(cur) == getDest(memwb))
			regA = state->MEMWB.writeData;
		else if(field0(cur) == getDest(wbend))
			regA = state->WBEND.writeData;

		if(field1(cur) == getDest(exmem))
			regB = state->EXMEM.aluResult;
		else if(field1(cur) == getDest(memwb))
			regB = state->MEMWB.writeData;
		else if(field1(cur) == getDest(wbend))
			regB = state->WBEND.writeData;

		if(opcode(cur) == ADD)
			return (regA + regB);
		else if(opcode(cur) == NAND)
			return ~(regA & regB);
		else if(opcode(cur) == BEQ){
			if(regA == regB)
				newState->EXMEM.branchTarget = state->IDEX.pcPlus1 + state->IDEX.offset;
			return(regA - regB);
		}
	}

	else if(opcode(cur) == LW || opcode(cur) == SW){
		int regA = state->IDEX.readRegA;

		if(field0(cur) == getDest(exmem))
			regA = state->EXMEM.aluResult;
		else if(field0(cur) == getDest(memwb))
			regA = state->MEMWB.writeData;
		else if(field0(cur) == getDest(wbend))
			regA = state->WBEND.writeData;

		return (regA + state->IDEX.offset);
	}
return 0;
}

int lwStall(const stateType *state){
	int cur = state->instrMem[state->pc];
	int ifid = state->IFID.instr;
	int idex = state->IDEX.instr;
	int exmem = state->EXMEM.instr;
	//int memwb = state->MEMWB.instr;
	//int wbend = state->WBEND.instr;


	if(opcode(cur) == ADD || opcode(cur) == NAND || opcode(cur) == BEQ){

		if(field0(cur) == getDest(ifid) && opcode(ifid) == LW)
			return 1;
		else if(field0(cur) == getDest(idex) && opcode(idex) == LW)
			return 1;
		else if(field0(cur) == getDest(exmem) && opcode(exmem) == LW)
			return 1;
		//else if(field0(cur) == getDest(memwb) && opcode(memwb) == LW)
	//		return 1;
		//else if(field0(cur) == getDest(wbend) && opcode(wbend) == LW)
		//	return 1;
		else if(field1(cur) == getDest(ifid) && opcode(ifid) == LW)
			return 1;
		else if(field1(cur) == getDest(idex) && opcode(idex) == LW)
			return 1;
		else if(field1(cur) == getDest(exmem) && opcode(exmem) == LW)
			return 1;
		//else if(field1(cur) == getDest(memwb) && opcode(memwb) == LW)
		//	return 1;
		//else if(field1(cur) == getDest(wbend) && opcode(wbend) == LW)
		//	return 1;
		else
			return 0;
	}

	else if(opcode(cur) == LW || opcode(cur) == SW){
		if(field0(cur) == getDest(ifid) && opcode(ifid) == LW)
			return 1;
		else if(field0(cur) == getDest(idex) && opcode(idex) == LW)
			return 1;
		else if(field0(cur) == getDest(exmem) && opcode(exmem) == LW)
			return 1;
		//else if(field0(cur) == getDest(memwb) && opcode(memwb) == LW)
		//	return 1;
		//else if(field0(cur) == getDest(wbend) && opcode(wbend) == LW)
		//	return 1;
		else
			return 0;
	}

	return 0;
}

int getDest(int instr){
	if(opcode(instr) == ADD || opcode(instr) == NAND){
		return field2(instr);
	}
	else if(opcode(instr) == LW)
		return field1(instr);
	else
		return -1;
}

int getArg0(int mem){
	mem = mem >> 19 & 7;
	return mem;
}

int getArg1(int mem){
	mem = mem >> 16 & 7;
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
