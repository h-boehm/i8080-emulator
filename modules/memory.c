#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../emulator/emulator.h"
#include "memory.h"

extern State8080 *state;

void print_memory() {
    // starting from RAM
    for (int i = 0; i < 16383; ++i) {
        // pad hex values for clearer output
        printf("%02x ", state->memory[i]);
        // newline every 16 bytes
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
}

void mem_write(int address, unsigned char byte) {
    // write one byte to memory
    // first 2K bytes are read-only memory
    if (address < 0x800) {
        printf("error: Can't write to ROM at address %x\n", address);
    }
    else {
        // set address to byte
        state->memory[address] = byte;
    }
}

unsigned char mem_read(int address) {
    // read one byte from memory
    return state->memory[address];
}

int load_file(State8080 *state, char *file, uint32_t offset) {
    FILE *f = fopen(file, "rb");
    if (!f) {
        fprintf(stderr, "error: could not open %s\n", file);
        return -1;
    }
    // get file size
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    // read bytes into memory
    uint8_t *buffer = &state->memory[offset];
    fread(buffer, fsize, 1, f);
    fclose(f);
    return 0;
}

void load_invaders(State8080 *state) {
    load_file(state, "invaders/invaders.h", 0);
    load_file(state, "invaders/invaders.g", 0x800);
    load_file(state, "invaders/invaders.f", 0x1000);
    load_file(state, "invaders/invaders.e", 0x1800);
}

int memory_init(State8080 *state) {
    state->memory = malloc(MEM_SIZE);
    if (state->memory == NULL) {
        return -1;
    }
    return 0;
}