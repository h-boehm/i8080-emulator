#ifndef INTERUPTS_H
#define INTERUPTS_H

#include "controls.h"

void generate_interrupt(State8080 *state, int interrupt_num);
void run_cpu(SpaceInvadersMachine *machine);
double time_ms();
double time_us();

#endif /* INTERUPTS_H */