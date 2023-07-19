#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 16K buffer (sufficient for Space Invaders)
#define MEM_SIZE 0x4000

// global memory buffer
// instructions/operands are unsigned chars
extern unsigned char memory[MEM_SIZE];

// function declarations
void load_file(char *file, int address);
unsigned char mem_read(int address);
void mem_write(int address, unsigned char byte);
void mem_init();
void print_memory();