#ifndef MEMORY_H
#define MEMORY_H

#include "../emulator/emulator.h"

// 8080 memory is 0 to 0xFFFF (64k bytes)
#define MEM_SIZE 0x10000

// Function declarations
int memory_init(State8080 *state);
void load_invaders();
void print_memory();

#endif /* MEMORY_H */