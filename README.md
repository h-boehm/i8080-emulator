## i8080-emulator

A complete emulation of the Intel 8080 processor written in C, capable of running classic arcade games (the program includes Space Invaders, Space Invaders 2, Balloon Bomber, and Lunar Rescue). The disassembler module converts compatible ROM images into assembly language instructions, faciliating CPU emulation and debugging. The memory module loads the game files into virtual memory, and the emulator then executes the instructions categorized into arithmetic, logical, branching, I/O, and stack operations. An interrupts module simulates the original hardware's 60 Hz frequency, which is critical to the execution of the gameplay. The user interface utilizes the SDL library for rendering graphics and capturing keyboard inputs, along with the SDL2 Mixer library for sound. A command-line interface provides options for game and screen size selection.

### Demo

<img src="https://imgur.com/RdO1yP4.gif" width="320"/>

### Terminal Menu

<img src="https://imgur.com/nnFFyJs.png" width="240"/>
<img src="https://imgur.com/CqAGuKp.png" width="240"/>
<img src="https://imgur.com/yXZIHQk.png" width="240"/>

### Diagnostic Test Result (cpudiag.asm)

<img src="https://imgur.com/AEFXoLH.png" width="375"/>
