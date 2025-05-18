/**
 * main.h - Header file for a simple 16-bit virtual machine implementation
 * 
 * This file defines the data structures and types for a basic virtual machine
 * that emulates a 16-bit CPU architecture with a 64KB memory space.
 * The VM supports a small instruction set and register file similar to 
 * early x86 architectures.
 */

 #ifndef MAIN_H
 #define MAIN_H
 
 #define _GNU_SOURCE
 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <assert.h>
 #include <errno.h>

 #define NoArgs {0x00, 0x00} // 16 bit 0
 
 /**
  * Type definitions for clarity and portability
  * These ensure consistent sizes across different platforms
  */
 typedef unsigned char      uint8_t;  // 8-bit unsigned integer
 typedef unsigned short     uint16_t; // 16-bit unsigned integer
 typedef unsigned int       uint32_t; // 32-bit unsigned integer
 typedef unsigned long long uint64_t; // 64-bit unsigned integer
 
 typedef uint16_t Reg;              // 16-bit registers
 typedef uint8_t Stack[65536];      // Memory: can hold 2^16 bytes (64KB)
 typedef uint8_t Args;              // Arguments for instructions
 
 /**
  * Enum defining operation codes (opcodes) for the VM
  * Each opcode represents a different instruction the VM can execute
  */
 typedef enum {
     MOV = 0x01,  // Move data between registers or load immediate values
     NOP = 0x02   // No operation (do nothing)
 } Opcode;
 
 /**
  * Structure mapping opcodes to their instruction sizes in bytes
  */
 typedef struct {
     Opcode opcode;  // The operation code
     uint8_t size;   // Size of the complete instruction in bytes
 } InstructionMap;
 
 /**
  * Structure representing a machine instruction
  * Contains an opcode and variable-length arguments
  */
 typedef struct {
     Opcode opcode;
     Args args[];    // Flexible array member for arguments (0-2 bytes)
 } Instruction;
 
 typedef Instruction Program;  // A program is a sequence of instructions
 
 /**
  * Structure containing all CPU registers
  */
 typedef struct {
     Reg ax;        // Accumulator register
     Reg bx;        // Base register
     Reg cx;        // Counter register
     Reg sp;        // Stack pointer
     Reg ip;        // Instruction pointer
 } Registers;
 
 /**
  * Structure representing the CPU
  */
 typedef struct {
     Registers regs;  // CPU registers
 } CPU;
 
 /**
  * Structure representing the complete virtual machine
  */
 typedef struct {
     CPU cpu;           // 16-bit CPU (with registers)
     Stack *stack;      // 64KB virtual memory (65,536 bytes)
     Program *program;  // Currently loaded program
 } VM;
 
 /**
  * Global instruction map table
  * Maps each opcode to its instruction size
  */
 static const InstructionMap instruction_map[] = {
     { MOV, 0x03 },  // MOV instruction takes 3 bytes
     { NOP, 0x01 }   // NOP instruction takes 1 byte
 };
 
 /**
  * Function declarations
  */
 VM *vm_create(void);
 int main(int argc, char *argv[]);
  
 #endif /* MAIN_H */
 