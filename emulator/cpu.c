#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./cpu.h"
#include "../utils/disasm.h"

void UnimplementedInstruction(State8080 *state);
int Emulate8080Op(State8080 *state);
int Parity(unsigned int value);

// display if the instruction is not implemented yet and stop the emulation
void UnimplementedInstruction(State8080 *state)
{
    // pc will have advanced one, so undo that
    printf("Error: Unimplemented instruction\n");
    exit(1);
}

// determine the parity of a number
// return 1 if even parity, 0 if odd parity
// gcc has a built in parity function that gives odd parity,
// so we should invert the result
int Parity(unsigned int value)
{
    return !__builtin_parity(value);
}

// function to emulate 8080 instructions
int Emulate8080Op(State8080 *state)
{
    unsigned char *opcode = &state->memory[state->pc];

    // add in the code from the disassembler so we know what instruction
    // is about to be executed.
    Disassemble8080Op(state->memory, state->pc);

    // giant switch statement for all of the opcodes
    // see http://www.emulator101.com/finishing-the-cpu-emulator.html
    // for opcodes we need for Space Invaders.
    // for now, only add the ones that we need to complete to allow the game to run.
    switch (*opcode)
    {
    case 0x00:          // NOP
        state->pc += 1; // for the opcode
        break;
    case 0x01: // LXI    B,word : B <- byte 3, C <- byte 2
        state->c = opcode[1];
        state->b = opcode[2];
        state->pc += 2;
        break;
    case 0x02:                                      // STAX B : (BC) <- A (store A in memory location BC)
        uint16_t offset = state->b << 8 | state->c; // for the memory location of register pair BC
        state->memory[offset] = state->a;
        break;
    case 0x03: // INX B : BC <- BC+1 (increment register pair) B. no condition flags affected
        uint16_t value = state->b << 8 | state->c;
        value += 1;
        state->b = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->c = value & 0xff;          // store the lower 8 bits in c
        break;
    case 0x04: // INR B : B <- B+1 - condition flags will be changed, see the add example
        UnimplementedInstruction(state);
        break;
    case 0x05: // DCR B : B <- B-1
        UnimplementedInstruction(state);
        break;

    case 0x06: // MVI B,D8 : B <- byte 2
        state->b = opcode[1];
        state->pc += 2;
        break;

    case 0x09: // DAD B : HL = HL + BC
        UnimplementedInstruction(state);
        break;

    case 0x0d: // DCR C : C <-C-1
        UnimplementedInstruction(state);
        break;

    case 0x0e: // MVI C, D8 : C <- byte 2
        UnimplementedInstruction(state);
        break;

    // rotate instruction example
    case 0x0f: // RRC : A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
    {
        uint8_t x = state->a;
        state->a = ((x & 1) << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
        state->pc += 1;
    }
    break;

    case 0x11: // LX1 D, D16 : D <- byte 3, E <- byte 2
        state->e = opcode[1];
        state->d = opcode[2];
        state->pc += 2;
        break;
        break;

    case 0x13: // INX D : DE <- DE + 1
        UnimplementedInstruction(state);
        break;

    case 0x19: // DAD D : HL = HL + DE (only affects the carry flag)
        UnimplementedInstruction(state);
        break;

    case 0x1a: // LDAX D : A <- (DE)
        UnimplementedInstruction(state);
        break;

    case 0x1f: // RAR : A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
    {
        uint8_t x = state->a;
        state->a = (state->cc.cy << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
    }
    break;

    case 0x21: // LXI H, D16 : H <- byte 3, L <- byte 2
        state->h = opcode[2];
        state->l = opcode[1];
        state->pc += 2;
        break;

    case 0x23: // INX H : HL <- HL + 1
        UnimplementedInstruction(state);
        break;

    case 0x26: // MVI H, D8 : H <- byte 2
        UnimplementedInstruction(state);
        break;

    case 0x29: // DAD H : HL = HL + HI
        UnimplementedInstruction(state);
        break;

    case 0x2f: // CMA (not) : A <- !A
        state->a = ~state->a;
        // CMA does not affect the flags
        break;

    case 0x31: // LXI SP, D16 : SP.hi <- byte 3, SP.lo <- byte 2
        state->sp = (opcode[2] << 8) | opcode[1];
        state->pc += 3;
        break;

    case 0x32: // STA addr : (adr) <- A
        UnimplementedInstruction(state);
        break;

    case 0x36: // MVI M, D8 : (HL) <- byte 2
        UnimplementedInstruction(state);
        break;

    case 0x3a: // LDA addr : A <- (adr)
        UnimplementedInstruction(state);
        break;

    case 0x3e: // MVI A, D8 : A <- byte 2
        UnimplementedInstruction(state);
        break;

    // fill in here
    case 0x41: // MOV    B,C : B <- C
        state->b = state->c;
        break;
    case 0x42: // MOV    B,D : B <- D
        state->b = state->d;
        break;
    case 0x43: // MOV    B,E : B <- E
        state->b = state->e;
        break;

    case 0x56: // MOV D, M : D <- (HL)
        UnimplementedInstruction(state);
        break;

    case 0x5e: // MOV E, M : E <- (HL)
        UnimplementedInstruction(state);
        break;

    case 0x66: // MOV H, M : H <- (HL)
        UnimplementedInstruction(state);
        break;

    case 0x6f: // MOV L, A : L <- A
        UnimplementedInstruction(state);
        break;

    case 0x77: // MOV M, A : (HL) <- A
        UnimplementedInstruction(state);
        break;

    case 0x7a: // MOV A, D : A <- D
        UnimplementedInstruction(state);
        break;

    case 0x7b: // MOV A, E : A <- E
        UnimplementedInstruction(state);
        break;

    case 0x7c: // MOV A, H : A <- H
        UnimplementedInstruction(state);
        break;

    case 0x7e: // MOV A, M : A <- (HL)
        UnimplementedInstruction(state);
        break;

    // arithmetic instruction examples
    // these will change the CPU condition codes (flags)
    // add register
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
        state->cc.p = Parity(answer & 0xff);
        // set A
        state->a = answer & 0xff;
        break;
    }
    // add register - condensed
    case 0x81: // ADD C : A <- A + C
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->c;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = Parity(answer & 0xff);
        state->a = answer & 0xff;
    }
    break;

    // memory form example
    case 0x86: // ADD M : A <- A + (HL)
    {
        uint16_t offset = (state->h << 8) | (state->l);
        uint16_t answer = (uint16_t)state->a + state->memory[offset];
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = Parity(answer & 0xff);
        state->a = answer & 0xff;
    }
    break;

    case 0xa7: // ANA A : A <- A & A
        state->a = state->a & state->a;
        break;

    case 0xaf: // XRA A : A <- A ^ B
        state->a = state->a ^ state->b;
        break;

    // stack group
    case 0xc1: // POP B : C <- (sp); B <- (sp+1); sp <- sp+2
    {
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp + 1];
        state->sp += 2;
    }
    break;

    // branch group

    // JMP instructions
    case 0xc2: // JNZ address : if NZ, PC <- adr
    {
        if (0 == state->cc.z)
        {
            state->pc = (opcode[2] << 8) | opcode[1];
        }
        else
        {
            // branch not taken
            state->pc += 2;
        }
    }
    break;

    case 0xc3: // JMP address : PC <= adr
        state->pc = (opcode[2] << 8) | opcode[1];
        break;

    case 0xc5: // PUSH B : (sp-2)<-C; (sp-1)<-B; sp <- sp - 2
    {
        state->memory[state->sp - 1] = state->b;
        state->memory[state->sp - 2] = state->c;
        state->sp = state->sp - 2;
    }
    break;

    // add immediate - example
    case 0xc6: // ADI byte : A <- A + byte
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)opcode[1];
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy = (answer > 0xff);
        state->cc.p = Parity(answer & 0xff);
        state->a = answer & 0xff;
    }
    break;

    case 0xc9: // RET : PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2
        state->pc = state->memory[state->sp] | (state->memory[state->sp + 1] << 8);
        state->sp += 2;
        break;

    case 0xcd: // CALL address - SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr
    {
        uint16_t ret = state->pc + 2;
        state->memory[state->sp - 1] = (ret >> 8) & 0xff;
        state->memory[state->sp - 2] = (ret & 0xff);
        state->sp = state->sp - 2;
        state->pc = (opcode[2] << 8) | opcode[1];
    }
    break;

    case 0xd1: // POP D : E <- (sp); D <- (sp+1); sp <- sp+2
    {
        state->memory[state->sp + 1] = state->d;
        state->memory[state->sp] = state->e;
        state->sp = state->sp + 2;
    }
    break;

    case 0xd3:          // OUT D8 : used to talk to hardware
        state->sp += 2; // for now, make this skip over the data byte
        break;

    case 0xd5: // PUSH D : (sp-2)<-E; (sp-1)<-D; sp <- sp - 2
    {
        state->memory[state->sp - 1] = state->d;
        state->memory[state->sp - 2] = state->e;
        state->sp = state->sp - 2;
    }
    break;

    case 0xe1: // POP H : L <- (sp); H <- (sp+1); sp <- sp+2
    {
        state->memory[state->sp + 1] = state->h;
        state->memory[state->sp] = state->l;
        state->sp = state->sp + 2;
    }
    break;

    case 0xe5: // PUSH H : (sp-2)<-L; (sp-1)<-H; sp <- sp - 2
    {
        state->memory[state->sp - 1] = state->h;
        state->memory[state->sp - 2] = state->l;
        state->sp = state->sp - 2;
    }
    break;

    // logical operation example
    case 0xe6: // ANI byte : A <- A & data
    {
        uint8_t x = state->a & opcode[1];
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = Parity(x & 0xff);
        state->cc.cy = 0; // data book says ANI clears CY
        state->a = x;
        state->pc++; // for the data byte
    }
    break;

    case 0xeb: // XCHG : H <-> D; L <-> E
        uint8_t tmp = state->h;
        state->h = state->d;
        state->d = tmp;

        tmp = state->l;
        state->l = state->e;
        state->e = tmp;
        break;

    case 0xf1: // POP PSW
    {
        state->a = state->memory[state->sp + 1];
        uint8_t psw = state->memory[state->sp];
        state->cc.z = (0x01 == (psw & 0x01));
        state->cc.s = (0x02 == (psw & 0x02));
        state->cc.p = (0x04 == (psw & 0x04));
        state->cc.cy = (0x05 == (psw & 0x08));
        state->cc.ac = (0x10 == (psw & 0x10));
        state->sp += 2;
    }
    break;

    case 0xf5: // PUSH PSW : flags <- (sp); A <- (sp+1); sp <- sp+2
    {
        state->memory[state->sp - 1] = state->a;
        uint8_t psw = (state->cc.z |
                       state->cc.s << 1 |
                       state->cc.p << 2 |
                       state->cc.cy << 3 |
                       state->cc.ac << 4);
        state->memory[state->sp - 2] = psw;
        state->sp = state->sp - 2;
    }
    break;

    case 0xfb: // EI : special - enable interrupt
        // this should set an interrupt_enabled flag in the processor state.
        UnimplementedInstruction(state);
        break;

    // compare example
    case 0xfe: // CPI byte : compare immediate with accumulator
    {
        uint8_t x = state->a - opcode[1];
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));

        // the reference had to pick what to do with parity flag
        state->cc.p = Parity(x & 0xff);
        state->cc.cy = (state->a < opcode[1]);
        state->pc++;
    }
    break;
    case 0xff: // RST : CALL $38
        UnimplementedInstruction(state);
        break;
    default:
        break;
    }

    // print out the processor state
    // condition codes (flags)
    printf("\tC=%d,P=%d,S=%d,Z=%d\n", state->cc.cy, state->cc.p,
           state->cc.s, state->cc.z);
    // registers
    printf("\tA $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n",
           state->a, state->b, state->c, state->d,
           state->e, state->h, state->l, state->sp);

    return 0;
}