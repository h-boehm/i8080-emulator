#ifndef SOUNDS_H
#define SOUNDS_H

#include <SDL2/SDL.h>
#include <SDL2_mixer/SDL_mixer.h>
#include "controls.h"

typedef struct Sound
{
    char *file_path;
    SDL_AudioSpec wav_spec;
    uint32_t wav_length;
    uint8_t *wav_buffer;
} Sound;

void init_sounds();
void play_sounds(SpaceInvadersMachine *machine);

#endif /* SOUNDS_H */