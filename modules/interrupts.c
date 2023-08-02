#include <stddef.h>
#include <sys/time.h>


#include <stdio.h>

#include "../emulator/emulator.h"

extern State8080 *state;

void generate_interrupt(State8080 *state, int interruptNum) {
    // push pc onto the stack
    push_pc(state, (state->pc & 0xFF00) >> 8, (state->pc & 0xff));
    // set pc to lowest memory vector (RST instruction)
    state->pc = 8 * interruptNum;
}

double get_time() {
    // declare timeval struct
    struct timeval time;
    // get currrent time
    gettimeofday(&time, NULL);
    // convert to milliseconds
    double ms = (time.tv_sec) * 1000 + (time.tv_usec) / 1000;
    return ms;
}