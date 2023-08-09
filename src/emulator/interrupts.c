#include <stdint.h>
#include <time.h>

#include "ports.h"
#include "processor.h"


// function to generate interrupts
void  generate_interrupt(State8080 *state, int interrupt_num)
{
    // printf("Generating interrupt\n");
    // perform "PUSH PC" - see the example for what this does.
    // Push(state, (state->pc & 0xFF00) >> 8, (state->pc & 0xff)); // this function doesn't exist yet.

    state->memory[state->sp - 1] = (state->pc & 0xFF00) >> 8;
    state->memory[state->sp - 2] = (state->pc & 0xff);
    state->sp = state->sp - 2;

    // printf("stack pointer is now: %04x\n", state->sp);

    // Set the PC to the low memory vector.
    // This is identical to an "RST interrupt_num" instruction.

    // for interrupt one this should take us to 0008, and for interrupt 2 this should take us to 0010.
    state->pc = 8 * interrupt_num;
    // printf("program counter is now %04x\n", state->pc);

    // mimic "DI" - disable interrupt
    //  see the debugging section. this should prevent a new interrupt from
    //  generating until EI (enable interrupt) is called.
    state->int_enable = 0;
    // getchar();
}

// function to get the current time. https://stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
// used to figure out when to do the interrupt.
double time_us()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec * 1e6) + ((double)ts.tv_nsec / 1000.0);
}


double time_ms()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec * 1e3) + ((double)ts.tv_nsec / 1e6);
}

// global to check how many interrupts - delete this later.
int numInterrupts = 0;

// some other recommended functions. see http://www.emulator101.com/cocoa-port-pt-2---machine-object.html
void run_cpu(SpaceInvadersMachine *machine)
{
    // determine if interrupt should happen
    double now = time_us();
    if (machine->lastTimer == 0.0)
    {
        // set up initial values for timers
        machine->lastTimer = now;
        machine->nextInterrupt = machine->lastTimer + 16000.0;
        machine->whichInterrupt = 1;
    }

    if (machine->state->int_enable && (now > machine->nextInterrupt))
    {
        if (machine->whichInterrupt == 1)
        {
            generate_interrupt(machine->state, 1);
            numInterrupts += 1;
            machine->whichInterrupt = 2;
        }
        else
        {
            generate_interrupt(machine->state, 2);
            numInterrupts += 1;
            machine->whichInterrupt = 1;
        }
        machine->nextInterrupt = now + 8000.0;
    }

    // run the emulation - seems like it gets stuck here when first starting
    double sinceLast = now - machine->lastTimer;

    int cycles_to_catch_up = 2 * sinceLast;
    int cycles = 0;

    // run a certain number of cycles
    // it seems to get stuck here.
    while (cycles_to_catch_up > cycles)
    {
        unsigned char *op;
        op = &machine->state->memory[machine->state->pc];

        // handle IN
        if (*op == 0xdb)
        {
            uint8_t port = op[1];

            machine->state->a = input_port(machine, port); // set register A to the value of the port.
            machine->state->pc += 2;                            // update the program counter
            cycles += 3;                                        // update cpu cycles

#ifdef DEBUG
            printf("%x: handling input of %d\n", op[0], port);
            printf("register a: %d\n", machine->state->a);
#endif
            // printf("press a key to continue.\n");
            // getchar();
        }
        // handle OUT
        else if (*op == 0xd3)
        {
            uint8_t port = machine->state->memory[machine->state->pc + 1];

            output_port(machine, port, machine->state->a); // set the port to the value of register A.
            machine->state->pc += 2;                            // update the program counter
            cycles += 3;                                        // update cpu cycles
#ifdef DEBUG
            printf("handling output of %d\n", port);
#endif
        }

        // perform normal emulation.
        else
        {
#ifdef DEBUG
            if (*op == 0x3a)
            {
                if (machine->state->memory[0x20c0] == 0)
                {
                    printf("LDA instruction and compare is 0\n");
                    printf("memory is %04x\n", machine->state->memory[0x20c0]);
                    // getchar();
                }
            }

            if (*op == 0x35)
            {
                printf("DCR H\n");
                printf("memory address is %02x %02x\n", machine->state->h, machine->state->l);
                uint16_t offset = (uint16_t)machine->state->h << 8 | (uint16_t)machine->state->l;
                printf("memory is now %d\n", machine->state->memory[offset]);
                // getchar();
            }
#endif

            // this should return the number of cycles per instruction.
            // cpu.c should be updated to return the number of cycles
            cycles +=  emulate_i8080(machine->state);
        }
    }
    // update the last timer value
    machine->lastTimer = now;
#ifdef DEBUG
    printf("stopping emulation for this iteration\n");

    // check the machine state - it is delaying for 64 interrupts.
    if (machine->state->memory[0x20c0] != 0)
    {
        printf("isrDelay value is still %04x\n", machine->state->memory[0x20c0]);
        printf("the program counter is %04x\n", machine->state->pc);
    }

    printf("num interrupts is %d\n", numInterrupts);
    if (numInterrupts >= 64)
    {
        numInterrupts = 0;
    }

    printf("cycles to catch up was: %d\n", cycles_to_catch_up);
// getchar();
#endif
}