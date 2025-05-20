/**
 * main.c - Complete implementation of the virtual machine
 *
 * This file contains all the code needed for the VM including
 * initialization, program loading, execution, and cleanup.
 */
#include "main.h"

/* Global Windows console handles and mode settings */
HANDLE hStdin = INVALID_HANDLE_VALUE; /* Handle for standard input stream */
DWORD fdwMode, fdwOldMode;            /* Current and original console mode flags */

/* Function prototypes */
void disable_input_buffering();
void restore_input_buffering();
uint16_t check_key();
void handle_interrupt();
uint16_t sign_extend(uint16_t x, int bit_count);
int read_image(const char *image_path);
void update_flags(uint16_t r);

/**
 * disable_input_buffering - Configure console for immediate input processing
 *
 * This function modifies the Windows console settings to allow for immediate
 * character-by-character input without requiring the user to press Enter.
 * It saves the original console mode to restore it later and disables features
 * like echo (displaying typed characters) and line buffering.
 */
void disable_input_buffering()
{
    /* Get handle to standard input */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    /* Save the current console mode configuration */
    GetConsoleMode(hStdin, &fdwOldMode);

    /* Create a new mode by toggling specific bits in the original mode:
     * - ENABLE_ECHO_INPUT: When disabled, typed characters aren't displayed
     * - ENABLE_LINE_INPUT: When disabled, input is available immediately without Enter key
     */
    fdwMode = fdwOldMode ^ ENABLE_ECHO_INPUT /* no input echo */
              ^ ENABLE_LINE_INPUT;           /* return when one or
                                                more characters are available */

    /* Apply the new mode settings to the console */
    SetConsoleMode(hStdin, fdwMode);
    /* Clear any pending input in the buffer */
    FlushConsoleInputBuffer(hStdin);
}

/**
 * restore_input_buffering - Restore original console input behavior
 *
 * This function restores the console to its original input mode settings,
 * which were saved when disable_input_buffering was called. This ensures
 * that the console behaves normally after our program exits.
 */
void restore_input_buffering()
{
    /* Restore the original console mode that was saved earlier */
    SetConsoleMode(hStdin, fdwOldMode);
}

/**
 * check_key - Detect if a key has been pressed
 *
 * This function checks if any keyboard input is available for reading.
 * It waits for a short time (1 second maximum) to see if input arrives,
 * and uses _kbhit to check if a key is in the input buffer.
 *
 * Returns:
 *   uint16_t: Non-zero value if a key is available, zero otherwise
 */
uint16_t check_key()
{
    /* Wait for input for up to 1000ms (1 second) and check if any key is pressed
     * WaitForSingleObject returns WAIT_OBJECT_0 if the input handle is signaled
     * _kbhit returns non-zero if there's a key in the input buffer
     */
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

/**
 * handle_interrupt - Clean up and exit when CTRL+C is pressed
 *
 * This function serves as a signal handler for interrupt signals (CTRL+C).
 * It restores the original console input settings and exits the program.
 */
void handle_interrupt()
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

/**
 * sign_extend - Extend a value to 16 bits with sign preservation
 *
 * This function takes a value with a specific bit count and extends it
 * to a 16-bit value, preserving the sign bit (most significant bit).
 *
 * Parameters:
 *   x: The value to extend
 *   bit_count: The number of bits in the original value
 *
 * Returns:
 *   uint16_t: The sign-extended 16-bit value
 */
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/**
 * update_flags - Update condition flags based on register value
 *
 * This function sets the condition flags (N, Z, P) based on the value
 * in the specified register. Exactly one flag will be set.
 *
 * Parameters:
 *   r: Register index to check
 */
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* Check if the most significant bit is 1 (negative) */
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

/**
 * read_image - Load a program image into memory
 *
 * This function loads a program binary file into the VM's memory space.
 * The file format has a header specifying the origin address, followed
 * by the program data.
 *
 * Parameters:
 *   image_path: Path to the image file
 *
 * Returns:
 *   int: 1 on success, 0 on failure
 */
int read_image(const char *image_path)
{
    FILE *file = fopen(image_path, "rb");
    if (!file)
    {
        printf("Error: Could not open file %s\n", image_path);
        return 0;
    }

    /* Read the origin (where in memory the program should be loaded) */
    uint16_t origin;
    if (fread(&origin, sizeof(origin), 1, file) != 1)
    {
        printf("Error: Could not read origin from file %s\n", image_path);
        fclose(file);
        return 0;
    }

    /* The origin tells us where in memory to put the program.
     * We need to convert it from big-endian to little-endian if needed.
     */
    /* Swap bytes on little endian architectures */
    origin = (origin << 8) | (origin >> 8);

    /* Load the program into memory starting at the origin address */
    printf("Loading image %s at origin 0x%04X\n", image_path, origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* Convert each word from big-endian to little-endian if necessary */
    for (size_t i = 0; i < read; i++)
    {
        p[i] = (p[i] << 8) | (p[i] >> 8);
    }

    /* Output statistics about the loaded image */
    printf("Loaded %zu words into memory\n", read);

    fclose(file);
    return 1;
}

/**
 * Main program
 *
 * This is the entry point for the virtual machine. It handles initialization,
 * loading programs, executing the instruction cycle, and cleanup.
 *
 * Parameters:
 *   argc: Number of command line arguments
 *   argv: Array of command line argument strings
 *
 * Returns:
 *   int: Exit status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int main(int argc, const char *argv[])
{
    /* Load arguments */
    /* To handle command line input to make our program usable. We expect one or more paths to VM images and present a usage string if none are given. */
    if (argc < 2)
    {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }
    // Load all image files provided as arguments
    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    /* Setup, to properly handle input to the terminal, we need to adjust some buffering settings. */
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    /* MAIN VM EXECUTION PROCEDURE */
    /* Since exactly one condition flag should be set at any given time, set the Z flag */
    reg[R_COND] = FL_ZRO;

    /* set the PC to starting position */
    /* 0x3000 is the default address */
    /* Programs start at address 0x3000 instead of 0x0, because the lower addresses are left empty to leave space for the trap routine code. */
    enum
    {
        PC_START = 0x3000 // Address 0011000000000000 in binary (16-bit), 12288 in decimal. This is the index position on the memory array
    };
    reg[R_PC] = PC_START;

    int running = 1;
    printf("VM initialized and ready. Hit Ctrl+C to exit.\n");

    /* CPU EXECUTION CYCLE */
    while (running)
    {
        /* FETCH */
        /* Fetch: Get the next instruction from memory at the address in PC, and advance PC */
        uint16_t instr = memory[reg[R_PC]++]; // Access the memory array at the address stored in the PC register, get the instruction (16 bit unsigned short int) and after that access, increment program counter (PC) by 1 to point to the next instruction in memory

        /* DECODE */
        /* Decode: Get the instruction's opcode, which are the 4 leftmost bits in the 16 bit unsigned int (the instruction) */
        uint16_t op = instr >> 12; // This right-shifts the instruction value by 12 bits. In the LC-3 architecture, the leftmost 4 bits (bits 12-15) contain the opcode that identifies which instruction to execute (ADD, AND, LD, etc.).

        /* For now, just print the instruction for debugging */
        printf("Executing instruction at 0x%04X: 0x%04X (opcode: 0x%X)\n", reg[R_PC] - 1, instr, op);

        /* EXECUTE */
        switch (op)
        {
        case OP_BR: /* Branch */
        {
            /*
                --- Instruction format ---
                15-12   11-9   8-0
                |OP_BR| nzp |PCoffset9|
                Where:
                * Bits 15–12: opcode (0000 for BR)
                * Bits 11–9: condition codes (N, Z, P) — e.g., 010 means "branch if result was zero"
                * Bits 8–0: a signed 9-bit offset to jump relative to current PC

                * The BR instruction will branch (jump) if ANY of the condition flags that are set in the instruction match the current condition flag in R_COND.
            */
            /*
            The BR instruction is very powerful because it allows for different kinds of conditional branches:
                1. BRn: Branch if negative (condition = 100)
                2. BRz: Branch if zero (condition = 010)
                3. BRp: Branch if positive (condition = 001)
                4. BRnz: Branch if negative or zero (condition = 110)
                5. BRnp: Branch if negative or positive (condition = 101)
                6. BRzp: Branch if zero or positive (condition = 011)
                7. BRnzp: Always branch (condition = 111)
            */
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9); // get 9-bit signed offset
            uint16_t cond_flag = (instr >> 9) & 0x7;            // get NZP bits (bits 11–9)
            if (cond_flag & reg[R_COND])                        // if current condition matches
            {
                reg[R_PC] += pc_offset; // jump relative to current PC
            }
            break;
        }

        case OP_ADD: /* Add */
        {
            /**
             * The ADD instruction takes two numbers, adds them together, and stores the result in a register. Each ADD instruction looks like the following:
             * Instruction format:argc
             *  15-12       11-9   8-6    5   4-3   2-0
                |OP_BR|     DR    |SR1|   0  |00|   SR2
            OR

                15-12       11-9   8-6     5      4-0
                |OP_BR|     DR    |SR1|    1     |imm5|

            If bit [5] is 0, the second source operand is obtained from SR2. If bit [5] is 1, the second source operand is obtained by sign-extending the imm5 field to 16 bits. In both cases, the second source operand is added to the contents of SR1 and the result stored in DR.
            */
            /* Destination register (DR) */
            uint16_t dr = (instr >> 9) & 0x7;
            /* First operand */
            uint16_t r1 = (instr >> 6) & 0x7;
            /* whether we are in immediate mode */
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag == 1)
            {
                /* Immediate mode */
                uint16_t imm5 = sign_extend(instr & 0x1F, 5); // Convert 5-bit value to 16-bit signed
                reg[dr] = reg[r1] + imm5;
            }
            else
            {
                /* Register mode */
                uint16_t r2 = instr & 0x7;
                reg[dr] = reg[r1] + reg[r2];
            }

            /* Update condition flags */
            update_flags(dr);
            break;
        }

        case OP_LD: /* Load */
        {
            /**
             * Loads a value from memory into a register
             * PC-relative addressing, meaning: The memory address is computed as PC + offset, and the content at that memory address is stored in the destination register.
             *  15     12 | 11    9 | 8                  0
                [  0010   |  DR     |   PCoffset9         ]
             *
             - Opcode (bits 15–12) = 0010 → this is LD
             - DR (bits 11–9): Destination Register (where to load the data)
             - PCoffset9 (bits 8–0): a 9-bit signed offset from the current PC (program counter)

             */
            uint16_t dr = (instr >> 9) & 0x7;                   // get the 11-9 bits (dr)
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9); // converts the 9-bit value into a proper signed 16-bit int, preserving its sign
            reg[dr] = memory[reg[R_PC] + pc_offset];
            update_flags(dr);
            break;
        }

        case OP_ST: /* Store */
        {
            /**
             * Store a register value into memory
             *  15     12 | 11    9 | 8                  0
                [  0011   |  DR    |   PCoffset9         ]
             */
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            memory[reg[R_PC] + pc_offset] = reg[dr];
            break;
        }

        case OP_JSR: /* Jump Register */
        {
            /**
             * Store a register value into memory
             *  15     12 |11| 10                   0
                [  0011   |DR| PCoffset11           ]
             */
            uint16_t long_flag = (instr >> 11) & 1;
            reg[R_R7] = reg[R_PC];

            if (long_flag == 1)
            {
                uint16_t long_pc_offset = sign_extend(instr & 0x7ff, 11);
                reg[R_PC] += long_pc_offset; /* JSR */
            }
            else
            {
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1]; /* JSRR */
            }
            break;
        }

        case OP_AND: /* Bitwise AND */
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag)
            {
                /* Immediate mode */
                uint16_t imm5 = sign_extend(instr & 0x1f, 5);
                reg[r0] = reg[r1] & imm5; // Bitwise AND
            }
            else
            {
                /* Register mode */
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] & reg[r2]; // Bitwise AND
            }
            update_flags(0);
            break;
        }

        case OP_LDR: /* Load Register */
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = sign_extend(instr & 0x3F, 6);

            reg[r0] = memory[reg[r1] + offset];
            update_flags(r0);
            break;
        }

        case OP_STR: /* Store Register */
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t offset = sign_extend(instr & 0x3F, 6);

            memory[reg[r1] + offset] = reg[r0];
            break;
        }

        case OP_RTI: /* Return from Interrupt */
        {
            /* Unused in basic implementation */
            printf("RTI instruction not implemented\n");
            break;
        }

        case OP_NOT: /* Bitwise NOT */
        {
            /**
             * Perform logical negation on each bit, forming the 1's complement of the given binary value
             */
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t sr = (instr >> 6) & 0x7;

            reg[dr] = ~reg[sr]; // Bitwise NOT
            update_flags(dr);
            break;
        }

        case OP_LDI: /* Load indirect*/
        {
            /**
             * Load a value from a location in memory into a register
             * Store a register value into memory
             *  15     12 | 11    9 | 8                  0
                [  1010   |  DR    |   PCoffset9         ]
             * An address is computed by sign-extending bits [8:0] to 16 bits and adding this value to the incremented PC. What is stored in memory at this address is the address of the data to be loaded into DR
             */
            /* destination register (DR) */
            uint16_t dr = (instr >> 9) & 0x7;
            /* PCoffset 9 */
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

            /* add pc_offset to the current PC, look at that memory location to get the final address */
            uint16_t addr = memory[reg[R_PC] + pc_offset];
            reg[dr] = memory[addr];
            update_flags(dr);
            break;
        }

        case OP_STI: /* Store Indirect */
        {
            uint16_t sr = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

            /* Get the address */
            uint16_t addr = memory[reg[R_PC] + pc_offset];
            /* Store the value at that address */
            memory[addr] = reg[sr];
            break;
        }

        case OP_JMP: /* Jump */
        {
            uint16_t baseR = (instr >> 9) & 0x7;
            reg[R_PC] = reg[baseR];
            break;
        }

        case OP_RES: /* Reserved */
        {
            printf("Reserved opcode encountered\n");
            break;
        }

        case OP_LEA: /* Load Effective Address */
        {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

            reg[dr] = reg[R_PC] + pc_offset;
            update_flags(dr);
            break;
        }

        case OP_TRAP: /* Trap / System Call */
        {
            /**
             * The LC-3 provides a few predefined routines for performing common tasks and interacting with I/O devices. For example, there are routines for getting input from the keyboard and for displaying strings to the console. These are called trap routines which you can think of as the operating system or API for the LC-3. Each trap routine is assigned a trap code which identifies it (similar to an opcode). To execute one, the TRAP instruction is called with the trap code of the desired routine.
             */
            /*
             * The trap will eventually return control back to where it was called from. So we store the current PC into register R7, which LC-3 uses as the return address register. This is similar to how real CPUs use a link register or stack to remember return points.
             */
            reg[R_R7] = reg[R_PC];

            /* Get trap vector */
            uint16_t trapvect = instr & 0xFF; // Extract lower 8 bits of the instruction

            switch (trapvect)
            {
            case TRAP_GETC: /* GETC: Read a character from keyboard */
            {
                /* read a single ASCII char */
                reg[R_R0] = (uint16_t)getchar();
                update_flags(R_R0);
                break;
            }
            case TRAP_OUT: /* OUT: Output a character */
            {
                putc((char)reg[R_R0], stdout);
                fflush(stdout);
                break;
            }
            case TRAP_PUTS: /* PUTS: output a null-terminated string to the screen (similar to 'printf' in C) */
            {
                /**
                 *  15    12 | 11-8  |  7      0
                    [ 1111  | xxxx  | trapvect8 ]

                 * Write a string of ASCII characters to the console display. The characters are contained in consecutive memory locations, one character per memory location, starting with the address specified in R0. Writing terminates with the occurrence of x0000 in a memory location. (Pg. 543)
                 * To display a string, we must give the trap routine a string to display. This is done by storing the address of the first character in R0 before beginning the trap.
                 */
                /**
                 * Notice that unlike C strings, characters are not stored in a single byte, but in a single memory location. Memory locations in LC-3 are 16 bits, so each character in the string is 16 bits wide. To display this with a C function, we will need to convert each value to a char and output them individually.
                 */

                // Get pointer to string in memory (R0 holds starting address)
                uint16_t *str_ptr;
                str_ptr = &memory[reg[R_R0]]; // point to the address of the value stored in memory's reg[R_R0] index
                char c;

                // Loop through each character until null terminator (x0000)
                while (*str_ptr != 0)
                {
                    c = (char)(*str_ptr); // Cast 16-bit word to char
                    putc(c, stdout);      // Print character to console
                    str_ptr++;            // Move to next character in string
                }
                fflush(stdout); // Make sure it prints immediately
                break;
            }
            case TRAP_IN: /* IN: Input a character with echo */
            {
                printf("Enter a character: ");
                int character = getchar();
                reg[R_R0] = (uint16_t)character;
                putchar((char)reg[R_R0]);
                fflush(stdout);
                update_flags(reg[R_R0]);
                break;
            }
            case TRAP_PUTSP: /* PUTSP: Output a byte string */
            {
                /**
                 * Same as PUTS except that it outputs null terminated strings with two ASCII chars packed into a single memory location, with low 8 bits outputted frist then the high 8 bits
                 */
                /*
                 * one char per byte (two bytes per word) here we need to swap back to big endian format
                 */

                /* Get string pointer from R0 */
                



            }
            }
        }

        default:
        {
            abort(); // Terminate or exit the program by raising the 'SIGABRT' signal. The 'SIGABRT' signal is one of the signals used in operating systems to indicate an abnormal termination of a program.
            break;
        }
        }
    }

    /* When the program is interrupted, we want to restore the terminal settings back to normal. */
    restore_input_buffering();

    /* Return successful exit status */
    return EXIT_SUCCESS;
}
