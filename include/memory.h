#ifndef MEMORY_H
#define MEMORY_H

// 16K buffer (sufficient for Space Invaders)
#define MEM_SIZE 0x4FFF

// global memory buffer
// instructions/operands are unsigned chars
extern unsigned char memory[MEM_SIZE];

// function declarations
void load_file(char *file, int address);
unsigned char mem_read(int address);
void mem_write(int address, unsigned char byte);
void mem_init();
void mem_init_dx();
void mem_init_lrescue();
void mem_init_balloon();
void print_memory();

#endif /* MEMORY_H */