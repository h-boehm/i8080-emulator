#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../emulator/emulator.h"

// function declarations
void generate_interrupt(State8080 *state, int interrupt_num);
double get_time();

#endif /* INTERRUPTS_H */