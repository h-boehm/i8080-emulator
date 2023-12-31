#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

// 16K buffer (sufficient for Space Invaders)
// increase mem size for other games
#define MEM_SIZE 0x4FFF

// global memory buffer
// instructions/operands are unsigned chars
unsigned char memory[MEM_SIZE];

void load_file(char *file, int address)
{
    FILE *f = fopen(file, "rb");
    if (f == NULL)
    {
        printf("error: Couldn't open %s\n", file);
        exit(1);
    }
    // get file size
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // read bytes into memory
    fread(memory + address, 1, fsize, f);
    fclose(f);
}

unsigned char mem_read(int address)
{
    // read one byte from memory
    return memory[address];
}

void mem_write(int address, unsigned char byte)
{
    // write one byte to memory
    // first 2K bytes are read-only memory
    if (address < 0x800)
    {
        printf("error: Can't write to ROM at address %x\n", address);
    }
    else
    {
        // set address to byte
        memory[address] = byte;
    }
}

void mem_init()
{
    // initialize memory
    // clear memory buffer (set all bytes to 0)
    memset(memory, 0, MEM_SIZE);

    // load game files (each one is 2048 (or 0x800) bytes)
    // start from RAM address
    load_file("./roms/invaders.h", 0x0000);
    load_file("./roms/invaders.g", 0x0800);
    load_file("./roms/invaders.f", 0x1000);
    load_file("./roms/invaders.e", 0x1800);
}

void mem_init_balloon()
{
    // initialize memory
    // clear memory buffer (set all bytes to 0)
    memset(memory, 0, MEM_SIZE);

    // balloon bomber
    load_file("./roms/tn01", 0x0000);
    load_file("./roms/tn02", 0x0800);
    load_file("./roms/tn03", 0x1000);
    load_file("./roms/tn04", 0x1800);
    load_file("./roms/tn05-1", 0x4000);
}


void mem_init_lrescue()
{
    // initialize memory
    // clear memory buffer (set all bytes to 0)
    memset(memory, 0, MEM_SIZE);

    // lunar rescue
    load_file("./roms/lrescue.1", 0x0000);
    load_file("./roms/lrescue.2", 0x0800);
    load_file("./roms/lrescue.3", 0x1000);
    load_file("./roms/lrescue.4", 0x1800);
    load_file("./roms/lrescue.5", 0x4000);
    load_file("./roms/lrescue.6", 0x4800);
}

void mem_init_dx()
{
    // initialize memory
    // clear memory buffer (set all bytes to 0)
    memset(memory, 0, MEM_SIZE);

    // load game files (each one is 2048 (or 0x800) bytes)
    // start from RAM address
    // space invaders deluxe
    load_file("./roms/invdelux.h", 0x0000);
    load_file("./roms/invdelux.g", 0x0800);
    load_file("./roms/invdelux.f", 0x1000);
    load_file("./roms/invdelux.e", 0x1800);
    load_file("./roms/invdelux.d", 0x4000);

}

void print_memory()
{
    // starting from RAM
    for (int i = 0; i < MEM_SIZE; ++i)
    {
        // pad hex values for clearer output
        printf("%02x ", memory[i]);
        // newline every 16 bytes
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
}