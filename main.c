/**
 * main.c - Complete implementation of the virtual machine
 *
 * This file contains all the code needed for the VM including
 * initialization, program loading, execution, and cleanup.
 */

#include "main.h"

/**
 * Creates and initializes a new virtual machine
 *
 * @return Pointer to the new VM or NULL on failure
 */
VM *vm_create(void)
{
    VM *vm;
    vm = (VM *)malloc(sizeof(VM)); // Allocate memory of size `VM` and return the starting memory address of the contiguous memory block of the VM 
    if (vm == NULL)
    {
        perror("Failed to allocate VM");
        return (VM *)0;
    }

    // Set all bytes in the CPU's registers to zero
    // &vm->cpu.regs : The memory address of the registers structure
    memset(&vm->cpu.regs, 0, sizeof(Registers));

    // Allocate 65,536 bytes (64KB) on the heap
    // The C runtime requests memory from the OS via sys calls
    // `stack` holds the address of the start of the alloced memory
    vm->stack = (Stack *)malloc(sizeof(Stack));
    if (!vm->stack)
    {
        perror("Failed to allocate VM memory");
        free(vm);
        return (VM *)0;
    }

    // Clear memory
    memset(vm->stack, 0, sizeof(Stack));

    // Program is initially NULL
    vm->program = NULL;

    return vm;
}
  
/**
 * Main program
 */
int main(int argc, char *argv[])
{
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;

    printf("Virtual Machine Starting...\n");

    // Create and initialize the VM
    VM *vm = vm_create();
    if (!vm)
    {
        fprintf(stderr, "Failed to create VM instance\n");
        return EXIT_FAILURE;
    }

    // Now we have a VM initialized on the program's heap in memory, and we have a pointer to it.
    // We now have to copu


    return EXIT_SUCCESS;
}
