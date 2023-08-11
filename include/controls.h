#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdint.h>
#include "processor.h"

// create a machine object - see http://www.emulator101.com/cocoa-port-pt-2---machine-object.html
typedef struct SpaceInvadersMachine
{
    State8080 *state;

    double lastTimer;
    double nextInterrupt;
    int whichInterrupt;

    uint8_t in_port;
    uint8_t in_port_2;
    uint8_t out_port;

    uint8_t shift0;       // LSB of external shift hardware
    uint8_t shift1;       // MSB of external shift hardware
    uint8_t shift_offset; // offset for external shift hardware

    uint8_t out_port_3;
    uint8_t out_port_5;
    uint8_t prev_out_port_3;
    uint8_t prev_out_port_5;

} SpaceInvadersMachine;

enum port_keys
{
    KEY_COIN,
    KEY_P2_START,
    KEY_P1_START,
    NONE,
    KEY_P1_SHOOT,
    KEY_P1_LEFT,
    KEY_P1_RIGHT,
    KEY_TILT,
    KEY_P2_SHOOT,
    KEY_P2_LEFT,
    KEY_P2_RIGHT,
    KEY_COIN_INFO
};

void key_down(SpaceInvadersMachine *machine, uint8_t key);
void key_up(SpaceInvadersMachine *machine, uint8_t key);

#endif /* CONTROLS_H */