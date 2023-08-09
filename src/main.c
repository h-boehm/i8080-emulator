#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "disasm.h"
#include "display.h"
#include "interrupts.h"
#include "controls.h"
#include "memory.h"
#include "ports.h"
#include "processor.h"

// compilation (on mac)
// gcc -o main main.c ./modules/memory.c ./utils/disasm.c ./emulator/cpu.c -I/Library/Frameworks/SDL2.framework/Headers -I./glad/include -F/Library/Frameworks -framework SDL2

// if we want to use SDL2, need to have this in there as well (for mac)
// -I/Library/Frameworks/SDL2.framework/Headers -I./glad/include -F/Library/Frameworks -framework SDL2

// to debug: add the -g flag
// run using gdb ./main or lldb ./main on Mac
// then type run

int main(int argc, char **argv)
{
    // initialize memory buffer and load ROM files into memory
    mem_init();

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
        double now = time_ms();
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
                    key_down(&machine, KEY_P1_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    key_down(&machine, KEY_P1_LEFT);
                }
                if (event.key.keysym.sym == SDLK_c)
                {
                    key_down(&machine, KEY_COIN);
                }
                if (event.key.keysym.sym == SDLK_z)
                {
                    key_down(&machine, KEY_P1_START);
                }
                if (event.key.keysym.sym == SDLK_x)
                {
                    key_down(&machine, KEY_P1_SHOOT);
                }
            }
            if (event.type == SDL_KEYUP) // change to switch
            {
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    key_up(&machine, KEY_P1_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    key_up(&machine, KEY_P1_LEFT);
                }
                if (event.key.keysym.sym == SDLK_c)
                {
                    key_up(&machine, KEY_COIN);
                }
                if (event.key.keysym.sym == SDLK_z)
                {
                    key_up(&machine, KEY_P1_START);
                }
                if (event.key.keysym.sym == SDLK_x)
                {
                    key_up(&machine, KEY_P1_SHOOT);
                }
            }
        }

        // 2. update state - perform emulation
        run_cpu(&machine);

        // 3. get the current frame buffer - pointer to the starting address
        // printf("getting framebuffer\n");
        framebuffer = get_framebuffer(&cpu_state);

        // 4. draw the screen - should run on a timer for 16ms
        if (elapsed > 16)
        {
            // clear the screen first
            // printf("drawing screen\n");
            wipe_surface(screen);

            // see https://www.reddit.com/r/EmuDev/comments/uxiux8/having_trouble_writing_the_space_invaders_video/
            // don't fully understand it yet? also screen is rotated.
            // seems like it draws but gets stuck on interrupts.
            for (int y = 0; y < 256; y++)
            {
                for (int x = 0; x < 224; x++)
                {
                    if (framebuffer[x / 8 + y * 256 / 8] & (1 << (x & 7)))
                    {
                        set_pixel(screen, x, y, 255, 255, 255);
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