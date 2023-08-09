#ifndef DISPLAY_H
#define DISPLAY_H

#include "processor.h"
#include <SDL2/SDL.h>

void *get_framebuffer(State8080 *state);
void wipe_surface(SDL_Surface *surface);
void set_pixel(SDL_Surface *surface, int x, int y, uint8_t r, uint8_t g, uint8_t b);

#endif /* DISPLAY_H */