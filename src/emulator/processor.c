#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "disasm.h"
#include "processor.h"

// if the instruction is not implemented yet
void unimplemented_instruction(State8080 *state)
{
    printf("error: unimplemented instruction %x\n", state->memory[state->pc]);
    exit(1);
}

// handles output from the game; just printing here for now
void redirect_output(uint8_t byte, uint8_t port)
{
    printf("Output to port %d: %02X\n", port, byte);
}

// determines parity of a number
int parity(int x, int size)
{
    int i;
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (i = 0; i < size; i++)
    {
        if (x & 0x1)
        {
            p++;
        }
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}

// sets flags after logical instruction on A register
void logic_flags_A(State8080 *state)
{
    state->cc.cy = state->cc.ac = 0;
    state->cc.z = (state->a == 0);
    state->cc.s = (0x80 == (state->a & 0x80));
    state->cc.p = parity(state->a, 8);
}

// set flags after arithmetic function on A register.
void arithmetic_flags_A(State8080 *state, int answer)
{
    state->cc.cy = (answer > 0xff);
    state->cc.z = ((answer & 0xff) == 0);
    state->cc.s = ((answer & 0x80) != 0);
    state->cc.p = parity(answer & 0xff, 8);
}

// function to emulate 8080 instructions
// modify this to return the number of cpu cycles for each instruction.
int emulate_i8080(State8080 *state)
{
    unsigned char *opcode = &state->memory[state->pc];
    int cycles = 0; // return the number of cycles (see the datasheet)

// add the code from the disassembler so we know which instruction
// is about to be executed.
#ifdef DEBUG
    disassemble_i8080(state->memory, state->pc);
#endif

    // giant switch statement for all the opcodes
    // see http://www.emulator101.com/finishing-the-cpu-emulator.html
    // for opcodes we need for Space Invaders.
    // for now, only add the ones that we need to complete to allow the game to run.
    switch (*opcode)
    {

    case 0x00: // NOP
    {
        state->pc += 1;
        cycles = 4;
        break;
    }
    case 0x01: // LXI B, word (B <- byte 3, C <- byte 2)
    {
        state->c = opcode[1];
        state->b = opcode[2];
        state->pc += 3;
        cycles = 10;
        break;
    }
    case 0x02: // STAX B (BC <- A)
    {
        int offset = state->b << 8 | state->c;
        // TODO - check if condition
        if (offset >= 0x800 && offset < 0x4000)
        {
            state->memory[offset] = state->a;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }
    case 0x03: // INX B (BC <- BC + 1)
    {
        int val =  state->b << 8 | state->c;
        val++;
        state->b = (val >> 8) & 0xff; // store higher 8 bits in b
        state->c = val & 0xff;        // store lower 8 bits in c
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x04: // INR B (B <- B + 1 - all condition flags will change execpt CY)
    {
        state->cc.ac = (state->b & 0x0F) == 0x0f; // check auxiliary carry flag
        state->cc.z = (state->b == 0);
        state->cc.s = (0x80 == (state->b & 0x80));
        state->cc.p = parity(state->b, 8);
        state->cc.ac = state->cc.ac && ((state->b & 0x0F) == 0x00); // set auxiliary carry flag
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x05: // DCR B (B <- B-1 (decrement B - all condition flags affected except CY)
    {
        state->cc.ac = (state->b & 0x0F) == 0x00; // reversed from instruction 0x04
        state->b--;
        state->cc.z = (state->b == 0);
        state->cc.s = (0x80 == (state->b & 0x80));
        state->cc.p = parity(state->b, 8);
        state->cc.ac = state->cc.ac && ((state->b & 0x0f) == 0x0f);
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x06: // MVI B, D8 (B <- byte 2)
    {
        state->b = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }
    case 0x07: // RLC (A = A << 1)
    {
        int res = state->a;
        state->a = ((res & 0x80) >> 7) | (res << 1); // left shift by 1, OR with previous bit 7
        state->cc.cy = (0x80 == (res & 0x80));
        state->pc += 1;
        cycles = 4;
        break;
    }
    case 0x08: // NOP
    {
        state->pc += 1;
        cycles = 4;
        break;
    }
    case 0x09: // DAD B (HL = HL + BC - only the CY flag is affected)
    {
        // adding 2 16-bit values, so store in 32 bit
        uint32_t val = (uint16_t)state->b << 8 | (uint16_t)state->c;
        uint32_t hl_val = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint32_t res = val + hl_val;

        // save the higher byte in h
        state->h = (res & 0xff00) >> 8;
        // save the lower byte in l
        state->l = (res & 0xff);

        // set the carry flag
        if (res > 0xffff)
        {
            state->cc.cy = 1;
        }
        else
        {
            state->cc.cy = 0;
        }
        state->pc += 1;
        cycles = 10;
        break;
    }
    case 0x0a: // LDAX B : A <- (BC) (load content of memory location (BC) into A)
    {
        uint16_t offset = state->b << 8 | state->c; // for the memory location in register pair DE
        state->a = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }
    case 0x0b: // DCX B : BC = BC-1 (decrement BC)
    {
        uint16_t value = (uint16_t)state->b << 8 | (uint16_t)state->c;
        value -= 1;
        state->b = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->c = value & 0xff;          // store the lower 8 bits in c
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x0c: // INR C : C <- C+1 (increment C)
    {
        // condensed version
        uint16_t answer = (uint16_t)state->c + 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->c = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x0d: // DCR C : C <-C-1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->c - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->c = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x0e: // MVI C, D8 : C <- byte 2
    {
        state->c = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }
    // rotate instruction
    case 0x0f: // RRC : A = A >> 1; bit 7 = prev bit 0; CY = prev bit
    {
        uint8_t x = state->a;
        state->a = ((x & 1) << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x10: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0x11: // LX1 D, D16 : D <- byte 3, E <- byte 2
    {
        state->e = opcode[1];
        state->d = opcode[2];
        state->pc += 3;
        cycles = 10;
        break;
    }

    case 0x12: // STAX D : (DE) <- A (store A in memory location DE)
    {
        uint16_t offset = (uint16_t)state->d << 8 | (uint16_t)state->e; // for the memory location of register pair DE
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->a;
        }

        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x13: // INX D : DE <- DE + 1 (increment register pair DE). no condition flags are affected
    {
        uint16_t value = (uint16_t)state->d << 8 | (uint16_t)state->e;
        value += 1;
        state->d = (value & 0xff00) >> 8; // store the higher 8 bits in d
        state->e = value & 0xff;          // store the lower 8 bits in e
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x14: // INR D : D <- D+1 (affects condition flags)
    {
        uint16_t answer = (uint16_t)state->d + 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->d = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x15: // DCR D : D <- D-1 (decrement D)
    {
        // condensed version
        uint16_t answer = (uint16_t)state->d - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->d = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x16: // MVI D, D8 : D <- byte 2
    {
        state->d = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0x17: // RAL : A = A << 1; bit 0 = prev CY; CY = prev bit 7 (rotate left through carry)
    {
        uint8_t x = state->a;
        state->a = state->cc.cy | (x << 1);  // left shift by 1, OR with the previous carry flag
        state->cc.cy = (0x80 == (x & 0x80)); // previous bit 7
        state->pc += 1;

        break;
    }

    case 0x18: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0x19: // DAD D : HL = HL + DE (only affects the carry flag)
    {
        uint32_t value = (uint16_t)state->d << 8 | (uint16_t)state->e;
        uint32_t hl_value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint32_t answer = value + hl_value;

        // save the higher byte in h
        state->h = (answer & 0xff00) >> 8;
        // save the lower byte in l
        state->l = (answer & 0xff);

        // set the carry flag
        if (answer > 0xffff)
        {
            state->cc.cy = 1;
        }
        else
        {
            state->cc.cy = 0;
        }
        state->pc += 1;
        cycles = 10;
        break;
    }
    case 0x1a: // LDAX D : A <- (DE). load content of memory location (DE) into A
    {
        uint16_t offset = state->d << 8 | state->e; // for the memory location in register pair DE
        state->a = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x1b: // DCX D : DE = DE-1
    {
        uint16_t value = (uint16_t)state->d << 8 | (uint16_t)state->e;
        value -= 1;
        state->d = (value & 0xff00) >> 8; // store the higher 8 bits in d
        state->e = value & 0xff;          // store the lower 8 bits in e
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x1c: // INR E : E <-E+1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->e + 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->e = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x1d: // DCR E : E <- E-1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->e - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->e = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x1e: // MVI E, D8 : E <- byte 2
    {
        state->e = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0x1f: // RAR : A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0 (rotate right through carry)
    {
        uint8_t x = state->a;
        state->a = (state->cc.cy << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x20: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0x21: // LXI H, D16 : H <- byte 3, L <- byte 2
    {
        state->h = opcode[2];
        state->l = opcode[1];
        state->pc += 3;
        cycles = 10;
        break;
    }

    case 0x22: // SHLD addr : (adr) <-L; (adr+1)<-H (store the value in l at offset and the value in h at offset+1)
    {
        uint16_t offset = opcode[2] << 8 | opcode[1];

        // should protect the memory?
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->l;
            state->memory[offset + 1] = state->h;
        }

        state->pc += 3;
        cycles = 16;
        break;
    }

    case 0x23: // INX H : HL <- HL + 1 (increment register pair HL)
    {
        uint16_t value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        value += 1;
        state->h = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->l = value & 0xff;          // store the lower 8 bits in c
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x24: // INR H : H <- H+1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->h + 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->h = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x25: // DCR H : H <- H-1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->h - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->h = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x26: // MVI H, D8 : H <- byte 2
    {
        state->h = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0x27: // DAA - reference http://www.emulator101.com/cocoa-port-pt-4---keyboard.html
    {
        if ((state->a & 0xf) > 9)
            state->a += 6;
        if ((state->a & 0xf0) > 0x90)
        {
            uint16_t res = (uint16_t)state->a + 0x60;
            state->a = res & 0xff;
            arithmetic_flags_A(state, res);
        }
        // increment program counter
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x28: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0x29: // DAD H : HL = HL + HL (affects carry flag)
    {
        // adding 2 16 bit
        uint32_t value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint32_t answer = value + value;

        // save the higher byte in h
        state->h = (answer & 0xff00) >> 8;
        // save the lower byte in l
        state->l = (answer & 0xff);

        // set the carry flag
        if (answer > 0xffff)
        {
            state->cc.cy = 1;
        }
        else
        {
            state->cc.cy = 0;
        }
        state->pc += 1;
        cycles = 10;
        break;
    }

    case 0x2a: // LHLD adr : L <- (adr); H<-(adr+1) (load H and L direct)
    {
        uint16_t offset = opcode[2] << 8 | opcode[1];
        state->l = state->memory[offset];
        state->h = state->memory[offset + 1];

        state->pc += 3;
        cycles = 16;
        break;
    }

    case 0x2b: // DCX H : HL = HL-1
    {
        uint16_t value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        value -= 1;
        state->h = (value & 0xff00) >> 8; // store the higher 8 bits in h
        state->l = value & 0xff;          // store the lower 8 bits in l
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x2c: // INR L : L <- L+1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->l + 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->l = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x2d: // DCR L : L <- L-1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->l - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->l = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x2e: // MVI L, D8 : L <- byte 2
    {
        state->l = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0x2f: // CMA (not) : A <- !A (no condition flags are affected)
        state->a = ~state->a;
        state->pc += 1;
        cycles = 4;
        break;

    case 0x30: // none
    {
        unimplemented_instruction(state);
        break;
    }
    case 0x31: // LXI SP, D16 : SP.hi <- byte 3, SP.lo <- byte 2
    {
        state->sp = (opcode[2] << 8) | opcode[1];
        state->pc += 3;
        cycles = 10;
        break;
    }
    case 0x32: // STA addr : (addr) <- A (store A into memory location addr or bytes 2 and 3)
    {
        uint16_t offset = (opcode[2] << 8 | opcode[1]);
        state->memory[offset] = state->a;
        state->pc += 3;
        cycles = 13;
        break;
    }
    case 0x33: // INX SP : SP = SP + 1
    {
        state->sp += 1;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x34: // INR M : (HL) <- (HL)+1 (content of memory location at HL is incremented by 1. all flags except cy affected.)
    {
        // condensed version

        // get the offset
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t value = state->memory[offset];
        uint16_t answer = value + 1;
        // set the memory value to the answer
        state->memory[offset] = answer;

        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->b = answer & 0xff;
        state->pc += 1;
        cycles = 10;
        break;
    }

    case 0x35: // DCR M : (HL) <- (HL)-1
    {
        // condensed version

        // get the offset
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t value = state->memory[offset];
        // printf("memory before is %d\n", value);
        uint16_t answer = value - 1;
        // set the memory value to the answer
        state->memory[offset] = answer;
        // printf("memory after is %d\n", state->memory[offset]);
        //  getchar();

        // set the flags
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->b = answer & 0xff;
        state->pc += 1;
        cycles = 10;
        break;
    }

    case 0x36: // MVI M, D8 : (HL) <- byte 2 (move byte 2 to the memory location in (HL))
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->memory[offset] = opcode[1];
        state->pc += 2;
        cycles = 10;
        break;
    }

    case 0x37: // STC : CY = 1 (set carry flag to 1)
    {
        state->cc.cy = 1;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x38: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0x39: // DAD SP : HL = HL + SP
    {
        uint32_t hl_value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint32_t answer = state->sp + hl_value;

        // save the higher byte in h
        state->h = (answer & 0xff00) >> 8;
        // save the lower byte in l
        state->l = (answer & 0xff);

        // set the carry flag
        if (answer > 0xffff)
        {
            state->cc.cy = 1;
        }
        else
        {
            state->cc.cy = 0;
        }
        state->pc += 1;
        cycles = 10;
        break;
    }

    case 0x3a: // LDA addr : A <- (adr) (load value stored at addr (bytes 2 and 3) to A)
    {
        uint16_t offset = (opcode[2] << 8 | opcode[1]);
        state->a = state->memory[offset];
        state->pc += 3;
        cycles = 13;
        break;
    }

    case 0x3b: // DCX SP : SP = SP-1
    {
        state->sp -= 1;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x3c: // INR A : A <- A+1
    {
        uint16_t answer = (uint16_t)state->a + 1;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x3d: // DCR A : A <- A-1
    {
        // condensed version
        uint16_t answer = (uint16_t)state->a - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x3e: // MVI A, D8 : A <- byte 2 (move byte 2 into A)
        state->a = opcode[1];
        state->pc += 2;
        cycles = 7;
        break;

    case 0x3f: // CMC : CY=!CY
    {
        state->cc.cy = ~state->cc.cy;
        state->pc += 1;
        cycles = 4;
        break;
    }
    case 0x40: // MOV B,B

    { // this is a no-op because it's moving the value to the same register.
        state->pc++;
        cycles = 5;
        break;
    }
    case 0x41: // MOV    B,C : B <- C
    {
        state->b = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x42: // MOV    B,D : B <- D
    {
        state->b = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x43: // MOV    B,E : B <- E
    {
        state->b = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x44: // MOV B,H : B <- H
    {
        state->b = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x45: // MOV B,L : B <- L
    {
        state->b = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x46: // MOV B,M : B <- (HL) (move value at location (HL) to B)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->b = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x47: // MOV B,A : B <- A
    {
        state->b = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x48: // MOV C,B : C <- B
    {
        state->c = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x49: // MOV C,C : C <- C
    {
        // can be no op since moving value to the same register
        state->c = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x4a: // MOV C,D : C <- D
    {
        state->c = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x4b: // MOV C,E : C <- E
    {
        state->c = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x4c: // MOV C,H: C <- H
    {
        state->c = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x4d: // MOV C,L : C <- L
    {
        state->c = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x4e: // MOV C,M : C <- (HL)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->c = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x4f: // MOV C,A : C <- A
    {
        state->c = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x50: // MOV D,B : D <- B
    {
        state->d = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x51: // MOV D,C : D <- C
    {
        state->d = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x52: // MOV D,D : D <- D
    {
        state->d = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x53: // MOV D,E : D <- E
    {
        state->d = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x54: // MOV D,H : D <- H
    {
        state->d = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x55: // MOV D,L : D <- L
    {
        state->d = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x56: // MOV D, M : D <- (HL) (move data at memory location (HL) to D)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->d = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x57: // MOV D,A : D <- A
    {
        state->d = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x58: // MOV E,B : E <- B
    {
        state->e = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x59: // MOV E,C : E <- C
    {
        state->e = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x5a: // MOV E,D : E <- D
    {
        state->e = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x5b: // MOV E,E : E <- E
    {
        // can be no op since moving data to same register.
        state->e = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x5c: // MOV E,H : E <- H
    {
        state->e = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x5d: // MOV E,L : E <- L
    {
        state->d = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x5e: // MOV E, M : E <- (HL) (move data at memory location (HL) to E)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->e = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x5f: // MOV E,A : E <- A
    {
        state->e = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x60: // MOV H,B : H <- B
    {
        state->h = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x61: // MOV H,C : H <- C
    {
        state->h = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x62: // MOV H,D : H <- D
    {
        state->h = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x63: // MOV H,E : H <- E
    {
        state->h = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x64: // MOV H,H : H <- H
    {
        state->h = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x65: // MOV H,L : H <- L
    {
        state->h = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x66: // MOV H, M : H <- (HL) (move data at memory location (HL) to H)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->h = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x67: // MOV H,A : H <- A
    {
        state->h = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x68: // MOV L,B : L <- B
    {
        state->l = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x69: // MOV L,C : L <- C
    {
        state->l = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x6a: // MOV L,D : L <- D
    {
        state->l = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x6b: // MOV L,E : L <- E
    {
        state->l = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x6c: // MOV L,H : L <- H
    {
        state->l = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x6d: // MOV L,L : L <- L
    {
        state->l = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x6e: // MOV L,M : L <- (HL)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->l = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }
    case 0x6f: // MOV L, A : L <- A (move data in A to L)
        state->l = state->a;
        state->pc += 1;
        cycles = 5;
        break;

    case 0x70: // MOV M,B : (HL) <- B (move data at B into memory location HL)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;

        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->b;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x71: // MOV M,C : (HL) <- C
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->c;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x72: // MOV M,D : (HL) <- D
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;

        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->d;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x73: // MOV M,E : (HL) <- E
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->e;
        }

        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x74: // MOV M,H : (HL) <- H
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->h;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x75: // MOV M,L : (HL) <- L
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->l;
        }
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x76: // HLT : special
    {
        unimplemented_instruction(state);
        cycles = 7;
        break;
    }

    case 0x77: // MOV M, A : (HL) <- A (move data in A to memory location (HL))
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        // change this to use a protected write function for any opcode that writes to memory
        if (offset > 0x2000 && offset <= 0x4000)
        {
            state->memory[offset] = state->a;
        }

        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x78: // MOV A,B : A <- B
    {
        state->a = state->b;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x79: // MOV A,C : A <- C
    {
        state->a = state->c;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x7a: // MOV A, D : A <- D (move data in D to A)

    {
        state->a = state->d;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x7b: // MOV A, E : A <- E

    {
        state->a = state->e;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x7c: // MOV A, H : A <- H
    {
        state->a = state->h;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x7d: // MOV A,L : A <- L
    {
        state->a = state->l;
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0x7e: // MOV A, M : A <- (HL) (move data in memory location (HL) to A)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->a = state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x7f: // MOV A,A : A <- A
    {
        // can be no op since moving value to the same register
        state->a = state->a;
        state->pc += 1;
        cycles = 5;
        break;
    }
    case 0x80: // ADD B : A <- A + B
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->b;
        // if the result is zero, set the zero flag to zero
        // else clear the flag
        if ((answer & 0xff) == 0)
        {
            state->cc.z = 1;
        }
        else
        {
            state->cc.z = 0;
        }
        // if bit 7 is set, set the sign flag
        // else clear the sign flag
        if (answer & 0x80)
        {
            state->cc.s = 1;
        }
        else
        {
            state->cc.s = 0;
        }
        // carry flag
        if (answer > 0xff)
        {
            state->cc.cy = 1;
        }
        else
        {
            state->cc.cy = 0;
        }
        // parity - used a subroutine which is not created yet?
        state->cc.p = parity(answer & 0xff, 8);
        // set A
        state->a = answer & 0xff;
        // increment pc
        state->pc += 1;
        cycles = 4;
        break;
    }
    // add register - condensed
    case 0x81: // ADD C : A <- A + C
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->c;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x82: // ADD D : A <- A + D
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->d;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x83: // ADD E : A <- A + E
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->e;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x84: // ADD H : A <- A + H
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->h;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x85: // ADD L : A <- A + L
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->l;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    // add - memory example
    case 0x86: // ADD M : A <- A + (HL)
    {
        uint16_t offset = (state->h << 8) | (state->l);
        uint16_t answer = (uint16_t)state->a + state->memory[offset];
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x87: // ADD A : A <- A + A
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->a;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = parity(answer & 0xff, 8);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x88: // ADC B : A <- A + B + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->b + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x89: // ADC C : A <- A + C + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->c + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x8a: // ADC D : A <- A + D + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->d + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x8b: // ADC E : A <- A + E + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->e + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x8c: // ADC H : A <- A + H + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->h + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x8d: // ADC L : A <- A + L + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->l + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x8e: // ADC M : A <- A + (HL) + CY
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->memory[offset] + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x8f: // ADC A : A <- A + A + CY
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->a + (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x90: // SUB B: A <- A - B
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->b;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x91: // SUB C: A <- A - C
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->c;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x92: // SUB D : A <- A - D
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->d;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x93: // SUB E : A <- A - E
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->e;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x94: // SUB H : A <- A - H
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->h;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x95: // SUB L : A <- A - L
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->l;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x96: // SUB M : A <- A - (HL) (subtract value at HL from A and put into A)
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->memory[offset];
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x97: // SUB A : A <- A - A
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->a;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x98: // SBB B : A <- A - B - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->b - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x99: // SBB C : A <- A - C - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->c - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x9a: // SBB D : A <- A - D - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->d - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x9b: // SBB E : A <- A - E - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->e - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x9c: // SBB H : A <- A - H - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->h - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x9d: // SBB L : A <- A - L - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->l - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0x9e: // SBB M : A <- A - (HL) - CY
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->memory[offset] - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0x9f: // SBB A : A <- A - A - CY
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->a - (uint16_t)state->cc.cy;
        arithmetic_flags_A(state, answer);
        state->a = answer & 0xff;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa0: // ANA B : A <- A & B
    {
        state->a = state->a & state->b;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa1: // ANA C : A <- A & C
    {
        state->a = state->a & state->c;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa2: // ANA D : A <- A & D
    {
        state->a = state->a & state->d;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa3: // ANA E : A <- A & E
    {
        state->a = state->a & state->e;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa4: // ANA H : A <- A & H
    {
        state->a = state->a & state->h;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa5: // ANA L : A <- A & L
    {
        state->a = state->a & state->l;
        logic_flags_A(state);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa6: // ANA M : A <- A & (HL)
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->a = state->a & state->memory[offset];
        state->pc += 1;

        // set logic flags
        logic_flags_A(state);

        cycles = 7;
        break;
    }

    case 0xa7: // ANA A : A <- A & A (clear the CY flag but all other flags behave the same)
    {
        state->a = state->a & state->a;
        logic_flags_A(state);
        // increment pc
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa8: // XRA B : A <- A ^ B
    {
        state->a = state->a ^ state->b;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xa9: // XRA C : A <- A ^ C
    {
        state->a = state->a ^ state->c;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xaa: // XRA D : A <- A ^ D
    {
        state->a = state->a ^ state->d;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xab: // XRA E : A <- A ^ E
    {
        state->a = state->a ^ state->e;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xac: // XRA H : A <- A ^ H
    {
        state->a = state->a ^ state->h;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }
    case 0xad: // XRA L : A <- A ^ L
    {
        state->a = state->a ^ state->l;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xae: // XRA M : A <- A ^ (HL)
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->a = state->a ^ state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0xaf: // XRA A : A <- A ^ A (clear the CY and AC flag but all other flags behave the same)

    {
        state->a = state->a ^ state->a;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb0: // ORA B : A <- A | B
    {
        state->a = state->a | state->b;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb1: // ORA C : A <- A | C
    {
        state->a = state->a | state->c;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb2: // ORA D : A <- A | D
    {
        state->a = state->a | state->d;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb3: // ORA E : A <- A | E
    {
        state->a = state->a | state->e;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb4: // ORA H : A <- A | H
    {
        state->a = state->a | state->h;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb5: // ORA L : A <- A | L
    {
        state->a = state->a | state->l;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb6: // ORA M : A <- A | (HL)
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->a = state->a | state->memory[offset];
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0xb7: // ORA A : A <- A | A
    {
        state->a = state->a | state->l;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb8: // CMP B : A - B (difference between this and SUB is that A is unchanged.)
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->b;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xb9: // CMP C : A - C
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->c;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xba: // CMP D : A - D
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->d;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xbb: // CMP E : A - E
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->e;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xbc: // CMP H : A - H
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->h;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xbd: // CMP L : A - L
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->l;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xbe: // CMP M : A - (HL)
    {
        // get the value of (HL)
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->memory[offset];
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 7;
        break;
    }

    case 0xbf: // CMP A : A - A
    {
        uint16_t answer = (uint16_t)state->a - (uint16_t)state->a;
        arithmetic_flags_A(state, answer);
        state->pc += 1;
        cycles = 4;
        break;
    }

    case 0xc0: // RNZ : if NZ, RET
    {
        if (state->cc.z == 0)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    // stack group
    case 0xc1: // POP B : C <- (sp); B <- (sp+1); sp <- sp+2
    {
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc += 1;
        cycles = 10;
        break;
    }
    case 0xc2: // JNZ address : if NZ, PC <- adr
    {
        if (0 == state->cc.z)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xc3: // JMP
    {
        uint16_t addr = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
        state->pc = addr;
        cycles = 10;
        break;
    }

    case 0xc4: // CNZ adr - if NZ, CALL adr (if zero flag == 0)
    {

        if (state->cc.z == 0)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
        cycles = 10;
        break;
    }

    case 0xc5: // PUSH B
        // Push B register onto the stack
        state->memory[state->sp - 1] = state->b;
        // Push C register onto the stack
        state->memory[state->sp - 2] = state->c;
        state->sp -= 2;
        state->pc++;
        cycles = 11;

        break;
    case 0xc6: // ADI byte - add the second byte of the instruction to A.
    {
        uint16_t x = (uint16_t)state->a + (uint16_t)opcode[1];
        state->cc.z = ((x & 0xff) == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = parity((x & 0xff), 8);
        state->cc.cy = (x > 0xff);
        state->a = (uint8_t)x;
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xc7: // RST 0 : CALL $0
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $00
        state->pc = 0x00;
        cycles = 11;
        break;
    }

    case 0xc8: // RZ : if Z, RET (return if zero flag = 1)
    {
        if (state->cc.z == 1)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xc9: // RET
    {
        // pop the return address from the stack
        uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));

        // set the program counter to the return address
        state->pc = ret;

        // set the sp to the return address
        state->sp += 2;

        cycles = 10;
        break;
    }

    case 0xca: // JZ adr - if Z, PC <- adr
    {
        if (1 == state->cc.z) // if zero flag is set, then jump
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xcb: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0xcc: // CZ addr : if Z, CALL adr
    {
        if (state->cc.z == 1)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
    }
        cycles = 10;
        break;

    case 0xcd: // CALL
#ifdef FOR_CPUDIAG
        if (5 == ((opcode[2] << 8) | opcode[1]))
        {
            if (state->c == 9)
            {
                uint16_t offset = (state->d << 8) | (state->e);
                char *str = &state->memory[offset + 3]; // skip the prefix bytes
                while (*str != '$')
                    printf("%c", *str++);
                printf("\n");
                exit(0); // added this so it stops running after printing something, otherwise it just keeps executing.
            }
            else if (state->c == 2)
            {
                // saw this in the inspected code, never saw it called
                printf("print char routine called\n");
            }
        }
        else if (0 == ((opcode[2] << 8) | opcode[1]))
        {
            exit(0);
        }
        else
#endif

        {
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
            cycles = 17;
            break;
        }

    case 0xce: // ACI D8 - add immediate with carry.
    {
        uint16_t x = (uint16_t)state->a + (uint16_t)state->cc.cy + (uint16_t)opcode[1];
        state->cc.z = ((x & 0xff) == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = parity((x & 0xff), 8);
        state->cc.cy = (x > 0xff);
        state->a = (uint8_t)x;
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xcf: // RST 1 - CALL $8
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $08
        state->pc = 0x0008;
        cycles = 11;
        break;
    }

    case 0xd0: // RNC : return if no carry (carry = 0)
    {
        if (state->cc.cy == 0)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xd1: // POP D

    {
        // pop the stack into the E register
        state->e = state->memory[state->sp];
        // pop the stack into the D register
        state->d = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc++;
        cycles = 10;
        break;
    }

    case 0xd2: // JNC adr - if NCY, PC<-adr
    {
        if (0 == state->cc.cy)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xd3: // OUT - content of A is placed on the bus to be transmitted to the specified port.
    {
        uint8_t port = state->memory[state->pc + 1];
        redirect_output(state->a, port);
        state->pc += 2;
        cycles = 10;
        break;
    }

    case 0xd4: // CNC adr : if NCY, CALL adr (if carry flag is 0)
    {
        if (state->cc.cy == 0)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
        cycles = 17;
        break;
    }

    case 0xd5: // PUSH D
        // Push D register onto the stack
        state->memory[state->sp - 1] = state->d;
        // Push E register onto the stack
        state->memory[state->sp - 2] = state->e;
        state->sp -= 2;
        state->pc++;
        cycles = 11;
        break;

    case 0xd6: // SUI D8 : A <- A - data . subtract immediate from A
    {
        uint16_t x = (uint16_t)state->a - (uint16_t)opcode[1];
        state->cc.z = ((x & 0xff) == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = parity((x & 0xff), 8);
        state->cc.cy = (x > 0xff);
        state->a = (uint8_t)x;
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xd7: // RST 2 : CALL $10
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $10
        state->pc = 0x0010;
        cycles = 11;
        break;
    }

    case 0xd8: // RC : if CY, RET (return if carry = 1)
    {
        if (state->cc.cy == 1)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xd9: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0xda: // JC adr : if CY, PC<-adr
    {
        if (0 != state->cc.cy)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
        }
        else
        {
            state->pc += 3;
        }
        cycles = 10;
        break;
    }

    case 0xdb: // IN D8
    {
        unimplemented_instruction(state);
        break;
    }

    case 0xdc: // CC adr : if CY, CALL adr
    {
        if (state->cc.cy != 0)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
    }
        cycles = 10;
        break;

    case 0xdd: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0xde: // SBI D8 : A <- A - data - CY
    {
        uint16_t x = (uint16_t)state->a - (uint16_t)opcode[1] - (uint16_t)state->cc.cy;
        state->cc.z = ((x & 0xff) == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = parity((x & 0xff), 8);
        state->cc.cy = (x > 0xff);
        state->a = (uint8_t)x;
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xdf: // RST 3 : CALL $18
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $18
        state->pc = 0x18;
        cycles = 11;
        break;
    }

    case 0xe0: // RPO : if PO, RET (ret if parity odd, p = 0)
    {
        if (state->cc.p == 0)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xe1: // POP H
        // pop the stack into the L register
        state->l = state->memory[state->sp];
        // pop the stack into the H register
        state->h = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc++;
        cycles = 10;

        break;

    case 0xe2: // JPO adr : if PO, PC <- adr (parity odd, p = 0)
    {
        if (0 == state->cc.p)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xe3: // XTHL : L <-> (SP); H <-> (SP+1) (exchange stack top with H and L)
    {
        // save h and l values
        uint8_t h = state->h;
        uint8_t l = state->l;

        // save the memory values in the stack to l and h
        state->l = state->memory[state->sp];
        state->h = state->memory[state->sp + 1];

        // write h and l to stack - this should be protected? try this for now.
        state->memory[state->sp] = l;
        state->memory[state->sp + 1] = h;

        state->pc += 1;
        cycles = 18;

        break;
    }

    case 0xe4: // CPO adr : if PO (parity odd, p = 0), CALL adr
    {
        if (state->cc.p == 0)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
    }
        cycles = 17;
        break;

    case 0xe5: // PUSH H
        // push H register onto the stack
        state->memory[state->sp - 1] = state->h;
        // Push L register onto the stack
        state->memory[state->sp - 2] = state->l;
        state->sp -= 2;
        state->pc++;
        cycles = 11;
        break;
    case 0xe6: // ANI - And immediate with A. The CY and AC flags are cleared.
    {
        // uint8_t data = state->memory[state->pc + 1]; // this should be and with memory?
        // state->a = state->a & data;

        state->a = state->a & opcode[1];

        // set the flags
        state->cc.z = (state->a == 0);
        state->cc.s = (0x80 == (state->a & 0x80));
        state->cc.p = parity(state->a, 8);
        state->cc.cy = 0; // ANI operation always clears CY flag
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xe7: // RST 4 : CALL $20
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $00
        state->pc = 0x20;
        cycles = 11;
        break;
    }

    case 0xe8: // RPE : if PE, RET (return on parity even, p = 1)
    {
        if (state->cc.p == 1)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xe9: // PCHL : PC.hi <- H; PC.lo <- L
    {
        state->pc = (state->h << 8 | state->l);
        cycles = 5;
        break;
    }

    case 0xea: // JPE adr : if PE (parity even, cc.p = 1), PC <- adr
    {
        if (1 == state->cc.p)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xeb: // XCHG : H <-> D; L <-> E
    {
        uint8_t tmp = state->h;
        uint8_t tmp2 = state->l;
        state->h = state->d;
        state->l = state->e;
        state->d = tmp;
        state->e = tmp2;
        state->pc += 1;
        cycles = 17;
        break;
    }

    case 0xec: // CPE adr : if PE, CALL adr (parity even, p = 1)
    {
        if (state->cc.p == 1)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
    }
        cycles = 17;
        break;

    case 0xed: // none
    {
        unimplemented_instruction(state);
        break;
    }

    case 0xee: // XRI D8 : A <- A ^ data
    {
        uint8_t data = state->memory[state->pc + 1];
        state->a = state->a ^ data;

        // set the flags
        state->cc.z = (state->a == 0);
        state->cc.s = (0x80 == (state->a & 0x80));
        state->cc.p = parity(state->a, 8);
        state->cc.cy = 0; // operation always clears CY flag
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xef: // RST 5 : CALL $28
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $00
        state->pc = 0x28;
        cycles = 11;
        break;
    }

    case 0xf0: // RP : if P, RET (return if positive, sign = 0)
    {
        if (state->cc.s == 0)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xf1: // POP PSW
    {
        state->a = state->memory[state->sp + 1];
        uint8_t psw = state->memory[state->sp];
        state->cc.z = (0x01 == (psw & 0x01));
        state->cc.s = (0x02 == (psw & 0x02));
        state->cc.p = (0x04 == (psw & 0x04));
        state->cc.cy = (0x08 == (psw & 0x08));
        state->cc.ac = (0x10 == (psw & 0x10));
        state->sp += 2;
        state->pc += 1;
        cycles = 10;

        // for (int i = 0x23ff; i == state->sp; i--)
        // {
        //     printf("%04x ", state->memory[i]);
        // }
        // printf("\n");

        break;
    }

    case 0xf2: // JP adr : if S=0 PC <- adr (jump on positive, s=0)
    {
        if (0 == state->cc.s)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xf3: // DI
        // implement disable interrupts instruction here
        state->int_enable = 0;
        state->pc += 1;
        cycles = 4;
        break;

    case 0xf4: // CP adr - call on positive (sign = 0)
    {
        if (state->cc.s == 0)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
    }
        cycles = 17;
        break;

    case 0xf5: // PUSH PSW : flags <- (sp); A <- (sp+1); sp <- sp+2
    {
        // print the stack pointer before
        // printf("stack pointer before: %04x\n", state->sp);
        state->memory[state->sp - 1] = state->a;
        uint8_t psw = (state->cc.z |
                       state->cc.s << 1 |
                       state->cc.p << 2 |
                       state->cc.cy << 3 |
                       state->cc.ac << 4);
        state->memory[state->sp - 2] = psw;
        state->sp -= 2;
        state->pc += 1;
        cycles = 11;

        // print the stack pointer after
        // printf("stack pointer after: %04x\n", state->sp);

        break;
    }

    case 0xf6: // ORI D8 : A <- A | data. CY and AC flags are cleared.
    {
        uint8_t data = state->memory[state->pc + 1];
        state->a = state->a | data;

        // set the flags
        state->cc.z = (state->a == 0);
        state->cc.s = (0x80 == (state->a & 0x80));
        state->cc.p = parity(state->a, 8);
        state->cc.cy = 0; // operation always clears CY flag
        state->pc += 2;
        cycles = 7;
        break;
    }

    case 0xf7: // RST 6 : CALL $30
    {
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc + 1;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $30
        state->pc = 0x30;
        cycles = 11;
        break;
    }

    case 0xf8: // RM : if M, RET (return on minus, sign = 1)
    {
        if (state->cc.s == 1)
        {
            // perform ret
            // pop the return address from the stack
            uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
            state->sp += 2;
            // set the program counter to the return address
            state->pc = ret;
        }
        else
        {
            // increment pc
            state->pc += 1;
        }
        cycles = 11;
        break;
    }

    case 0xf9: // SPHL : SP=HL (move registers H and L to SP)
    {
        state->sp = (state->h << 8 | state->l);
        state->pc += 1;
        cycles = 5;
        break;
    }

    case 0xfa: // JM adr : if M, PC <- adr (jump on minus, s=1)
    {
        if (1 == state->cc.s)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 3;
        cycles = 10;
        break;
    }

    case 0xfb: // EI : special - enable interrupt
        // this should set an interrupt_enabled flag in the processor state.

        // printf("enable interrupt\n");

        state->int_enable = 1;
        state->pc += 1;

        // printf("interrupt state: %d\n", state->int_enable);
        // printf("press key to continue\n");
        // getchar();
        cycles = 4;
        break;

    case 0xfc: // CM adr : if M, CALL adr (call on minus, s = 1)
    {

        if (state->cc.s == 1)
        {
            // call addr
            uint16_t ret = state->pc + 3; // address to return to after the CALL
            // push return address onto the stack
            state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
            state->memory[state->sp - 2] = ret & 0xFF;
            state->sp = state->sp - 2;
            // jump to address specified in the CALL instruction
            uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
            state->pc = target;
        }
        else
        {
            // increment pc
            state->pc += 3;
        }
        cycles = 17;
        break;
    }

    case 0xfd: // none
    {
        unimplemented_instruction(state);
        break;
    }

    // compare example
    case 0xfe: // CPI byte : compare immediate with accumulator
    {
        uint8_t x = state->a - opcode[1];
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));
        // the reference had to pick what to do with parity flag
        state->cc.p = parity(x & 0xff, 8);
        state->cc.cy = (state->a < opcode[1]);
        state->pc += 2;
        cycles = 7;
        break;
    }
    case 0xff: // RST 7: CALL $38
        // Save the current PC on the stack before jumping
        {
            uint16_t ret = state->pc + 1;
            state->memory[state->sp - 1] = (ret >> 8) & 0xff; // high part of PC
            state->memory[state->sp - 2] = (ret & 0xff);      // low part of PC
            state->sp = state->sp - 2;
            // Jump to the address $38
            state->pc = 0x38;
            cycles = 11;
            break;
        }

    default:
        unimplemented_instruction(state);
        break;
    }

// print the processor state condition codes (flags)
#ifdef DEBUG
    printf("\tC=%d,P=%d,S=%d,Z=%d\n", state->cc.cy, state->cc.p,
           state->cc.s, state->cc.z);
    // registers
    printf("\tA $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x PC %04x\n",
           state->a, state->b, state->c, state->d,
           state->e, state->h, state->l, state->sp, state->pc);
#endif

    return cycles;
}