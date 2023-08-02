#include <stdint.h>

#include "emulator.h"

extern State8080 *state;

uint8_t in_byte(State8080 *state, uint8_t port)
{
    uint8_t byte;
    switch (port) {
    case 1:
        // read from port 1
        byte = state->port.read1;
        break;
    case 2:
        // read from port 2
        byte = state->port.read2;
        break;
    case 3:
        // read from port 3 (requires shift operation)
        uint16_t v = (state->port.shift1 << 8) | state->port.shift0;
        byte = ((v >> (8 - state->port.write2)) & 0xff);
        break;
    default:
        unimplemented_instruction(state);
        break;
    }
    return byte;
}

void out_byte(State8080 *state, uint8_t port)
{
    switch (port) {
    case 2:
        // write accumulator value to port 2
        // need 3 LSBs
        state->port.write2 = state->a & 0x7; 
        break;
    case 4:
        // write to port 4 (requires shift operation)
        state->port.shift0 = state->port.shift1;
        state->port.shift1 = state->a;
        break;
    }
}