#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "controls.h"
#include "disasm.h"
#include "display.h"
#include "interrupts.h"
#include "memory.h"
#include "ports.h"
#include "processor.h"
#include "sounds.h"
#include <unistd.h>

void printDelay(const char *str) {
    while (*str) {
        putchar(*str++);
        fflush(stdout); // Flush the output buffer to ensure immediate printing
        usleep(2400);
    }
    fseek(stdout, 0, SEEK_END); // Move cursor to end of the line
}


int main(int argc, char **argv)
{
    // hide the cursor
    printf("\e[?25l");

    // title art
    printDelay("\n");
    printDelay("         ____________________\n");
    printDelay("        /  ________________  \\\n");
    printDelay("       /  / _______________\\  \\\n");
    printDelay("      /  / /              \\ \\  \\\n");
    printDelay("     /  / /                \\ \\  \\\n");
    printDelay("    |  | |      i8080      | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | | E M U L A T O R | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | |     welcome!    | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | |_________________| |  |\n");
    printDelay("    |  |  _________________  |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | |   O             | |  |\n");
    printDelay("    |  | |   |    [] []    | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("    |  | |   __   __       | |  |\n");
    printDelay("    |  | |   ||   ||       | |  |\n");
    printDelay("    |  | |   --   --       | |  |\n");
    printDelay("    |  | |                 | |  |\n");
    printDelay("     \\  \\ \\________________| /  /\n");
    printDelay("      \\_________________________/\n");
    printDelay("\n");
    // game selection
    printDelay("       _________________________\n");
    printDelay("      /                         \\\n");
    printDelay("     |    Game Selection:       |\n");
    printDelay("     |    1. Space Invaders     |\n");
    printDelay("     |    2. Space Invaders II  |\n");
    printDelay("     |    3. Lunar Escape       |\n");
    printDelay("     |    4. Bubble Bomber      |\n");
    printDelay("      \\________________________/\n");
    printDelay("\n");

    int gameSelection;
    printDelay("     Enter game selection (1-4): ");
    printf("\e[?25h");
    printf("\e[?25l");
    scanf("%d", &gameSelection);

    // screen size selection
    printDelay("\n");
    printDelay("       _________________________\n");
    printDelay("      /                         \\\n");
    printDelay("     |    Screen size:          |\n");
    printDelay("     |    1. Small              |\n");
    printDelay("     |    2. Medium             |\n");
    printDelay("     |    3. Large              |\n");
    printDelay("      \\________________________/\n");
    printDelay("\n");
    int screenSize;
    printDelay("     Choose screen size (1-3): ");
    printf("\e[?25h");
    printf("\e[?25l");
    scanf("%d", &screenSize);

    // start option
    printDelay("\n");
    char startOption = '\0'; // Initialize to null character
    do {
        printDelay("       _________________________\n");
        printDelay("      /                         \\\n");
        printDelay("     |    Press 's' to start!   |\n");
        printDelay("     |                          |\n");
        printDelay("      \\________________________/\n");
        printDelay("\n");
        printDelay("     "); // Added spaces to align the cursor
        printf("\e[?25h");
        printf("\e[?25l");
        scanf(" %c", &startOption);
        if (startOption != 'S' && startOption != 's') {
            printDelay("     Invalid selection - please try again!\n");
        } 
    } while (startOption != 'S' && startOption != 's');

    printDelay("     Starting game ...\n");

    // initialize memory buffer and load ROM files into memory

    if (gameSelection == 1)
    {
        mem_init();
    }
    else if (gameSelection == 2)
    {
        mem_init_dx();
    }
    else if (gameSelection == 3)
    {
        mem_init_lrescue();
    }
    else 
    {
        mem_init_balloon();
    }

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

    // create SDL window
    SDL_Window *window = NULL;

    // grab the window surface
    SDL_Surface *screen;

    // initialize the video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printDelay("     SDL could not be initialized\n");
    }
    else
    {
        printDelay("     SDL video system ready\n");
    }

    // initialize sound subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printDelay("     SDL audio could not be initialized\n");
    }
    else
    {
        printDelay("     SDL audio system ready\n");
    }

    // initialize the sounds.
    init_sounds();

    // create a window
    //printDelay("creating SDL window\n");
    int scale = screenSize;
    window = SDL_CreateWindow("SDL2 Window", 20, 20, 224 * scale, 256 * scale, SDL_WINDOW_SHOWN);

    screen = SDL_GetWindowSurface(window);

    // pointer to the framebuffer
    uint8_t *framebuffer;

    // state of the program
    int running = 1;

    // store when the last timer occurred for redrawing the screen
    double lastTimer = 0;

    //printDelay("starting game loop\n");
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