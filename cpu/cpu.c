/* J65: 6502-based chip emulator
 *
 * Supported chips:
 *
 * WIP chips:
 * 	MOS 6502	(KIM-1, PET, VIC-20 emulation)
 * 	+	This is the current focus.
 *
 * 	WDC 65C02	(Modern 65c02 system emulation)
 * 	WDC 65sc02
 * 	+	This will be implemented with an opcode map. It's
 *		secondary but should be done when the 6502 is done.
 *
 * 	Ricoh 2A03	(Is the code still heavily based on NES memory mappings?)
 *
 * Planned support:
 * 	WDC 65C816	(SNES emulation)
 *
 * by genericrandom64
 * Public-domain software
 */

// TODO remove UOP in favor of WDC_65C02 opcode map
#define NMOS_6502
#include "cpu.h"

#ifdef NES
// 2A03 does not support BCD
#define NOBCD
#endif

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

// TODO make more stuff into their own functions to cut down on code size
// TODO functions missing zero/negative checks

uint16_t PC;
uint8_t	S, A, X, Y, P, ITC;
char
	#ifdef NES
	vram[2048],	// 2kb of video ram
	oam[256],	// sprite data
	palette[28],	// on the ppu chip
	#endif
	memmap[0xFFFF],	// system memory
	// TODO the stack funcs are probably wrong, or this is; i have low confidence in
	// the accuracy of the stack code considering the length of time it was written in
	*stack = memmap+0x1FF;

void (*srqh)(uint8_t a, uint8_t b) = NULL;

#include "internal/common.h"

// TODO page wrap might be wrong?
// http://6502.org/tutorials/65c816opcodes.html#APPENDIX:
// 5.1.1 seems to indicate problems in pages.

// TODO does the 65c02 have different cycle times?
// TODO use some damn pointers

// TODO 2 stage pipeline?
// https://retrocomputing.stackexchange.com/questions/5369/how-does-the-6502-implement-its-branch-instructions

// see below comment
//uint8_t pregupdmap[1024], wramupdmap[4];

/*
// TODO instructions should automatically do this if the absolute address is within one of these regions. in fact, this doesn't even work because it clears writes inside the mirrors
void copymirrors() {
	// Copy WRAM
	for(uint8_t i = 1; i < 4; i++) memcpy(memmap, (memmap+(0x800*i)), 0x800);
	// Copy PPU registers
}
*/

// TODO simulate open bus on unmapped addresses

// TODO implement overflow, negative statuses in math ops
// TODO check that chkzero() isnt used where not needed, if not check A = 0 at step emulator instead of opcode emulator
// TODO do zpx/zpy take x/y signed or unsigned?


void op02() {
	system_request(memmap[PC], memmap[PC+1]);
}

void opea() {
	ITC = 2;
	PC++;
}

void op04() {
	opea();
	ITC++;
	PC++;
}

void op05() {
	ITC = 3;
	or(memmap[memmap[PC+1]]);
	PC+=2;
}

void op06() {
	ITC = 6;

	A = memmap[memmap[PC+1]];

	if((A & 0b10000000) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;
	chkzero()
	PC+=2;
}

void op08() {
	ITC = 3;
	stack[S--] = P;
	PC++;
}

void op09() {
	ITC = 2;
	or(memmap[PC+1]);
	PC+=2;
}

void op0a() {
	ITC = 2;

	if((A & 0b10000000) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;
	chkzero()
	PC++;
}

void op0c() {
	opea();
	ITC+=2;
	PC+=2;
}

void op0d() {
	ITC = 4;
	or(memmap[short2addr(memmap[PC+1], memmap[PC+2])]);
	PC+=3;
}

void op0e() {
	ITC = 6;

	A = memmap[short2addr(PC+1, PC+2)];

	if((A & 0b10000000) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;
	chkzero()
	PC+=3;

}

void op10() {
	ITC = 2;
	if((P & SET_P_NEGATIVE) == 0) branch();
	else PC+=2;
}

void op12() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op14() {
	opea();
	ITC+=2;
	PC++;
}

void op15() {
	ITC = 4;
	// dear god
	or(memmap[memmap[zpz(X, memmap[PC+1])]]);
	PC+=2;
}

void op18() {
	ITC = 2;
	P &= MASK_P_CARRY;
	PC++;
}

void op1a() {
	opea();
}

void op1c() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
}

void op4c() {
	ITC = 3;
	PC = short2addr(memmap[PC+1], memmap[PC+2]);
}

void op20() {
	// TODO i have no idea if this is right
	stack[S--] = (uint8_t) PC+2;
	stack[S--] = PC+2 >> 8;
	op4c();
	ITC = 6;
}

void op22() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op24() {
	ITC = 3;
	bit(memmap[memmap[PC+1]]);
	PC+=2;
}

void op25() {
	ITC = 3;
	A &= memmap[memmap[PC+1]];
	chkzero()
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
	chkzero()
	PC+=2;
}

void op2a() {

	ITC = 2;

	uint8_t tmp = P & SET_P_CARRY;

	if((A & 0b10000000) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A <<= 1;

	if(tmp != 0) A |= 1;
	chkzero()

	PC++;

}

void op2c() {
	ITC = 4;
	bit(memmap[short2addr(PC+1, PC+2)]);
	PC+=3;
}

void op30() {
	ITC = 2;
	if((P & SET_P_NEGATIVE) != 0) branch();
	else PC+=2;
}

void op32() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op34() {
	opea();
	ITC+=2;
	PC++;
}

void op35() {
	ITC = 4;
	A = memmap[memmap[PC+1]] & X;
	chkzero()
	PC+=2;
}

void op38() {
	ITC = 2;
	P |= SET_P_CARRY;
	PC++;
}

void op3a() {
	opea();
}

void op3c() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
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
	// simulating 2 opcodes has broken program counter, account for this
	PC--;
}

void op42() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op44() {
	opea();
	ITC++;
	PC++;
}

void op45() {
	ITC = 3;
	xor(memmap[memmap[PC+1]]);
	PC+=2;
}

void op48() {
	ITC = 3;
	stack[S--] = A;
	PC++;
}

void op49() {
	ITC = 2;
	xor(memmap[PC+1]);
	PC+=2;
}

void op4a() {
	ITC = 2;

	if((A & 0b00000001) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A >>= 1;
	chkzero()
	PC++;
}

void op4d() {
	ITC = 4;
	xor(short2addr(memmap[PC+1], memmap[PC+2]));
	PC+=3;
}

void op50() {
	ITC = 2;
	if((P & SET_P_OVERFLOW) == 0) branch();
	else PC+=2;
}

void op52() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op54() {
	opea();
	ITC+=2;
	PC++;
}

void op55() {
	ITC = 3;
	xor(memmap[memmap[zpz(X, memmap[PC+1])]]);
	PC+=2;
}

void op58() {
	ITC = 2;
	P &= MASK_P_INT;
	PC++;
}

void op5a() {
	opea();
}

void op5c() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
}

void op62() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op64() {
	opea();
	ITC++;
	PC++;
}

void op68() {
	ITC = 4;
	A = stack[S++];
	chkzero()
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

	if((A & 0b00000001) != 0) {
		P |= SET_P_CARRY;
	} else {
		P &= MASK_P_CARRY;
	}

	A >>= 1;

	if(tmp != 0) A |= 1 << 7;
	chkzero()

	PC++;

}

void op6c() {
	ITC = 5;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), acp = addr;
	#ifdef NMOS_6502
	// only do this on a 6502
	// 65c02 fixes this
	if(((addr+1) % 0x100) == 0) addr-=0x100;
	#endif
	PC = short2addr(memmap[acp], memmap[addr+1]);
}

void op70() {
	ITC = 2;
	if((P & SET_P_OVERFLOW) != 0) branch();
	else PC+=2;
}

void op72() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op74() {
	opea();
	ITC+=2;
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

void op7a() {
	opea();
}

void op7c() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
}

void op80() {
	opea();
	PC++;
}

void op82() {
	opea();
	PC++;
}

void op84() {
	ITC = 3;
	memmap[memmap[PC+1]] = Y;
	PC+=2;
}

void op86() {
	ITC = 3;
	memmap[memmap[PC+1]] = X;
	PC+=2;
}

void op87() {
	ITC = 3;
	memmap[memmap[PC+1]] = A & X;
	PC+=2;
}

void op88() {
	ITC = 2;
	Y--;
	if((Y & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	PC++;
}

void op89() {
	opea();
	PC++;
}

void op8a() {
	ITC = 2;
	A = X;
	if((A & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzero()
	PC++;
}

void op8b() {
	ITC = 2;
	A = X & A & memmap[PC+1];
	system_request(0x8B, memmap[PC+1]);
	PC+=2;
}

void op8c() {
	ITC = 4;
	memmap[short2addr(PC+1, PC+2)] = Y;
	PC+=3;
}

void op8e() {
	ITC = 4;
	memmap[short2addr(PC+1, PC+2)] = X;
	PC+=3;
}

void op8f() {
	ITC = 4;
	memmap[short2addr(memmap[PC+1], memmap[PC+2])] = A & X;
	PC+=3;
}

void op90() {
	ITC = 2;
	if((P & SET_P_CARRY) == 0) branch();
	else PC+=2;
}

void op92() {
	system_request(memmap[PC], memmap[PC+1]);
}

void op94() {
	ITC = 4;
	memmap[memmap[zpz(X, memmap[PC+1])]] = Y;
	PC+=2;
}

void op95() {
	ITC = 4;
	memmap[memmap[zpz(X, memmap[PC+1])]] = A;
	PC+=2;
}

void op96() {
	ITC = 4;
	memmap[memmap[zpz(Y, memmap[PC+1])]] = X;
	PC+=2;
}

void op98() {
	ITC = 2;
	A = Y;
	chkzero()
	if((A & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	PC++;
}

void op9a() {
	ITC = 2;
	S = X;
	PC++;
}

void opa5() {
	ITC = 3;
	lda(memmap[memmap[PC+1]]);
	PC+=2;
}

void opa7() {
	ITC = 3;
	lax(memmap[memmap[PC+1]]);
	PC+=2;
}

void opa8() {
	ITC = 2;
	Y = A;
	if((Y & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzeroy()
	PC++;
}

void opa9() {
	ITC = 2;
	lda(memmap[PC+1]);
	PC+=2;
}

void opaa() {
	ITC = 2;
	X = A;
	if((X & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzerox()
	PC++;
}

void opab() {
	ITC = 2;
	A &= memmap[PC+1];
	X = A & memmap[PC+1];
	system_request(0xAB, memmap[PC+1]);
	PC+=2;
}

void opad() {
	ITC = 4;
	lda(memmap[short2addr(memmap[PC+1], memmap[PC+2])]);
	PC+=3;
}

void opaf() {
	A = memmap[short2addr(PC+1, PC+2)];
	X = A;
	if((X & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzerox()
	ITC = 4;
	PC+=3;
}

void opb0() {
	ITC = 2;
	if((P & SET_P_CARRY) != 0) branch();
	else PC+=2;
}

void opb2() {
	system_request(memmap[PC], memmap[PC+1]);
}

void opb4() {
	ITC = 4;
	ldy(memmap[memmap[zpz(X, memmap[PC+1])]]);
	PC+=2;
}

void opb5() {
	ITC = 4;
	lda(memmap[memmap[zpz(X, memmap[PC+1])]]);
	PC+=2;
}

void opb6() {
	ITC = 4;
	ldx(memmap[memmap[zpz(Y, memmap[PC+1])]]);
	PC+=2;
}

void opb8() {
	ITC = 2;
	P &= MASK_P_OVERFLOW;
	PC++;
}

void opba() {
	ITC = 2;
	X = S;
	if((X & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzerox()
	PC++;
}

void opc2() {
	opea();
	PC++;
}

void opc6() {
	ITC = 5;
	uint8_t tmp = memmap[PC+1];
	tmp--;
	memmap[PC+1]--;
	if((tmp & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	if(tmp == 0) {P |= SET_P_ZERO;} else {P &= MASK_P_ZERO;}
	PC+=2;
}

void opc8() {
	ITC = 2;
	Y++;
	if((Y & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	PC++;
}

void opca() {
	ITC = 2;
	X--;
	if((X & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	PC++;
}

void opce() {
	ITC = 6;
	uint8_t tmp = memmap[short2addr(PC+1, PC+2)];
	tmp--;
	memmap[short2addr(PC+1, PC+2)]--;
	if((tmp & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	if(tmp == 0) {P |= SET_P_ZERO;} else {P &= MASK_P_ZERO;}
	PC+=3;
}

void opd2() {
	system_request(memmap[PC], memmap[PC+1]);
}

void opd4() {
	opea();
	ITC+=2;
	PC++;
}

void opd8() {
	ITC = 2;
	P &= MASK_P_DECIMAL;
	PC++;
}

void opd0() {
	ITC = 2;
	if((P & SET_P_ZERO) == 0) branch();
	else PC+=2;
}

void opda() {
	opea();
}

void opdc() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
}

void ope2() {
	opea();
	PC++;
}

void ope6() {
	ITC = 5;
	uint8_t tmp = memmap[PC+1];
	tmp++;
	memmap[PC+1]++;
	if((tmp & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	if(tmp == 0) {P |= SET_P_ZERO;} else {P &= MASK_P_ZERO;}
	PC+=2;
}

void ope8() {
	ITC = 2;
	X++;
	if((X & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}

	PC++;
}

void opee() {
	ITC = 6;
	uint8_t tmp = memmap[short2addr(PC+1, PC+2)] + 1;
	memmap[short2addr(PC+1, PC+2)] = tmp;
	if((tmp & 0b10000000) != 0) {P |= SET_P_NEGATIVE;} else {P &= MASK_P_NEGATIVE;}
	if(tmp == 0) {P |= SET_P_ZERO;} else {P &= MASK_P_ZERO;}
	PC+=3;
}

void opf0() {
	ITC = 2;
	if((P & SET_P_ZERO) != 0) branch();
	else PC+=2;
}

void opf2() {
	system_request(memmap[PC], memmap[PC+1]);
}

void opf4() {
	opea();
	ITC+=2;
	PC++;
}

void opf8() {
	ITC = 2;
	P |= SET_P_DECIMAL;
	PC++;
}

void opfa() {
	opea();
}

void opfc() {
	ITC = 4;
	uint16_t addr = short2addr(memmap[PC+1], memmap[PC+2]), addrx = addr+X;
	if(new_page(addr, addrx) == 1) ITC++;
	PC+=3;
}

#ifdef NMOS_6502
#undef WDC_65C02
// opcode map for tick() to use
function_pointer_array opcodes[] = {
//  0  1      2      3      4      5      6      7      8      9      a      b      c      d      e      f
NULL,  NULL,  &op02, NULL,  &op04, &op05, &op06, NULL,  &op08, &op09, &op0a, NULL,  &op0c, &op0d, &op0e, NULL, // 0
&op10, NULL,  &op12, NULL,  &op14, &op15, NULL,  NULL,  &op18, NULL,  NULL,  NULL,  &op1c, NULL,  NULL,  NULL, // 1
&op20, NULL,  &op22, NULL,  &op24, &op25, NULL,  NULL,  &op28, &op29, &op2a, NULL,  &op2c, NULL,  NULL,  NULL, // 2
&op30, NULL,  &op32, NULL,  &op34, &op35, NULL,  NULL,  &op38, NULL,  NULL,  NULL,  &op3c, NULL,  NULL,  NULL, // 3
&op40, NULL,  &op42, NULL,  &op44, &op45, NULL,  NULL,  &op48, &op49, &op4a, NULL,  &op4c, &op4d, NULL,  NULL, // 4
&op50, NULL,  &op52, NULL,  &op54, &op55, NULL,  NULL,  &op58, NULL,  NULL,  NULL,  &op5c, NULL,  NULL,  NULL, // 5
&op60, NULL,  &op62, NULL,  &op64, &op65, NULL,  NULL,  &op68, &op69, &op6a, NULL,  &op6c, NULL,  NULL,  NULL, // 6
&op70, NULL,  &op72, NULL,  &op74, &op75, NULL,  NULL,  &op78, NULL,  NULL,  NULL,  &op7c, NULL,  NULL,  NULL, // 7
&op80, NULL,  &op82, NULL,  &op84, NULL,  &op86, &op87, &op88, &op89, &op8a, &op8b, &op8c, NULL,  &op8e, &op8f,// 8
&op90, NULL,  &op92, NULL,  &op94, &op95, &op96, NULL,  &op98, NULL,  &op9a, NULL,  NULL,  NULL,  NULL,  NULL, // 9
NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  &opa5, &opa7, &opa8, &opa9, &opaa, &opab, NULL,  &opad, NULL,  &opaf,// a
&opb0, NULL,  &opb2, NULL,  &opb4, NULL,  &opb5, &opb6, &opb8, NULL,  &opba, NULL,  NULL,  NULL,  NULL,  NULL, // b
NULL,  NULL,  &opc2, NULL,  NULL,  NULL,  &opc6, NULL,  &opc8, NULL,  &opca, NULL,  NULL,  NULL,  &opce, NULL, // c
&opd0, NULL,  &opd2, NULL,  &opd4, NULL,  NULL,  NULL,  &opd8, NULL,  NULL,  NULL,  &opdc, NULL,  NULL,  NULL, // d
NULL,  NULL,  &ope2, NULL,  NULL,  NULL,  &ope6, NULL,  &ope8, NULL,  &opea, NULL,  NULL,  NULL,  &opee, NULL, // e
&opf0, NULL,  &opf2, NULL,  &opf4, NULL,  NULL,  NULL,  &opf8, NULL,  NULL,  NULL,  &opfc, NULL,  NULL,  NULL  // f
};
#endif

#ifdef WDC_65C02
#endif

void tick() {
	if(ITC == 1) {
		// TODO run code cycle end
		// possibly have to rework ITC as a table?
		// + extensible
		// + condenses code a lot
		// - page bounds?
		// - takes a long time
		// - harder to read maybe?
		//
		// huh, turns out fake6502 also uses an ITC table
		uint8_t copc = memmap[PC];
		if(opcodes[copc] != NULL) {
			#ifndef NOLIBC
			printf("Opcode 0x%X at 0x%X\n", copc, PC);
			#endif
			opcodes[copc]();
		} else {
			#ifndef NOLIBC
			printf("NULL OPCODE: 0x%X. It will be replaced by NOP. This *will* break the program counter!\nPC: 0x%X\n", (uint8_t) memmap[PC+1], PC);
			#endif
			opea();
		}
	}
	ITC--;
}
