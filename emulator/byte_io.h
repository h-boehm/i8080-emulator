#ifndef BYTE_IO_H
#define BYTE_IO_H

#include <stdint.h>
#include "emulator.h"

// function declarations
uint8_t in_byte(State8080 *state, uint8_t port);
void out_byte(State8080 *state, uint8_t port);

#endif /* BYTE_IO_H */