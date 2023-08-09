
#ifndef PORTS_H
#define PORTS_H

#include "controls.h"

uint8_t input_port(SpaceInvadersMachine *machine, uint8_t port);
uint8_t output_port(SpaceInvadersMachine *machine, uint8_t port, uint8_t value);

#endif /* PORTS_H */