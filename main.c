#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "modules/memory.h"
#include "emulator/cpu.h"
#include "utils/disasm.h"

#include <SDL2/SDL.h>

// compilation (on mac)
// gcc -o main main.c ./modules/memory.c ./utils/disasm.c ./emulator/cpu.c -I/Library/Frameworks/SDL2.framework/Headers -I./glad/include -F/Library/Frameworks -framework SDL2

// if we want to use SDL2, need to have this in there as well (for mac)
// -I/Library/Frameworks/SDL2.framework/Headers -I./glad/include -F/Library/Frameworks -framework SDL2

// to debug: add the -g flag
// run using gdb ./main or lldb ./main on Mac
// then type run

// create a machine object - see http://www.emulator101.com/cocoa-port-pt-2---machine-object.html
typedef struct SpaceInvadersMachine
{
    State8080 *state;

    double lastTimer;
    double nextInterrupt;
    int whichInterrupt;

    uint8_t in_port;
    uint8_t out_port;

    uint8_t shift0;       // LSB of external shift hardware
    uint8_t shift1;       // MSB of external shift hardware
    uint8_t shift_offset; // offset for external shift hardware
} SpaceInvadersMachine;

// function to handle input port. this should put the value of the port into register A.
// to run in attract mode, we need to return 1 from IN 0 and zero from IN 1.
// external shift is IN 3, OUT 2, OUT 4.
uint8_t InSpaceInvaders(SpaceInvadersMachine *machine, uint8_t port)
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

// function to handle output port.
void OutSpaceInvaders(SpaceInvadersMachine *machine, uint8_t port, uint8_t value)
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

// function to get the current time. https://stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
// used to figure out when to do the interrupt.
double timeusec()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec * 1e6) + ((double)ts.tv_nsec / 1000.0);
}

double timemsec()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((double)ts.tv_sec * 1e3) + ((double)ts.tv_nsec / 1e6);
}

// global to check how many interrupts - delete this later.
int numInterrupts = 0;

// some other recommended functions. see http://www.emulator101.com/cocoa-port-pt-2---machine-object.html
void doCPU(SpaceInvadersMachine *machine)
{
    // determine if interrupt should happen
    double now = timeusec();
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
            GenerateInterrupt(machine->state, 1);
            numInterrupts += 1;
            machine->whichInterrupt = 2;
        }
        else
        {
            GenerateInterrupt(machine->state, 2);
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

            machine->state->a = InSpaceInvaders(machine, port); // set register A to the value of the port.
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

            OutSpaceInvaders(machine, port, machine->state->a); // set the port to the value of register A.
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
            cycles += Emulate8080Op(machine->state);
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

// return the location of the video memory in the machine.
void *get_framebuffer(State8080 *state)
{
    return (void *)&state->memory[0x2400];
}

enum port_1_keys
{
    KEY_COIN,
    KEY_P2_START,
    KEY_P1_START,
    NONE,
    KEY_P1_SHOOT,
    KEY_P1_LEFT,
    KEY_P1_RIGHT
};
// enum port_2_keys
// {
//     NONE,
//     NONE,
//     KEY_TILT,
//     NONE,
//     KEY_P2_SHOOT,
//     KEY_P2_LEFT,
//     KEY_P2_RIGHT,
//     KEY_COIN_INFO
// };

// functions to accept key events
void KeyDown(SpaceInvadersMachine *machine, uint8_t key)
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

void KeyUp(SpaceInvadersMachine *machine, uint8_t key)
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

// wipe SDL surface
void WipeSurface(SDL_Surface *surface)
{
    /* This is fast for surfaces that don't require locking. */
    /* Once locked, surface->pixels is safe to access. */
    SDL_LockSurface(surface);

    /* This assumes that color value zero is black. Use
       SDL_MapRGBA() for more robust surface color mapping! */
    /* height times pitch is the size of the surface's whole buffer. */
    SDL_memset(surface->pixels, 0, surface->h * surface->pitch);

    SDL_UnlockSurface(surface);
}

// set a pixel color.
void SetPixel(SDL_Surface *surface, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_LockSurface(surface);
    uint8_t *pixelArray = (uint8_t *)surface->pixels;
    pixelArray[y * surface->pitch + x * surface->format->BytesPerPixel + 0] = g;
    pixelArray[y * surface->pitch + x * surface->format->BytesPerPixel + 1] = b;
    pixelArray[y * surface->pitch + x * surface->format->BytesPerPixel + 2] = r;
    SDL_UnlockSurface(surface);
}

// main function
int main(int argc, char **argv)
{
    // init functions

    // initialize memory buffer and load ROM files into memory
    mem_init();

    // can redirect this to file
    // print_memory();

    // set up a State8080 struct
    State8080 cpu_state;

    // create a machine
    SpaceInvadersMachine machine;
    // initialize the machine
    machine.state = &cpu_state;
    machine.lastTimer = 0;  // set the initial timer value - otherwise the game won't start right away.
    machine.in_port = 0x00; // set initial port values
    machine.out_port = 0x00;

    cpu_state.pc = 0;
    cpu_state.memory = memory;

    printf("press key to start\n");
    getchar();

    // create SDL window
    SDL_Window *window = NULL;

    // grab the window surface
    SDL_Surface *screen;

    // initialize the video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not be initialized\n");
    }
    else
    {
        printf("SDL video system ready\n");
    }

    // create a window
    printf("creating SDL window\n");
    window = SDL_CreateWindow("SDL2 Window", 20, 20, 256, 224, SDL_WINDOW_SHOWN);

    screen = SDL_GetWindowSurface(window);

    // pointer to the framebuffer
    uint8_t *framebuffer;

    int running = 1;
    double lastTimer = 0;
    // while (cpu_state.pc < sizeof(memory))

    printf("starting game loop\n");
    while (running) // infinite game loop?
    {
        double now = timemsec();
        double elapsed = now - lastTimer;
        //  game loop
        //  1. read input from keyboard.
        //  printf("reading keyboard\n");
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN) // change to switch
            {
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    KeyDown(&machine, KEY_P1_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    KeyDown(&machine, KEY_P1_LEFT);
                }
                if (event.key.keysym.sym == SDLK_c)
                {
                    KeyDown(&machine, KEY_COIN);
                }
                if (event.key.keysym.sym == SDLK_z)
                {
                    KeyDown(&machine, KEY_P1_START);
                }
                if (event.key.keysym.sym == SDLK_x)
                {
                    KeyDown(&machine, KEY_P1_SHOOT);
                }
            }
            if (event.type == SDL_KEYUP) // change to switch
            {
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    KeyUp(&machine, KEY_P1_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    KeyUp(&machine, KEY_P1_LEFT);
                }
                if (event.key.keysym.sym == SDLK_c)
                {
                    KeyUp(&machine, KEY_COIN);
                }
                if (event.key.keysym.sym == SDLK_z)
                {
                    KeyUp(&machine, KEY_P1_START);
                }
                if (event.key.keysym.sym == SDLK_x)
                {
                    KeyUp(&machine, KEY_P1_SHOOT);
                }
            }
        }

        // 2. update state - perform emulation
        doCPU(&machine);

        // 3. get the current frame buffer - pointer to the starting address
        // printf("getting framebuffer\n");
        framebuffer = get_framebuffer(&cpu_state);

        // 4. draw the screen - should run on a timer for 16ms
        if (elapsed > 16)
        {
            // clear the screen first
            // printf("drawing screen\n");
            WipeSurface(screen);

            // see https://www.reddit.com/r/EmuDev/comments/uxiux8/having_trouble_writing_the_space_invaders_video/
            // don't fully understand it yet? also screen is rotated.
            // seems like it draws but gets stuck on interrupts.
            for (int y = 0; y < 256; y++)
            {
                for (int x = 0; x < 224; x++)
                {
                    if (framebuffer[x / 8 + y * 256 / 8] & (1 << (x & 7)))
                    {
                        SetPixel(screen, x, y, 255, 255, 255);
                    }
                }
            }

            SDL_UpdateWindowSurface(window);
            lastTimer = now;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
