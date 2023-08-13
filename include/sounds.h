#ifndef SOUNDS_H
#define SOUNDS_H

#include <SDL2/SDL.h>
#include <SDL_mixer.h>
#include "controls.h"

void init_sounds();
void play_sounds(SpaceInvadersMachine *machine);

#endif /* SOUNDS_H */