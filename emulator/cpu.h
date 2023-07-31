#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <inttypes.h>

// ref: http://www.emulator101.com/emulator-shell.html

// set up struct for CPU condition codes
typedef struct ConditionCodes
{
    uint8_t z : 1;
    uint8_t s : 1;
    uint8_t p : 1;
    uint8_t cy : 1;
    uint8_t ac : 1;
    uint8_t pad : 3;
} ConditionCodes;

// set up struct for CPU state
typedef struct State8080
{
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
    struct ConditionCodes cc;
    uint8_t int_enable;
} State8080;

// quit the program for every opcode with an error
void unimplementedInstruction(State8080 *state);

// redirect the output
void redirect_output(uint8_t byte, uint8_t port);

// emulate the opcode given the current CPU state
int Emulate8080Op(State8080 *state);

// sets flags after logical instruction on A register
void logic_flags_A(State8080 *state);

// set flags after arithmetic function on A register.
void arithmetic_flags_A(State8080 *state, int answer);

void GenerateInterrupt(State8080 *state, int interrupt_num);

// parity function
int parity(int x, int size);

#endif