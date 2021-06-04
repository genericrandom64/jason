/* unnamed 6502 emulator
 *
 * by genericrandom64
 * Public-domain software
 */

#include "cpu.h"
// MEMORY

char	vram[2048],	// 2kb of video ram
	oam[256],	// sprite data
	palette[28],	// on the ppu chip
	memmap[0xFFFF],	// system memory
*stack = memmap+0x1FF;	// convenience pointer for the stack on page 1

/* ADDR		TYPE
 * 0000h-07FFh	WRAM (this is where the zero page and stack is)
 * 0800h-0FFFh
 * 1000h-17FFh	Mirrors of WRAM
 * 1800h-1FFFh
 *
 * 2000h-2007h	PPU registers
 * 2008h-3FFFh	Mirrors of PPU registers (loop 8)
 *
 * 4000h-4017h	APU/IO registers (see top of cpu.h)
 * 4018h-401Fh	Disabled registers
 *
 * 4020h-FFFFh	All cartridge memory (ROM, SRAM, etc)
 */

// see below comment
uint8_t pregupdmap[1024], wramupdmap[4];

// TODO instructions should automatically do this if the absolute address is within one of these regions. in fact, this doesn't even work because it clears writes inside the mirrors
void copymirrors() {
	// Copy WRAM
	for(uint8_t i = 1; i < 4; i++) memcpy(memmap, (memmap+(0x800*i)), 0x800);
	// Copy PPU registers
	// TODO im not writing a bad impl
}

// CPU REGISTERS

uint16_t PC = 0;	// program counter
uint8_t	S = 0,		// stack pointer
	A = 0,		// accumulator
	X = 0,		// X register
	Y = 0,		// Y register
	P = 0,		// status register
	ITC;		// instruction timer to waste cycles until the instruction is supposed to be done

// TODO do instructions explicitly unset status flags?
// TODO implement overflow, negative, zero statuses in math ops

uint8_t bcd2int(uint8_t in) {
	uint8_t l = (in >> 4)*10, r = in & 0b00001111;
	return l + r;
}

uint8_t int2bcd(uint8_t in) {
	uint8_t l = ((in / 10) % 10) << 4, r = in % 10;
	return l | r;
}

// TODO write adc16 for 16 bit addresses
void adc8(uint8_t src, uint8_t *addreg) {
	// TODO sign this and set overflow accordingly
	uint16_t res = 0;
	int cmpval = 0;
	int8_t res8 = 0;
	if(P & SET_P_DECIMAL != 0) {
		res = *addreg + memmap[PC+1];
		cmpval = *addreg + memmap[PC+1];
		res8 = res & 0b0000000011111111;
		if(res & 0b0000000100000000 != 0) {
			P |= SET_P_CARRY;
		} else {
			P &= MASK_P_CARRY;
		}
	} else {
		// TODO does bcd have sign?
		int8_t AB = bcd2int(*addreg), CB = bcd2int(memmap[PC+1]);
		res = AB + CB;
		cmpval = AB + CB;
		res8 = res & 0b0000000011111111;
		// TODO set carry correctly
		if(res & 0b0000000100000000 != 0) {
			P |= SET_P_CARRY;
		} else {
			P &= MASK_P_CARRY;
		}
		res8 = int2bcd(res8);
	}
	// TODO is the result stored to A?
	A = res8;
	// TODO make sure cmp works
	if(cmpval != res8) P |= SET_P_OVERFLOW;
	PC+=2;
}

void op08() {
	ITC = 3;
	stack[S--] = P;
	PC++;
}

void op0a() {
	ITC = 2;

	if(A & 0b10000000 != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;
	PC++;
}

void op18() {
	ITC = 2;
	P &= MASK_P_CARRY;
	PC++;
}

void op25() {
	ITC = 3;
	A &= memmap[memmap[PC+1]];
	PC+=2;
}

void op28() {
	ITC = 4;
	P = stack[S++];
	PC++;
}

void op29() {
	ITC = 2;
	A &= memmap[PC+1];
	PC+=2;
}

void op2a() {

	ITC = 2;

	uint8_t tmp = P & SET_P_CARRY;

	if(A & 0b10000000 != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;

	if(tmp != 0) A |= 1;

	PC++;

}

void op35() {
	ITC = 4;
	A = memmap[memmap[PC+1]] & X;
	PC+=2;
}

void op38() {
	ITC = 2;
	P |= SET_P_CARRY;
	PC++;
}

void op60() {
	ITC = 6;
	PC = (stack[S--] << 8) | stack[S--];
	PC++;
}

void op40() {
	// TODO this seems *really, really* wrong to me, figure this out
	// TODO check this and op60 to make sure low byte is popped first
	op28();
	op60();
}

void op48() {
	ITC = 3;
	stack[S--] = A;
	PC++;
}

void op4a() {
	ITC = 2;

	if(A & 0b00000001 != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A >>= 1;
	PC++;
}

void op58() {
	ITC = 2;
	P &= MASK_P_INT;
	PC++;
}

void op68() {
	ITC = 4;
	A = stack[S++];
	PC++;
}

void op65() {
	ITC = 3;
	adc8(memmap[memmap[PC+1]], &A);
}

void op69() {
	ITC = 2;
	adc8(memmap[PC+1], &A);
}

void op6a() {

	ITC = 2;

	uint8_t tmp = P & SET_P_CARRY;

	if(A & 0b00000001 != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A >>= 1;

	if(tmp != 0) A |= 1 << 7;

	PC++;

}

void op75() {
	ITC = 3;
	adc8(memmap[memmap[PC+1]], &X);
}

void op78() {
	ITC = 2;
	P |= SET_P_INT;
	PC++;
}

void op88() {
	ITC = 2;
	Y--;
	PC++;
}

void op8a() {
	ITC = 2;
	A = X;
	PC++;
}

void op98() {
	ITC = 2;
	A = Y;
	PC++;
}

void op9a() {
	ITC = 2;
	S = X;
	PC++;
}

void opa8() {
	ITC = 2;
	Y = A;
	PC++;
}

void opaa() {
	ITC = 2;
	X = A;
	PC++;
}

void opb8() {
	ITC = 2;
	P &= MASK_P_OVERFLOW;
	PC++;
}

void opba() {
	ITC = 2;
	X = S;
	PC++;
}

void opc8() {
	ITC = 2;
	Y++;
	PC++;
}

void opca() {
	ITC = 2;
	X--;
	PC++;
}

void opd8() {
	ITC = 2;
	P &= MASK_P_DECIMAL;
	PC++;
}

void ope8() {
	ITC = 2;
	X++;
	PC++;
}

void opea() {
	ITC = 2;
	PC++;
}

void opf8() {
	ITC = 2;
	P |= SET_P_DECIMAL;
	PC++;
}
