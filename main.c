#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "modules/memory.h"
#include "emulator/cpu.h"
#include "utils/disasm.h"

// main function to read in the file
int main(int argc, char **argv) {
    
    // initialize memory buffer and load ROM files into memory
    mem_init();

    // can redirect this to file
    // print_memory();

    // set up a State8080 struct
    State8080 cpu_state;
    // int pc = 0;
    //  try setting the initial pc value - should point to the start of the program

    cpu_state.pc = 0;
    cpu_state.memory = memory;
    while (cpu_state.pc < sizeof(memory)) {
        // pc += Disassemble8080Op(buffer, pc);
        // cpu_state.pc += Disassemble8080Op(buffer, cpu_state.pc);

        // for debugging
        printf("pc: %d\n", cpu_state.pc);
        printf("instruction: %02X\n", cpu_state.memory[cpu_state.pc]);

        Emulate8080Op(&cpu_state);

        // wait for user to press enter before going to next instruction
        // getchar();
    }

    return 0;
}
