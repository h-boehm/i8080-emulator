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
#include "sounds.h"

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
    // mem_init();
    mem_init_dx();
    // set up a State8080 struct
    State8080 cpu_state;

    // create a machine
    SpaceInvadersMachine machine;

    // initialize the machine
    machine.state = &cpu_state;
    machine.lastTimer = 0;  // set the initial timer value - otherwise the game won't start right away.
    machine.in_port = 0x00; // set initial port values
    machine.in_port_2 = 0x00;
    machine.out_port = 0x00;
    machine.out_port_3 = 0;
    machine.out_port_5 = 0;
    machine.prev_out_port_3 = 0;
    machine.prev_out_port_5 = 0;

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

    // initialize sound subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL audio could not be initialized\n");
    }
    else
    {
        printf("SDL audio system ready");
    }

    // initialize the sounds.
    init_sounds();

    // create a window
    printf("creating SDL window\n");
    int scale = 2;
    window = SDL_CreateWindow("SDL2 Window", 20, 20, 224 * scale, 256 * scale, SDL_WINDOW_SHOWN);

    screen = SDL_GetWindowSurface(window);

    // pointer to the framebuffer
    uint8_t *framebuffer;

    // state of the program
    int running = 1;

    // store when the last timer occurred for redrawing the screen
    double lastTimer = 0;

    printf("starting game loop\n");
    while (running) // infinite game loop?
    {
        double now = time_ms();
        double elapsed = now - lastTimer;
        //  game loop
        //  1. read input from keyboard.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }

            else if (event.type == SDL_KEYDOWN) // change to switch
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

                // player 2
                if (event.key.keysym.sym == SDLK_a)
                {
                    key_down(&machine, KEY_P2_START);
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    key_down(&machine, KEY_P2_SHOOT);
                }
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    key_down(&machine, KEY_P2_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    key_down(&machine, KEY_P2_LEFT);
                }

                // tilt - not sure if this does anything?
                if (event.key.keysym.sym == SDLK_d)
                {
                    key_down(&machine, KEY_TILT);
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

                // player 2
                if (event.key.keysym.sym == SDLK_a)
                {
                    key_up(&machine, KEY_P2_START);
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    key_up(&machine, KEY_P2_SHOOT);
                }
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    key_up(&machine, KEY_P2_RIGHT);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    key_up(&machine, KEY_P2_LEFT);
                }

                // tilt - not sure if this does anything?
                if (event.key.keysym.sym == SDLK_d)
                {
                    key_up(&machine, KEY_TILT);
                }
            }
        }

        // 2. update state - perform emulation
        run_cpu(&machine);

        // 3. get the current frame buffer - pointer to the starting address
        framebuffer = get_framebuffer(&cpu_state);

        // 4. draw the screen - should run on a timer for 16ms
        if (elapsed > 16)
        {
            // clear the screen
            wipe_surface(screen);

            // see https://www.reddit.com/r/EmuDev/comments/uxiux8/having_trouble_writing_the_space_invaders_video/
            // mem location 2400 has pixels (0,255),(0,254),(0,253),(0,252),(0,251),(0,250),(0,249),(0,248)
            int dx = 0;
            int dy = 0;
            for (int i = 0; i < 256 * 28; i++)
            {
                // loop through each byte in the frame buffer
                for (int j = 0; j < 8; j++)
                {
                    if (framebuffer[i] & (1 << j))
                    {
                        // set the pixel at dx,dy

                        // this gives a scanline looking effect?
                        // try to add some color as well based on the y value
                        for (int k = 0; k < scale; k++)
                        {
                            // score display - top of screen - this number will change depending on scale
                            if (((256 * scale - 1) - dy) < (256 * scale - 1) / 5)
                            {
                                set_pixel(screen, dx + k, (256 * scale - 1) - dy, 255, 0, 0);
                            }

                            // bottom of screen
                            else if (((256 * scale - 1) - dy) > (9 * (256 * scale - 1) / 10))
                            {
                                set_pixel(screen, dx + k, (256 * scale - 1) - dy, 0, 0, 255);
                            }

                            // white pixel
                            else
                            {
                                set_pixel(screen, dx + k, (256 * scale - 1) - dy, 255, 255, 255);
                            }
                        }
                    }
                    dy += scale;
                    if (dy >= 256 * scale)
                    {
                        dx += scale;
                        dy = 0;
                    }
                }
            }

            // update the display and set timer
            SDL_UpdateWindowSurface(window);
            lastTimer = now;
        }
    }

    // close the window and quit
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}