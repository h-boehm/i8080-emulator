#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "emulator/cpu.h"
#include "utils/disasm.h"

// main function to read in the file
int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL)
    {
        printf("error: Couldn't open %s\n", argv[1]);
        exit(1);
    }

    // Get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // need to allocate some more memory for this?
    // see http://www.emulator101.com/memory-maps.html
    // the game has 8K of RAM starting at address 2000.
    // 0000 to 1fff : ROM
    // 2000 - 23ff  : work RAM
    // 2400 - 3fff  : video RAM
    // 4000 -       : RAM mirror
    unsigned char *buffer = malloc(8 * 1024 * sizeof(char));

    fread(buffer, fsize, 1, f);
    fclose(f);

    // set up a State8080 struct
    State8080 cpu_state;
    // int pc = 0;
    //  try setting the initial pc value
    cpu_state.pc = 0;
    cpu_state.memory = buffer;

    while (cpu_state.pc < fsize)
    {
        // pc += Disassemble8080Op(buffer, pc);
        // cpu_state.pc += Disassemble8080Op(buffer, cpu_state.pc);
        Emulate8080Op(&cpu_state);

        // wait for user to press enter before going to next instruction
        // getchar();
    }
    return 0;
}
