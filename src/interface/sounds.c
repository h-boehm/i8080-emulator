#include "sounds.h"

#define NUM_SOUNDS 9
const char *wav_files[] = {
    "./sounds/0.wav",
    "./sounds/1.wav",
    "./sounds/2.wav",
    "./sounds/3.wav",
    "./sounds/4.wav",
    "./sounds/5.wav",
    "./sounds/6.wav",
    "./sounds/7.wav",
    "./sounds/8.wav"};

Mix_Chunk *sample[NUM_SOUNDS];

// ufo sound
int ufo = 0;

// initialize the library and load the sample sounds
void init_sounds()
{
    memset(sample, 0, sizeof(Mix_Chunk *) * NUM_SOUNDS);

    int result = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512);
    if (result < 0)
    {
        printf("unable to open audio: %s\n", SDL_GetError());
        exit(0);
    }
    result = Mix_AllocateChannels(4);
    if (result < 0)
    {
        printf("unable to allocate mixing channels: %s\n", SDL_GetError());
    }

    // load our samples
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        sample[i] = Mix_LoadWAV(wav_files[i]);
        if (sample[i] == NULL)
        {
            printf("unable to load wav file: %s\n", wav_files[i]);
        }
    }
}

// function to play sounds - ref http://www.emulator101.com/cocoa-port-pt-5---sound.html
// create variables to hold the previous state of OUT 3 and OUT 5
// when the state changes from 0 to 1, play the sound.
void play_sounds(SpaceInvadersMachine *machine)
{
    // if the previous value of out port 3 is different than the current value, then play a sound
    // from computerarchaeology - list of sounds
    // Port 3: (discrete sounds)
    //  bit 0=UFO (repeats)        SX0 0.raw
    //  bit 1=Shot                 SX1 1.raw
    //  bit 2=Flash (player die)   SX2 2.raw
    //  bit 3=Invader die          SX3 3.raw
    //  bit 4=Extended play        SX4
    //  bit 5= AMP enable          SX5
    //  bit 6= NC (not wired)
    //  bit 7= NC (not wired)
    //  Port 4: (discrete sounds)
    //  bit 0-7 shift data (LSB on 1st write, MSB on 2nd)
    if (machine->out_port_3 != machine->prev_out_port_3)
    {
        // ufo sound - repeats while ufo is on screen
        if ((machine->out_port_3 & 0x1) && !(machine->prev_out_port_3 & 0x1))
        {
            // start ufo sound while ufo is on screen.
            ufo = 1;
            Mix_PlayChannel(-1, sample[0], -1);
        }
        else if (!(machine->out_port_3 & 0x1) && (machine->prev_out_port_3 & 0x1))
        {
            if (ufo == 1)
            {
                // stop playing the ufo sound
                Mix_HaltChannel(-1);
                ufo = 0;
            }
        }

        // shoot sound effect
        if ((machine->out_port_3 & 0x2) && !(machine->prev_out_port_3 & 0x2))
        {
            Mix_PlayChannel(-1, sample[1], 0);
        }

        // death sound effect
        if ((machine->out_port_3 & 0x4) && !(machine->prev_out_port_3 & 0x4))
        {
            Mix_PlayChannel(-1, sample[2], 0);
        }

        // alien blows up sound effect
        if ((machine->out_port_3 & 0x8) && !(machine->prev_out_port_3 & 0x8))
        {
            Mix_PlayChannel(-1, sample[3], 0);
        }

        // set the prev out port 3
        machine->prev_out_port_3 = machine->out_port_3;
    }

    // if the previous value of out port 5 is different than the current value, then play a sound.
    // Port 5:
    //  bit 0=Fleet movement 1     SX6 4.raw
    //  bit 1=Fleet movement 2     SX7 5.raw
    //  bit 2=Fleet movement 3     SX8 6.raw
    //  bit 3=Fleet movement 4     SX9 7.raw
    //  bit 4=UFO Hit              SX10 8.raw
    //  bit 5= NC (Cocktail mode control ... to flip screen)
    //  bit 6= NC (not wired)
    //  bit 7= NC (not wired)
    if (machine->out_port_5 != machine->prev_out_port_5)
    {
        // invader movement sound 1
        if ((machine->out_port_5 & 0x1) && !(machine->prev_out_port_5 & 0x1))
        {
            Mix_PlayChannel(-1, sample[4], 0);
        }
        // invader movement sound 2
        if ((machine->out_port_5 & 0x2) && !(machine->prev_out_port_5 & 0x2))
        {
            Mix_PlayChannel(-1, sample[5], 0);
        }
        // invader movement sound 3
        if ((machine->out_port_5 & 0x4) && !(machine->prev_out_port_5 & 0x4))
        {
            Mix_PlayChannel(-1, sample[6], 0);
        }
        // invader movement sound 4
        if ((machine->out_port_5 & 0x8) && !(machine->prev_out_port_5 & 0x8))
        {
            Mix_PlayChannel(-1, sample[7], 0);
        }
        // ufo hit sound
        if ((machine->out_port_5 & 0x10) && !(machine->prev_out_port_5 & 0x10))
        {
            Mix_PlayChannel(-1, sample[8], 0);
        }

        // set the prev out port 5
        machine->prev_out_port_5 = machine->out_port_5;
    }
}
