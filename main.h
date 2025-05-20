/**
 * main.h - Header file for a simple 16-bit virtual machine implementation
 *
 * This file defines the data structures and types for a basic virtual machine
 * that emulates a 16-bit CPU architecture with a 128KB memory space.
 * The VM supports a small instruction set and register file similar to
 * early x86 architectures.
 */
#ifndef MAIN_H
#define MAIN_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* windows only */
#include <Windows.h>
#include <conio.h> // _kbhit

/**
 * Type definitions for clarity and portability
 * These ensure consistent sizes across different platforms
 */
typedef unsigned char uint8_t;       // 8-bit unsigned integer
typedef unsigned short uint16_t;     // 16-bit unsigned integer
typedef unsigned int uint32_t;       // 32-bit unsigned integer
typedef unsigned long long uint64_t; // 64-bit unsigned integer

/**
 * MEMORY_MAX becomes 65536, which is the total number of memory locations the LC-3 (addressable range)
 * can address (from 0x0000 to 0xFFFF).
 */
#define MEMORY_MAX (1 << 16) // "shift the number 1 to the left by 16 bits." or 1 * 2^16 = 65536
uint16_t memory[MEMORY_MAX]; /* 65536 locations each 16 bits wide */

/* CPU Registers */
enum
{
    R_R0 = 0, /* General purpose register 0 */
    R_R1,     /* General purpose register 1 */
    R_R2,     /* General purpose register 2 */
    R_R3,     /* General purpose register 3 */
    R_R4,     /* General purpose register 4 */
    R_R5,     /* General purpose register 5 */
    R_R6,     /* General purpose register 6 */
    R_R7,     /* General purpose register 7 */
    R_PC,     /* Program counter: holds the address of the next instruction to execute */
    R_COND,   /* Condition register: stores flags about the last operation */
    R_COUNT   /* Total number of registers (not an actual register) */
};

uint16_t reg[R_COUNT]; /* Array that stores the current values of all CPU registers */

/* CPU Architecture (LC-3) Opcodes */
/* Instructions are 16 bits long, with the left 4 bits storing the opcode... the rest of the 12 bits are used to store params */
/* They are ordered so that they are assigned the proper enum value */
enum
{
    OP_BR = 0, /* branch: conditionally branch to a location if condition flags match */
    OP_ADD,    /* add: add two values and store the result */
    OP_LD,     /* load: load a value from memory into a register */
    OP_ST,     /* store: store a register value into memory */
    OP_JSR,    /* jump register: jump to subroutine, saving return address */
    OP_AND,    /* bitwise and: perform logical AND operation between two values */
    OP_LDR,    /* load register: load a value from memory with base register + offset */
    OP_STR,    /* store register: store a register value into memory with base register + offset */
    OP_RTI,    /* return from interrupt: unused in basic implementation */
    OP_NOT,    /* bitwise not: perform logical NOT operation on a value */
    OP_LDI,    /* load indirect: load a value from a memory location pointed to by another memory location */
    OP_STI,    /* store indirect: store a value to a memory location pointed to by another memory location */
    OP_JMP,    /* jump: unconditional jump to a location */
    OP_RES,    /* reserved: unused opcode */
    OP_LEA,    /* load effective address: load the address of a memory location into a register */
    OP_TRAP    /* execute trap: system call for I/O operations and program control */
};

/* Condition Flag Definitions */
/* Each CPU has a variety of condition flags to signal various situations. The LC-3 uses only 3 condition flags which indicate the sign of the previous calculation. */
enum
{
    FL_POS = 1 << 0, /* P, POSITIVE: Result of last operation was positive (bit 0 set to 1) */
    FL_ZRO = 1 << 1, /* Z, ZERO: Result of last operation was zero (bit 1 set to 1) */
    FL_NEG = 1 << 2, /* N, NEGATIVE: Result of last operation was negative (bit 2 set to 1) */
};

/* TRAP Codes */
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

/**
 * Function declarations
 */
int main(int argc, const char *argv[]); /* Main function that serves as the entry point for the VM */

#endif /* MAIN_H */