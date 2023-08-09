// this should be similar to the main file - we want to load cpudiag.bin instead
// see http://www.emulator101.com/full-8080-emulation.html for guidance

// to compile (from project root)
// make test

// to run (from project root):
// ./cpu-test

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "disasm.h"
#include "memory.h"
#include "processor.h"

int main(int argc, char **argv)
{
    // initialize memory buffer and load file into memory
    load_file("cpudiag.bin", 0x100);

    // some other initialization steps
    // we need the first thing to happen be jumping to the code at 0x100
    memory[0] = 0xc3; // JMP
    memory[1] = 0;    // 0x0100
    memory[2] = 0x01;

    // fix the stack pointer from 0x6ad to 0x7ad
    // this 0x06 byte 112 in the code - which is
    // byte 112 + 0x100 = 368 in memory
    memory[368] = 0x7;

    // skip DAA test
    memory[0x59c] = 0xc3; // JMP
    memory[0x59d] = 0xc2;
    memory[0x59e] = 0x05;

    // may also have to modify something in the cpu emulation for instruction 0xcd (CALL)
    // which prints things to the console.

    // set up a State8080 struct
    State8080 cpu_state;
    //  try setting the initial pc value - should point to the start of the program
    cpu_state.pc = 0;
    cpu_state.memory = memory;
    while (cpu_state.pc < sizeof(memory))
    {
        // pc += disassemble_i8080(buffer, pc);
        // cpu_state.pc += disassemble_i8080(buffer, cpu_state.pc);

        // for debugging
        // printf("pc: %d\n", cpu_state.pc);
        // printf("instruction: %02X\n", cpu_state.memory[cpu_state.pc]);

         emulate_i8080(&cpu_state);

        // wait for user to press enter before going to next instruction
        // getchar();
    }

    return 0;
}