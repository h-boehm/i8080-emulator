#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "modules/memory.h"
#include "emulator/cpu.h"
#include "utils/disasm.h"

// main function to read in the file
int main(int argc, char **argv)
{
    // initialize memory
    mem_init();

    // can redirect this to file
    // when running the executable
    print_memory();

    /*
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

    */
    return 0;
}
