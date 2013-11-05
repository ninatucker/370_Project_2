/* 
 * File:   main.c
 * Author: jcruse03
 *
 * Created on October 7, 2013, 6:11 PM
 */

/* FSM for LC */

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
#define JALR 5
#define HALT 6
#define NOOP 7

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int memoryAddress;
    int memoryData;
    int instrReg;
    int aluOperand;
    int aluResult;
    int numMemory;
} stateType;

void printState(stateType *, char *);
void run(stateType);
int memoryAccess(stateType *, int);
int convertNum(int);

int
main(int argc, char *argv[]) {

    //test comment
    int i;
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>n", argv[0]);
        exit(1);
    }

    /* initialize memories and registers */
    for (i = 0; i < NUMMEMORY; i++) {
        state.mem[i] = 0;
    }
    for (i = 0; i < NUMREGS; i++) {
        state.reg[i] = 0;
    }

    state.pc = 0;

    /* read machine-code file into instruction/data memory (starting at
        address 0) */

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s\n", argv[1]);
        perror("fopen");
        exit(1);
    }

    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {
        if (sscanf(line, "%d", state.mem + state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    printf("\n");

    /* run never returns */
    run(state);

    return (0);
}

void
printState(stateType *statePtr, char *stateName) {
    int i;
    static int cycle = 0;
    printf("\n@@@\nstate %s (cycle %d)\n", stateName, cycle++);
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i = 0; i < statePtr->numMemory; i++) {
        printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i = 0; i < NUMREGS; i++) {
        printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("\tinternal registers:\n");
    printf("\t\tmemoryAddress %d\n", statePtr->memoryAddress);
    printf("\t\tmemoryData %d\n", statePtr->memoryData);
    printf("\t\tinstrReg %d\n", statePtr->instrReg);
    printf("\t\taluOperand %d\n", statePtr->aluOperand);
    printf("\t\taluResult %d\n", statePtr->aluResult);
}

/*
 * Access memory:
 *     readFlag=1 ==> read from memory
 *     readFlag=0 ==> write to memory
 * Return 1 if the memory operation was successful, otherwise return 0
 */
int
memoryAccess(stateType *statePtr, int readFlag) {
    static int lastAddress = -1;
    static int lastReadFlag = 0;
    static int lastData = 0;
    static int delay = 0;

    if (statePtr->memoryAddress < 0 || statePtr->memoryAddress >= NUMMEMORY) {
        printf("memory address out of range\n");
        exit(1);
    }

    /*
     * If this is a new access, reset the delay clock.
     */
    if ((statePtr->memoryAddress != lastAddress) ||
            (readFlag != lastReadFlag) ||
            (readFlag == 0 && lastData != statePtr->memoryData)) {
        delay = statePtr->memoryAddress % 3;
        lastAddress = statePtr->memoryAddress;
        lastReadFlag = readFlag;
        lastData = statePtr->memoryData;
    }

    if (delay == 0) {
        /* memory is ready */
        if (readFlag) {
            statePtr->memoryData = statePtr->mem[statePtr->memoryAddress];
        } else {
            statePtr->mem[statePtr->memoryAddress] = statePtr->memoryData;
        }
        return (1);
    } else {
        /* memory is not ready */
        delay--;
        return (0);
    }
}

int
convertNum(int num) {
    /* convert a 16-bit number into a 32-bit integer */
    if (num & (1 << 15)) {
        num -= (1 << 16);
    }
    return (num);
}

void
run(stateType state) {
    int arg0, arg1, arg2, addressField;
    int instructions = 0;
    int opcode;
    int maxMem = -1; /* highest memory address touched during run */

    //for (; 1; instructions++) { /* infinite loop, exits when it executes halt */
    //printState(&state);
    int bus;

fetch:

    instructions++;
    if (state.pc < 0 || state.pc >= NUMMEMORY) {
        printf("pc went out of the memory range\n");
        exit(1);
    }

    maxMem = (state.pc > maxMem) ? state.pc : maxMem;

    /* this is to make the following code easier to read */
    opcode = state.mem[state.pc] >> 22;
    arg0 = (state.mem[state.pc] >> 19) & 0x7;
    arg1 = (state.mem[state.pc] >> 16) & 0x7;
    arg2 = state.mem[state.pc] & 0x7; /* only for add, nand */

    addressField = convertNum(state.mem[state.pc] & 0xFFFF); /* for beq,
								    lw, sw */

    //begin new 
    printState(&state, "fetch");
    bus = state.pc;
    state.memoryAddress = bus;
    goto fetchLoad;

fetchLoad:
    printState(&state, "fetch2");
    if (memoryAccess(&state, 1)) { // read
        bus = state.memoryData;
        state.instrReg = bus;
        goto fetchOp;
    } else {
        goto fetchLoad;
    }

fetchOp:
    if ((state.instrReg >> 22) == ADD) {
        goto halt;
    } else if ((state.instrReg >> 22) == NAND) {
        goto halt;
    } else if ((state.instrReg >> 22) == LW) {
        goto lw;
    } else if ((state.instrReg >> 22) == SW) {
        goto halt;
    } else if ((state.instrReg >> 22) == NOOP) {
        goto halt;
    } else if ((state.instrReg >> 22) == BEQ) {
        goto halt;
    } else if ((state.instrReg >> 22) == JALR) {
        goto halt;
    } else if ((state.instrReg >> 22) == HALT) {
        goto halt;
    }






    /*	if (opcode == ADD) {
                printState(&state, "ADD");
                state.reg[arg2] = state.reg[arg0] + state.reg[arg1];
                state.pc++;
            } else if (opcode == NAND) {
                printState(&state, "NAND");
                state.reg[arg2] = ~(state.reg[arg0] & state.reg[arg1]);
                state.pc++;
            } else if (opcode == LW) {
                goto lw;
                printState(&state, "LW");
                if (state.reg[arg0] + addressField < 0 ||
                        state.reg[arg0] + addressField >= NUMMEMORY) {
                    printf("address out of bounds\n");
                    exit(1);
                }
                state.reg[arg1] = state.mem[state.reg[arg0] + addressField];
                if (state.reg[arg0] + addressField > maxMem) {
                    maxMem = state.reg[arg0] + addressField;
                }
                state.pc++;
            } else if (opcode == SW) {
                printState(&state, "SW");
                if (state.reg[arg0] + addressField < 0 ||
                        state.reg[arg0] + addressField >= NUMMEMORY) {
                    printf("address out of bounds\n");
                    exit(1);
                }
                state.mem[state.reg[arg0] + addressField] = state.reg[arg1];
                if (state.reg[arg0] + addressField > maxMem) {
                    maxMem = state.reg[arg0] + addressField;
                }
                state.pc++;
            } else if (opcode == BEQ) {
                printState(&state, "BEQ");
                if (state.reg[arg0] == state.reg[arg1]) {
                    state.pc += addressField;
                }
                else{
                    state.pc++;
                }
            } else if (opcode == JALR) {
                printState(&state, "JALR");
                state.reg[arg1] = state.pc;
                if(arg0 != 0)
                    state.pc = state.reg[arg0];
                else
                    state.pc = 0;
            } else if (opcode == NOOP) {
                printState(&state, "NOOP");
            } else if (opcode == HALT) {
                goto halt;
            } else {
                printf("error: illegal opcode 0x%x\n", opcode);
                exit(1);
            }
            state.reg[0] = 0;
        
            goto fetch;
     */ //}

lw:
    /* printState(&state, "LW");
     if (state.reg[arg0] + addressField < 0 ||
             state.reg[arg0] + addressField >= NUMMEMORY) {
         printf("address out of bounds\n");
         exit(1);
     }
     state.reg[arg1] = state.mem[state.reg[arg0] + addressField];
     if (state.reg[arg0] + addressField > maxMem) {
         maxMem = state.reg[arg0] + addressField;
     }
     */

    printState(&state, "lw");
    bus = state.instrReg; // >> 19 & 7;
    state.aluOperand = bus;
lw2:
    printState(&state, "lw2");
    bus = state.instrReg;
    state.aluResult = (state.aluOperand >> 19 & 7) +
            convertNum(bus & 0xFFFF);

lw3:
    printState(&state, "lw3");
    bus = state.aluResult;
    state.memoryAddress = bus;

lw4:
    printState(&state, "lw4");
    if (memoryAccess(&state, 1)) { // read
        goto lwSetReg;
    } else {
        goto lw4;
    }

lwSetReg:
    printState(&state, "lwSetReg");
    bus = state.memoryData;
    state.reg[state.instrReg >> 16 & 7] = state.memoryData; //store reg val
    state.pc++;
    goto fetch;

halt:
    printf("total of %d instructions executed\n", instructions + 1);
    printf("final state of machine:\n");
    printState(&state, "halt");
    state.pc++;
    exit(0);

}

