#include <SDL2/SDL.h>

#include "controls.h"
#include "processor.h"

// functions to accept key events
void key_down(SpaceInvadersMachine *machine, uint8_t key)
{
    switch (key)
    {
    case KEY_COIN:               // need to define these
        machine->in_port |= 0x1; // set to 00000001 (bit 0)
        break;
    case KEY_P1_LEFT:
        machine->in_port |= 0x20; // set to 00100000 (bit 5)
        break;
    case KEY_P1_RIGHT:
        machine->in_port |= 0x40; // set to 01000000 (bit 6)
        break;
    case KEY_P1_START:
        machine->in_port |= 0x2; // set to 00000010 (bit 2)
        break;
    case KEY_P1_SHOOT:
        machine->in_port |= 0x10; // set to 00010000 (bit 4)
        break;
    }
}

void key_up(SpaceInvadersMachine *machine, uint8_t key)
{
    switch (key)
    {
    case KEY_COIN:
        machine->in_port &= ~0x1;
        break;
    case KEY_P1_LEFT:
        machine->in_port &= ~0x20;
        break;
    case KEY_P1_RIGHT:
        machine->in_port &= ~0x40;
        break;
    case KEY_P1_START:
        machine->in_port &= ~0x2;
        break;
    case KEY_P1_SHOOT:
        machine->in_port &= ~0x10;
        break;
    }
}