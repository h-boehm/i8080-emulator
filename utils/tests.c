// this should be similar to the main file.
// we want to load in the cpudiag.bin file instead
// see http://www.emulator101.com/full-8080-emulation.html for guidance

// to compile:
// gcc -o tests tests.c ../modules/memory.c ../utils/disasm.c ../emulator/cpu.c -DFOR_CPUDIAG

// to run:
// ./testing/tests

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../modules/memory.h"
#include "../emulator/cpu.h"
#include "../utils/disasm.h"

// main function to read in the file
int main(int argc, char **argv)
{

    // initialize memory buffer and load ROM files into memory
    // mem_init();
    load_file("./cpudiag.bin", 0x100);

    // some other initialization steps
    // we need the first thing to happen to be jumping to the code at 0x100
    memory[0] = 0xc3; // JMP
    memory[1] = 0;    // 0x0100
    memory[2] = 0x01;

    // Fix the stack pointer from 0x6ad to 0x7ad
    //  this 0x06 byte 112 in the code, which is
    //  byte 112 + 0x100 = 368 in memory
    memory[368] = 0x7;

    // Skip DAA test
    memory[0x59c] = 0xc3; // JMP
    memory[0x59d] = 0xc2;
    memory[0x59e] = 0x05;

    // may also have to modify something in the cpu emulation for instruction 0xcd (CALL)
    // which prints things to the console.

    // can redirect this to file
    // print_memory();

    // set up a State8080 struct
    State8080 cpu_state;
    // int pc = 0;
    //  try setting the initial pc value - should point to the start of the program

    cpu_state.pc = 0;
    cpu_state.memory = memory;
    while (cpu_state.pc < sizeof(memory))
    {
        // pc += Disassemble8080Op(buffer, pc);
        // cpu_state.pc += Disassemble8080Op(buffer, cpu_state.pc);

        // for debugging
        // printf("pc: %d\n", cpu_state.pc);
        // printf("instruction: %02X\n", cpu_state.memory[cpu_state.pc]);

        Emulate8080Op(&cpu_state);

        // wait for user to press enter before going to next instruction
        // getchar();
    }

    return 0;
}
