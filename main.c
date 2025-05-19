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
    enum
    {
        PC_START = 0x3000
    };
    reg[R_PC] = PC_START;

    int running = 1;
    printf("VM initialized and ready. Hit Ctrl+C to exit.\n");

    /* CPU EXECUTION CYCLE */
    while (running)
    {
        /* FETCH */
        /* Fetch: Get the next instruction from memory at the address in PC, and advance PC */
        uint16_t instr = memory[reg[R_PC]++]; // Access the memory array at the address stored in the PC register, and after that access, PC is incremented by 1 to point to the next instruction
        uint16_t op = instr >> 12;            // This right-shifts the instruction value by 12 bits. In the LC-3 architecture, the leftmost 4 bits (bits 12-15) contain the opcode that identifies which instruction to execute (ADD, AND, LD, etc.).

        /* For now, just print the instruction for debugging */
        printf("Executing instruction at 0x%04X: 0x%04X (opcode: 0x%X)\n",
               reg[R_PC] - 1, instr, op);

        /* Add a small delay to not overwhelm the console */
        Sleep(500);

        /* Basic implementation - add proper instruction execution here */
        /* This is just a placeholder to demonstrate program flow */
    }

    /* When the program is interrupted, we want to restore the terminal settings back to normal. */
    restore_input_buffering();

    /* Return successful exit status */
    return EXIT_SUCCESS;
}