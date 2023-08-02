#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emulator/emulator.h"
#include "modules/memory.h"
#include "modules/interrupts.h"

State8080 *state;

int main(int argc, char **argv)
{
    int done = 0;

    // initialize state struct
    state = malloc(sizeof(State8080));
    if (!state) {
        fprintf(stderr, "error: could not initialize state object: %s\n", strerror(errno));
    }

    // initialize memory buffer
    memory_init(state);

    // load Space Invaders into memory
    load_invaders(state);

    // save initial time
    double lastInterrupt = get_time();

    // run program in a loop
    while (!done) {

        done = emulate_8080(state);

        // generate interrupt every 1/60 second 
        // (interrupts execute at 60 Hz)
        if (get_time() - lastInterrupt > 1.0/60.0) {
            if (state->int_enable) {
                generate_interrupt(state, 2);
                lastInterrupt = get_time();
            }
        }

        // debugging
        printf("pc: %d\n", state->pc);
        printf("instruction: %02X\n", state->memory[state->pc]);
        printf("A: 0x%02x, B: 0x%02x, C: 0x%02x, D: 0x%02x, E: 0x%02x, H: 0x%02x, L: 0x%02x\n", state->a, state->b, state->c, state->d, state->e, state->h, state->l);
    }

    return 0;
}