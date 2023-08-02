#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct Flags {
    uint8_t z : 1;   // zero
    uint8_t s : 1;   // sign
    uint8_t p : 1;   // get_parity
    uint8_t cy : 1;  // carry
    uint8_t ac : 1;  // auxiliary carry
    uint8_t pad : 3; // padding
} Flags;

typedef struct Ports
{
    uint8_t read1;
    uint8_t read2;
    uint8_t shift1;
    uint8_t shift0;
    uint8_t write2;
    uint8_t write4;
} Ports;

typedef struct State8080 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memory;
    struct Flags cc;
    struct Ports port;
    uint8_t int_enable;
} State8080;

extern State8080 *state;
// ref: http://www.emulator101.com/emulator-shell.html

// function declarations
int emulate_8080(State8080 *state);
void push_pc(State8080 *state, uint8_t high_byte, uint8_t low_byte);
void unimplemented_instruction(State8080 *state);

#endif /* CPU_H */