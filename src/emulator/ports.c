#include <stdint.h>

#include "processor.h"
#include "controls.h"

// function to handle input port. this should put the value of the port into register A.
// to run in attract mode, we need to return 1 from IN 0 and zero from IN 1.
// external shift is IN 3, OUT 2, OUT 4.
uint8_t input_port(SpaceInvadersMachine *machine, uint8_t port)
{
    unsigned char a;
    switch (port)
    {
        case 0:                   // INPUTS (not used)
            return 1;             // this will change if we handle input later. this may not do anything
        case 1:                   // INPUTS
            a = machine->in_port; // set register A to the value of in_port
            break;
        case 2:       // INPUTS
            return 0; // change this later
        case 3:       // this handles shift register. the result will be sent to register A.
        {
            uint16_t v = (machine->shift1 << 8) | machine->shift0;
            a = ((v >> (8 - machine->shift_offset)) & 0xff);
            break;
        }
    }
    return a;
}

// function to handle output port
void output_port(SpaceInvadersMachine *machine, uint8_t port, uint8_t value)
{
    switch (port)
    {
    case 2:
        machine->shift_offset = value & 0x7;
        break;
    case 3: // sound bits
        break;
    case 4:
        machine->shift0 = machine->shift1;
        machine->shift1 = value;
        break;
    case 5: // sound bits
        break;
    case 6: // watchdog
        break;
    }
}