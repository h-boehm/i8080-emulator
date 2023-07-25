#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "./cpu.h"
#include "../utils/disasm.h"

// display if the instruction is not implemented yet
// and stop the emulation
void unimplemented_instruction(State8080 *state) {
    // pc will have advanced one, so undo that
    printf("Error: unimplemented instruction\n");
    exit(1);
}

// handles output from the game; just printing here for now
void redirect_output(uint8_t byte, uint8_t port) {
    printf("Output to port %d: %02X\n", port, byte);
}

// determines parity of a number
int parity(int x, int size) {
    int i;
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (i = 0; i < size; i++) {
        if (x & 0x1) {
            p++;
        }
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}

// sets flags after logical instruction on A register
void LogicFlagsA(State8080 *state) {
	state->cc.cy = state->cc.ac = 0;
	state->cc.z = (state->a == 0);
	state->cc.s = (0x80 == (state->a & 0x80));
	state->cc.p = parity(state->a, 8);
}

// function to emulate 8080 instructions
int Emulate8080Op(State8080 *state) {

    unsigned char *opcode = &state->memory[state->pc];

    // add the code from the disassembler so we know which instruction
    // is about to be executed.
    Disassemble8080Op(state->memory, state->pc);

    // giant switch statement for all the opcodes
    // see http://www.emulator101.com/finishing-the-cpu-emulator.html
    // for opcodes we need for Space Invaders.
    // for now, only add the ones that we need to complete to allow the game to run.
    switch (*opcode) {

    case 0x00: // NOP
    {
        state->pc += 1; // for the opcode
        break;
    }
    case 0x01: // LXI B, word : B <- byte 3, C <- byte 2 (load immediate into register pair BC)
    {
        state->c = opcode[1];
        state->b = opcode[2];
        state->pc += 3;
        break;
    }
    case 0x02: // STAX B : (BC) <- A (store A in memory location BC)
    {
        uint16_t offset = (uint16_t)state->b << 8 | (uint16_t)state->c; // for the memory location of register pair BC
        state->memory[offset] = state->a;
        state->pc += 1;
        break;
    }
    case 0x03: // INX B : BC <- BC+1 (increment register pair) B. no condition flags affected
    {
        uint16_t value = (uint16_t)state->b << 8 | (uint16_t)state->c;
        value += 1;
        state->b = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->c = value & 0xff;          // store the lower 8 bits in c
        state->pc += 1;
        break;
    }
    case 0x04: // INR B : B <- B+1 (increment B) - all condition flags will change execpt CY. see ADD example
    { 
        uint16_t answer = (uint16_t)state->b + 1;
        // if the result is zero, set the zero flag to zero
        // else clear the flag
        if ((answer & 0xff) == 0) {
            state->cc.z = 1;
        }
        else {
            state->cc.z = 0;
        }
        // if bit 7 is set, set the sign flag
        // else clear the sign flag
        if (answer & 0x80) {
            state->cc.s = 1;
        }
        else {
            state->cc.s = 0;
        }
        // parity - used a subroutine which is not created yet?
        state->cc.p = parity(answer & 0xff, 8);
        // set A
        state->b = answer & 0xff;
        state->pc += 1;
        break;
    }
    case 0x05: // DCR B : B <- B-1 (decrement B) - all condition flags affected except CY
    {
        // condensed version
        uint16_t answer = (uint16_t)state->b - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->b = answer & 0xff;
        state->pc += 1;
        break;
    }
    case 0x06: // MVI B,D8 : B <- byte 2 (move immediate into B)
    {
        state->b = opcode[1];
        state->pc += 2;
        break;
    }
    case 0x09: // DAD B : HL = HL + BC (add register pair BC to register pair HL). only the CY flag is affected.
    {
        uint16_t value = (uint16_t)state->b << 8 | (uint16_t)state->c;
        uint16_t hl_value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = value + hl_value;
        // set the carry flag
        if (answer > 0xffff) {
            state->cc.cy = 1;
        }
        else {
            state->cc.cy = 0;
        }
        state->pc += 1;
        break;
    }
    case 0x0a: // LDAX B : A <- (BC) (load content of memory location (BC) into A)
    {
        uint16_t offset = state->b << 8 | state->c; // for the memory location in register pair DE
        state->a = state->memory[offset];
        state->pc += 1;
        break;
    }
    case 0x0b: // DCX B : BC = BC-1 (decrement BC)
    {
        uint16_t value = (uint16_t)state->b << 8 | (uint16_t)state->c;
        value -= 1;
        state->b = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->c = value & 0xff;          // store the lower 8 bits in c
        state->pc += 1;
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
        break;
    }    
    case 0x0e: // MVI C, D8 : C <- byte 2
    {
        state->c = opcode[1];
        state->pc += 2;
        break;
    }
    // rotate instruction
    case 0x0f: // RRC : A = A >> 1; bit 7 = prev bit 0; CY = prev bit
    {
        uint8_t x = state->a;
        state->a = ((x & 1) << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
        state->pc += 1;
    }
    case 0x11: // LX1 D, D16 : D <- byte 3, E <- byte 2
        state->e = opcode[1];
        state->d = opcode[2];
        state->pc += 3;
        break;
    case 0x13: // INX D : DE <- DE + 1 (increment register pair DE). no condition flags are affected
        uint16_t value = (uint16_t)state->d << 8 | (uint16_t)state->e;
        value += 1;
        state->d = (value & 0xff00) >> 8; // store the higher 8 bits in d
        state->e = value & 0xff;          // store the lower 8 bits in e
        state->pc += 1;
        break;
    case 0x14: // INR D : D <- D+1 (affects condition flags)
        uint16_t answer = (uint16_t)state->d - 1;
        state->cc.z = ((answer & 0xff) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.p = parity(answer & 0xff, 8);
        state->d = answer & 0xff;
        state->pc += 1;
        break;
    case 0x19: // DAD D : HL = HL + DE (only affects the carry flag)
    {
        uint16_t value = (uint16_t)state->d << 8 | (uint16_t)state->e;
        uint16_t hl_value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = value + hl_value;
        // set the carry flag
        if (answer > 0xffff) {
            state->cc.cy = 1;
        }
        else {
            state->cc.cy = 0;
        }
        state->pc += 1;
        break;
    }
    case 0x1a: // LDAX D : A <- (DE). load content of memory location (DE) into A
        uint16_t offset = state->d << 8 | state->e; // for the memory location in register pair DE
        state->a = state->memory[offset];
        state->pc += 1;
        break;
    case 0x1f: // RAR : A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0 (rotate right through carry)
        uint8_t x = state->a;
        state->a = (state->cc.cy << 7) | (x >> 1);
        state->cc.cy = (1 == (x & 1));
        state->pc += 1;
        break;
    case 0x21: // LXI H, D16 : H <- byte 3, L <- byte 2
    {
        state->h = opcode[2];
        state->l = opcode[1];
        state->pc += 3;
        break;
    }
    case 0x23: // INX H : HL <- HL + 1 (increment register pair HL)
    {
        uint16_t value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        value += 1;
        state->h = (value & 0xff00) >> 8; // store the higher 8 bits in b
        state->l = value & 0xff;          // store the lower 8 bits in c
        state->pc += 1;
        break;
    }
    case 0x26: // MVI H, D8 : H <- byte 2
        state->h = opcode[1];
        state->pc += 2;
        break;
    case 0x29: // DAD H : HL = HL + HL (affects carry flag)
    {
        uint16_t value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t hl_value = (uint16_t)state->h << 8 | (uint16_t)state->l;
        uint16_t answer = value + hl_value;
        // set the carry flag
        if (answer > 0xffff) {
            state->cc.cy = 1;
        }
        else {
            state->cc.cy = 0;
        }
        state->pc += 1;
        break;
    }
    case 0x2f: // CMA (not) : A <- !A (no condition flags are affected)
        state->a = ~state->a;
        state->pc += 1;
        break;
    case 0x31: // LXI SP, D16 : SP.hi <- byte 3, SP.lo <- byte 2
    {
        state->sp = (opcode[2] << 8) | opcode[1];
        state->pc += 3;
        break;
    }
    case 0x32: // STA addr : (addr) <- A (store A into memory location addr or bytes 2 and 3)
    {
        uint16_t offset = opcode[2] << 8 | opcode[1];
        state->memory[offset] = state->a;
        state->pc += 3;
        break;
    }
    case 0x36: // MVI M, D8 : (HL) <- byte 2 (move byte 2 to the memory location in (HL))
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->memory[offset] = opcode[1];
        state->pc += 2;
        break;
    }
    case 0x3a: // LDA addr : A <- (adr) (load value stored at addr (bytes 2 and 3) to A)
    {
        uint16_t offset = opcode[2] << 8 | opcode[1];
        state->a = state->memory[offset];
        state->pc += 3;
        break;
    }
    case 0x3e: // MVI A, D8 : A <- byte 2 (move byte 2 into A)
        state->a = opcode[1];
        state->pc += 2;
        break;
    case 0x40: // MOV B,B
        // this is a no-op because it's moving the value to the same register.
        state->pc++;
        break;
    case 0x41: // MOV    B,C : B <- C
        state->b = state->c;
        state->pc += 1;
        break;
    case 0x42: // MOV    B,D : B <- D
        state->b = state->d;
        state->pc += 1;
        break;
    case 0x43: // MOV    B,E : B <- E
        state->b = state->e;
        state->pc += 1;
        break;
    case 0x56: // MOV D, M : D <- (HL) (move data at memory location (HL) to D)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->d = state->memory[offset];
        state->pc += 1;
        break;
    }
    case 0x5e: // MOV E, M : E <- (HL) (move data at memory location (HL) to E)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->e = state->memory[offset];
        state->pc += 1;
        break;
    }
    case 0x66: // MOV H, M : H <- (HL) (move data at memory location (HL) to H)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->h = state->memory[offset];
        state->pc += 1;
        break;
    }
    case 0x6f: // MOV L, A : L <- A (move data in A to L)
        state->l = state->a;
        state->pc += 1;
        break;
    case 0x77: // MOV M, A : (HL) <- A (move data in A to memory location (HL))
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->memory[offset] = state->a;
        state->pc += 1;
        break;
    }
    case 0x7a: // MOV A, D : A <- D (move data in D to A)
        state->a = state->d;
        state->pc += 1;
        break;
    case 0x7b: // MOV A, E : A <- E
        state->a = state->e;
        state->pc += 1;
        break;
    case 0x7c: // MOV A, H : A <- H
        state->a = state->h;
        state->pc += 1;
        break;
    case 0x7e: // MOV A, M : A <- (HL) (move data in memory location (HL) to A)
    {
        uint16_t offset = (uint16_t)state->h << 8 | (uint16_t)state->l;
        state->a = state->memory[offset];
        state->pc += 1;
        break;
        // arithmetic instruction examples
        // these will change the CPU condition codes (flags)
        // add register
    }
    case 0x80: // ADD B : A <- A + B
    {
        uint16_t answer = (uint16_t)state->a + (uint16_t)state->b;
        // if the result is zero, set the zero flag to zero
        // else clear the flag
        if ((answer & 0xff) == 0) {
            state->cc.z = 1;
        }
        else {
            state->cc.z = 0;
        }
        // if bit 7 is set, set the sign flag
        // else clear the sign flag
        if (answer & 0x80) {
            state->cc.s = 1;
        }
        else {
            state->cc.s = 0;
        }
        // carry flag
        if (answer > 0xff) {
            state->cc.cy = 1;
        }
        else {
            state->cc.cy = 0;
        }
        // parity - used a subroutine which is not created yet?
        state->cc.p = parity(answer & 0xff, 8);
        // set A
        state->a = answer & 0xff;
        // increment pc
        state->pc += 1;
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
    break;
    }
    case 0xa7: // ANA A : A <- A & A (clear the CY flag but all other flags behave the same)
        state->a = state->a & state->a; LogicFlagsA(state);
        break; 
    case 0xaf: // XRA A : A <- A ^ B (clear the CY and AC flag but all other flags behave the same)
        state->a = state->a ^ state->b;
        // set the condition flags
        state->cc.z = ((state->a & 0xff) == 0);
        state->cc.s = ((state->a & 0x80) != 0);
        state->cc.p = parity(state->a & 0xff, 8);
        // clear the cy and ac flags
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->pc += 1;
        break;
    // stack group
    case 0xc1: // POP B : C <- (sp); B <- (sp+1); sp <- sp+2
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc += 1;
        break;
    case 0xc2: // JNZ address
        if (0 == state->cc.z)
            state->pc = (opcode[2] << 8) | opcode[1];
        else
            state->pc += 2;
        break;
    case 0xc3: // JMP
        uint16_t addr = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
        state->pc = addr;
        break;
    case 0xc5: // PUSH B
        // Push B register onto the stack
        state->memory[state->sp - 1] = state->b;
        // Push C register onto the stack
        state->memory[state->sp - 2] = state->c;
        state->sp -= 2;
        state->pc++;
        break;
    case 0xc6: // ADI byte
    {
        uint16_t x = (uint16_t) state->a + (uint16_t) opcode[1];
        state->cc.z = ((x & 0xff) == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = parity((x&0xff), 8);
        state->cc.cy = (x > 0xff);
        state->a = (uint8_t) x;
        state->pc++;
        break;
    }
    case 0xcd: // CALL
    {
        uint16_t ret = state->pc + 3; // address to return to after the CALL
        // push return address onto the stack
        state->memory[state->sp - 1] = (ret >> 8) & 0xFF;
        state->memory[state->sp - 2] = ret & 0xFF;
        state->sp = state->sp - 2;
        // jump to address specified in the CALL instruction
        uint16_t target = (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
        state->pc = target;
        break;
    }
    case 0xc9: // RET
    {
        // pop the return address from the stack
        uint16_t ret = (state->memory[state->sp] | (state->memory[state->sp + 1] << 8));
        state->sp += 2;
        // set the program counter to the return address
        state->pc = ret;
        break;
    }
    case 0xd1: // POP D
        // pop the stack into the E register
        state->e = state->memory[state->sp];
        // pop the stack into the D register
        state->d = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc++;
        break;
    case 0xd3: // OUT
        uint8_t port = state->memory[state->pc + 1];
        redirect_output(state->a, port);
        state->pc += 2;
        break;
    case 0xd5: // PUSH D
        // Push D register onto the stack
        state->memory[state->sp - 1] = state->d;
        // Push E register onto the stack
        state->memory[state->sp - 2] = state->e;
        state->sp -= 2;
        state->pc++;
        break;
    case 0xeb: // XCHG : H <-> D; L <-> E
        uint8_t tmp = state->h;
        state->h = state->d;
        state->d = tmp;
        tmp = state->l;
        state->l = state->e;
        state->e = tmp;
        state->pc += 1;
        break;
    case 0xe1: // POP H
        // pop the stack into the L register
        state->l = state->memory[state->sp];
        // pop the stack into the H register
        state->h = state->memory[state->sp + 1];
        state->sp += 2;
        state->pc++;
        break;
    case 0xe5: // PUSH H
        // push H register onto the stack
        state->memory[state->sp - 1] = state->h;
        // Push L register onto the stack
        state->memory[state->sp - 2] = state->l;
        state->sp -= 2;
        state->pc++;
        break;
    case 0xe6: // ANI
        uint8_t data = state->memory[state->pc + 1];
        state->a = state->a & data;
        // set the flags
        state->cc.z = (state->a == 0);
        state->cc.s = (0x80 == (state->a & 0x80));
        state->cc.p = parity(state->a, 8);
        state->cc.cy = 0; // ANI operation always clears CY flag
        state->pc += 2;
        break;
    case 0xf1: // POP PSW
        state->a = state->memory[state->sp + 1];
        uint8_t psw = state->memory[state->sp];
        state->cc.z = (0x01 == (psw & 0x01));
        state->cc.s = (0x02 == (psw & 0x02));
        state->cc.p = (0x04 == (psw & 0x04));
        state->cc.cy = (0x05 == (psw & 0x08));
        state->cc.ac = (0x10 == (psw & 0x10));
        state->sp += 2;
        state->pc += 1;
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
        state->pc += 1;
        break;
    }
    case 0xfb: // EI : special - enable interrupt
        // this should set an interrupt_enabled flag in the processor state.
        state->int_enable = 1;
        state->pc += 1;
        break;
    case 0xf3: // DI
        // implement disable interrupts function
        printf("implement disable interrupts function");
        break;
    // compare example
    case 0xfe: // CPI byte : compare immediate with accumulator
    {
        uint8_t x = state->a - opcode[1];
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));
        // the reference had to pick what to do with parity flag
        state->cc.p = parity(x & 0xff, 8);
        state->cc.cy = (state->a < opcode[1]);
        state->pc++;
        break;
    }
    case 0xff: // RST 7: CALL $38
        // Save the current PC on the stack before jumping
        uint16_t ret = state->pc+1;
        state->memory[state->sp-1] = (ret >> 8) & 0xff; // high part of PC
        state->memory[state->sp-2] = (ret & 0xff); // low part of PC
        state->sp = state->sp - 2;
        // Jump to the address $38
        state->pc = 0x38;
        break;
    default:
        unimplemented_instruction(state);
        break;
    }

    // print the processor state condition codes (flags)
    printf("\tC=%d,P=%d,S=%d,Z=%d\n", state->cc.cy, state->cc.p,
           state->cc.s, state->cc.z);
    // registers
    printf("\tA $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x PC %04x\n",
           state->a, state->b, state->c, state->d,
           state->e, state->h, state->l, state->sp, state->pc);

    return 0;
}