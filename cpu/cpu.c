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

// TODO might have added pagebound ITCs where unnecessary? check *9 and *D
#define NMOS_6502
#include "cpu.h"

#ifdef NES
#define NOBCD // 2A03 does not support BCD
#endif

/* ADDR		TYPE
 * 0000h-07FFh	WRAM (this is where the zero page and stack is)
 * 0800h-0FFFh
 * 1000h-17FFh	Mirrors of WRAM
 * 1800h-1FFFh
 *
 * 2000h-2007h	cpu->PPU registers
 * 2008h-3FFFh	Mirrors of PPU registers (loop 8)
 *
 * 4000h-4017h	cpu->APU/IO registers (see top of cpu.h)
 * 4018h-401Fh	Disabled registers
 *
 * 4020h-FFFFh	cpu->All cartridge memory (ROM, SRAM, etc)
 */

#ifdef NES
char vram[2048];
char oam[256];
char palette[28];
#endif

void j65_init(j65_t* cpu) {
	cpu->PC = 0;
	cpu->ITC = 0;
	cpu->P = 0;
	cpu->S = 255;
	cpu->stack = cpu->memmap+0x100;	// TODO the stack funcs are wrong
}

void (*srqh)(uint8_t a, uint8_t b) = NULL;

#include "internal/common.h"

// TODO does the 65c02 have different cycle times?
// TODO use some damn pointers
// TODO 2 stage pipeline?
// https://retrocomputing.stackexchange.com/questions/5369/how-does-the-6502-implement-its-branch-instructions
// TODO simulate open bus on unmapped addresses
// TODO formulate a not-too-hacky way of simulating mirrored memory
// TODO implement overflow, negative statuses in math ops
// TODO check that chkzero is used where needed
// TODO functions missing zero/negative checks


void op02(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opea(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->PC++;
}

void op04(j65_t* cpu) {
	opea(cpu);
	cpu->ITC++;
	cpu->PC++;
}

void op05(j65_t* cpu) {
	cpu->ITC = 3;
	or(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void op06(j65_t* cpu) {
	cpu->ITC = 6;
	asl(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void op08(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->stack[cpu->S--] = cpu->P;
	cpu->PC++;
}

void op09(j65_t* cpu) {
	cpu->ITC = 2;
	or(cpu, cpu->memmap[cpu->PC+1]);
	cpu->PC+=2;
}

void op0a(j65_t* cpu) {
	cpu->ITC = 2;
	asl(cpu, cpu->A);
	cpu->PC++;
}

void op0c(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC+=2;
}

void op0d(j65_t* cpu) {
	cpu->ITC = 4;
	or(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op0e(j65_t* cpu) {
	cpu->ITC = 6;
	asl(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;

}

void op10(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_NEGATIVE) == 0) branch(cpu);
	else cpu->PC+=2;
}

void op12(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op14(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void op15(j65_t* cpu) {
	cpu->ITC = 4;
	// dear god
	or(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
	cpu->PC+=2;
}

void op18(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P &= MASK_P_CARRY;
	cpu->PC++;
}

void op1a(j65_t* cpu) {
	opea(cpu);
}

void op1c(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void op1e(j65_t* cpu) {
	cpu->ITC = 7;
	asl(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op4c(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->PC = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]);
}

void op20(j65_t* cpu) {
	// TODO i have no idea if this is right
	cpu->stack[cpu->S--] = (uint8_t) cpu->PC+2;
	cpu->stack[cpu->S--] = (cpu->PC+2) >> 8;
	op4c(cpu);
	cpu->ITC = 6;
}

void op22(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op24(j65_t* cpu) {
	cpu->ITC = 3;
	bit(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void op25(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->A &= cpu->memmap[cpu->memmap[cpu->PC+1]];
	chkzero(cpu, cpu->A)
	cpu->PC+=2;
}

void op28(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->P = cpu->stack[cpu->S++];
	cpu->PC++;
}

void op29(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->A &= cpu->memmap[cpu->PC+1];
	chkzero(cpu, cpu->A)
	cpu->PC+=2;
}

void op2a(j65_t* cpu) {

	cpu->ITC = 2;

	uint8_t tmp = cpu->P & SET_P_CARRY;

	chkcarry(cpu, cpu->A);

	cpu->A <<= 1;

	if(tmp != 0) cpu->A |= 1;
	chkzero(cpu, cpu->A)

	cpu->PC++;

}

void op2c(j65_t* cpu) {
	cpu->ITC = 4;
	bit(cpu, cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)]);
	cpu->PC+=3;
}

void op30(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_NEGATIVE) != 0) branch(cpu);
	else cpu->PC+=2;
}

void op32(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op34(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void op35(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->A = cpu->memmap[cpu->memmap[cpu->PC+1]] & cpu->X;
	chkzero(cpu, cpu->A)
	cpu->PC+=2;
}

void op38(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P |= SET_P_CARRY;
	cpu->PC++;
}

void op39(j65_t* cpu) {
	cpu->ITC = 4;
	// TODO right cpu->PC+i for cycle?
	if(new_page(cpu->PC+1, (uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	and(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op3a(j65_t* cpu) {
	opea(cpu);
}

void op3c(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void op3d(j65_t* cpu) {
	cpu->ITC = 4;
	// TODO right cpu->PC+i for cycle?
	if(new_page(cpu->PC+1, (uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	and(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op60(j65_t* cpu) {
	cpu->ITC = 6;
	cpu->PC = (cpu->stack[cpu->S-1] << 8) | cpu->stack[cpu->S-2];
	cpu->S -= 2;
	cpu->PC++;
}

void op40(j65_t* cpu) {
	// TODO this seems *really, really* wrong to me, figure this out
	// TODO check this and op60 to make sure low byte is popped first
	op28(cpu);
	op60(cpu);
	// simulating 2 opcodes has broken program counter, account for this
	cpu->PC--;
}

void op42(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op44(j65_t* cpu) {
	opea(cpu);
	cpu->ITC++;
	cpu->PC++;
}

void op45(j65_t* cpu) {
	cpu->ITC = 3;
	xor(cpu,  cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void op48(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->stack[cpu->S--] = cpu->A;
	cpu->PC++;
}

void op49(j65_t* cpu) {
	cpu->ITC = 2;
	xor(cpu,  cpu->memmap[cpu->PC+1]);
	cpu->PC+=2;
}

void op4a(j65_t* cpu) {
	cpu->ITC = 2;

	if((cpu->A & 0b00000001) != 0) {
		cpu->P |= SET_P_CARRY;
	} else {
		cpu->P &= MASK_P_CARRY;
	}

	cpu->A >>= 1;
	chkzero(cpu, cpu->A)
	cpu->PC++;
}

void op4d(j65_t* cpu) {
	cpu->ITC = 4;
	xor(cpu,  short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]));
	cpu->PC+=3;
}

void op50(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_OVERFLOW) == 0) branch(cpu);
	else cpu->PC+=2;
}

void op52(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op54(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void op55(j65_t* cpu) {
	cpu->ITC = 3;
	xor(cpu,  cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
	cpu->PC+=2;
}

void op58(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P &= MASK_P_INT;
	cpu->PC++;
}

void op59(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, (uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	xor(cpu,  cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op5a(j65_t* cpu) {
	opea(cpu);
}

void op5c(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void op5d(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, (uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	xor(cpu,  cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void op62(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op64(j65_t* cpu) {
	opea(cpu);
	cpu->ITC++;
	cpu->PC++;
}

void op68(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->A = cpu->stack[cpu->S++];
	chkzero(cpu, cpu->A)
	cpu->PC++;
}

void op65(j65_t* cpu) {
	cpu->ITC = 3;
	adc8(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]], &cpu->A);
}

void op69(j65_t* cpu) {
	cpu->ITC = 2;
	adc8(cpu, cpu->memmap[cpu->PC+1], &cpu->A);
}

void op6a(j65_t* cpu) {

	cpu->ITC = 2;

	uint8_t tmp = cpu->P & SET_P_CARRY;

	if((cpu->A & 0b00000001) != 0) {
		cpu->P |= SET_P_CARRY;
	} else {
		cpu->P &= MASK_P_CARRY;
	}

	cpu->A >>= 1;

	if(tmp != 0) cpu->A |= 1 << 7;
	chkzero(cpu, cpu->A)

	cpu->PC++;

}

void op6c(j65_t* cpu) {
	cpu->ITC = 5;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), acp = addr;
	#ifdef NMOS_6502
	// only do this on a 6502
	// 65c02 fixes this
	if(((addr+1) % 0x100) == 0) addr-=0x100;
	#endif
	cpu->PC = short2addr(cpu->memmap[acp], cpu->memmap[addr+1]);
}

void op70(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_OVERFLOW) != 0) branch(cpu);
	else cpu->PC+=2;
}

void op72(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op74(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void op75(j65_t* cpu) {
	cpu->ITC = 3;
	adc8(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]], &cpu->X);
}

void op78(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P |= SET_P_INT;
	cpu->PC++;
}

void op7a(j65_t* cpu) {
	opea(cpu);
}

void op7c(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void op80(j65_t* cpu) {
	opea(cpu);
	cpu->PC++;
}

void op82(j65_t* cpu) {
	opea(cpu);
	cpu->PC++;
}

void op84(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->Y;
	cpu->PC+=2;
}

void op86(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->X;
	cpu->PC+=2;
}

void op87(j65_t* cpu) {
	cpu->ITC = 3;
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->A & cpu->X;
	cpu->PC+=2;
}

void op88(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->Y--;
	chknegative(cpu, cpu->Y);
	cpu->PC++;
}

void op89(j65_t* cpu) {
	opea(cpu);
	cpu->PC++;
}

void op8a(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->A = cpu->X;
	chknegative(cpu, cpu->A);
	chkzero(cpu, cpu->A)
	cpu->PC++;
}

void op8b(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->A = cpu->X & cpu->A & cpu->memmap[cpu->PC+1];
	system_request(cpu, 0x8B, cpu->memmap[cpu->PC+1]);
	cpu->PC+=2;
}

void op8c(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)] = cpu->Y;
	cpu->PC+=3;
}

void op8e(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)] = cpu->X;
	cpu->PC+=3;
}

void op8f(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] = cpu->A & cpu->X;
	cpu->PC+=3;
}

void op90(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_CARRY) == 0) branch(cpu);
	else cpu->PC+=2;
}

void op92(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op94(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]] = cpu->Y;
	cpu->PC+=2;
}

void op95(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]] = cpu->A;
	cpu->PC+=2;
}

void op96(j65_t* cpu) {
	cpu->ITC = 4;
	cpu->memmap[cpu->memmap[zpz(cpu->Y, cpu->memmap[cpu->PC+1])]] = cpu->X;
	cpu->PC+=2;
}

void op98(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->A = cpu->Y;
	chkzero(cpu, cpu->A)
	chknegative(cpu, cpu->A);
	cpu->PC++;
}

void op9a(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->S = cpu->X;
	cpu->PC++;
}

void opa5(j65_t* cpu) {
	cpu->ITC = 3;
	lda(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void opa7(j65_t* cpu) {
	cpu->ITC = 3;
	lax(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
	cpu->PC+=2;
}

void opa8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->Y = cpu->A;
	chknegative(cpu, cpu->Y);
	chkzero(cpu, cpu->Y)
	cpu->PC++;
}

void opa9(j65_t* cpu) {
	cpu->ITC = 2;
	lda(cpu, cpu->memmap[cpu->PC+1]);
	cpu->PC+=2;
}

void opaa(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->X = cpu->A;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
	cpu->PC++;
}

void opab(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->A &= cpu->memmap[cpu->PC+1];
	cpu->X = cpu->A & cpu->memmap[cpu->PC+1];
	system_request(cpu, 0xAB, cpu->memmap[cpu->PC+1]);
	cpu->PC+=2;
}

void opad(j65_t* cpu) {
	cpu->ITC = 4;
	lda(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void opaf(j65_t* cpu) {
	cpu->A = cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)];
	cpu->X = cpu->A;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
	cpu->ITC = 4;
	cpu->PC+=3;
}

void opb0(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_CARRY) != 0) branch(cpu);
	else cpu->PC+=2;
}

void opb2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opb4(j65_t* cpu) {
	cpu->ITC = 4;
	ldy(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
	cpu->PC+=2;
}

void opb5(j65_t* cpu) {
	cpu->ITC = 4;
	lda(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
	cpu->PC+=2;
}

void opb6(j65_t* cpu) {
	cpu->ITC = 4;
	ldx(cpu, cpu->memmap[cpu->memmap[zpz(cpu->Y, cpu->memmap[cpu->PC+1])]]);
	cpu->PC+=2;
}

void opb8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P &= MASK_P_OVERFLOW;
	cpu->PC++;
}

void opb9(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	cpu->A = cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
	chknegative(cpu, cpu->A);
	chkzero(cpu, cpu->A)
	cpu->PC+=3;
}

void opba(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->X = cpu->S;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
	cpu->PC++;
}

void opbc(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	cpu->Y = cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
	chknegative(cpu, cpu->Y);
	chkzero(cpu, cpu->Y)
	cpu->PC+=3;
}

void opbd(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	lda(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void opbe(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	ldx(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void opbf(j65_t* cpu) {
	cpu->ITC = 4;
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	lda(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	ldx(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	cpu->PC+=3;
}

void opc2(j65_t* cpu) {
	opea(cpu);
	cpu->PC++;
}

void opc6(j65_t* cpu) {
	cpu->ITC = 5;
	uint8_t tmp = cpu->memmap[cpu->memmap[cpu->PC+1]];
	tmp--;
	cpu->memmap[cpu->memmap[cpu->PC+1]]--;
	chknegative(cpu, tmp);
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	cpu->PC+=2;
}

void opc8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->Y++;
	chknegative(cpu, cpu->Y);
	cpu->PC++;
}

void opca(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->X--;
	chknegative(cpu, cpu->X);
	cpu->PC++;
}

void opce(j65_t* cpu) {
	cpu->ITC = 6;
	uint8_t tmp = cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)];
	tmp--;
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)]--;
	chknegative(cpu, tmp);
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	cpu->PC+=3;
}

void opd2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opd4(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void opd8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P &= MASK_P_DECIMAL;
	cpu->PC++;
}

void opd0(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_ZERO) == 0) branch(cpu);
	else cpu->PC+=2;
}

void opda(j65_t* cpu) {
	opea(cpu);
}

void opdc(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void opde(j65_t* cpu) {
	cpu->ITC = 7;
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X]--;
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X];
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	chknegative(cpu, tmp);
	cpu->PC+=3;
}

void ope2(j65_t* cpu) {
	opea(cpu);
	cpu->PC++;
}

void ope6(j65_t* cpu) {
	cpu->ITC = 5;
	uint8_t tmp = cpu->memmap[cpu->PC+1];
	tmp++;
	cpu->memmap[cpu->PC+1]++;
	chknegative(cpu, tmp);
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	cpu->PC+=2;
}

void ope8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->X++;
	chknegative(cpu, cpu->X);
	cpu->PC++;
}

void opee(j65_t* cpu) {
	cpu->ITC = 6;
	// TODO 90% sure something here is wrong
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] + 1;
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] = tmp;
	chknegative(cpu, tmp);
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	cpu->PC+=3;
}

void opf0(j65_t* cpu) {
	cpu->ITC = 2;
	if((cpu->P & SET_P_ZERO) != 0) branch(cpu);
	else cpu->PC+=2;
}

void opf2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opf4(j65_t* cpu) {
	opea(cpu);
	cpu->ITC+=2;
	cpu->PC++;
}

void opf8(j65_t* cpu) {
	cpu->ITC = 2;
	cpu->P |= SET_P_DECIMAL;
	cpu->PC++;
}

void opfa(j65_t* cpu) {
	opea(cpu);
}

void opfc(j65_t* cpu) {
	cpu->ITC = 4;
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cpu->PC+=3;
}

void opfe(j65_t* cpu) {
	cpu->ITC = 7;
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X]++;
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X];
	if(tmp == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}
	chknegative(cpu, tmp);
	cpu->PC+=3;
}

#ifdef NMOS_6502
#undef WDC_65C02
// opcode map for tick() to use
function_pointer_array opcodes[] = {
//  0  1      2      3      4      5      6      7      8      9      a      b      c      d      e      f
NULL,  NULL,  &op02, NULL,  &op04, &op05, &op06, NULL,  &op08, &op09, &op0a, NULL,  &op0c, &op0d, &op0e, NULL, // 0
&op10, NULL,  &op12, NULL,  &op14, &op15, NULL,  NULL,  &op18, NULL,  NULL,  NULL,  &op1c, NULL,  &op1e, NULL, // 1
&op20, NULL,  &op22, NULL,  &op24, &op25, NULL,  NULL,  &op28, &op29, &op2a, NULL,  &op2c, NULL,  NULL,  NULL, // 2
&op30, NULL,  &op32, NULL,  &op34, &op35, NULL,  NULL,  &op38, &op39, NULL,  NULL,  &op3c, &op3d, NULL,  NULL, // 3
&op40, NULL,  &op42, NULL,  &op44, &op45, NULL,  NULL,  &op48, &op49, &op4a, NULL,  &op4c, &op4d, NULL,  NULL, // 4
&op50, NULL,  &op52, NULL,  &op54, &op55, NULL,  NULL,  &op58, &op59, NULL,  NULL,  &op5c, &op5d, NULL,  NULL, // 5
&op60, NULL,  &op62, NULL,  &op64, &op65, NULL,  NULL,  &op68, &op69, &op6a, NULL,  &op6c, NULL,  NULL,  NULL, // 6
&op70, NULL,  &op72, NULL,  &op74, &op75, NULL,  NULL,  &op78, NULL,  NULL,  NULL,  &op7c, NULL,  NULL,  NULL, // 7
&op80, NULL,  &op82, NULL,  &op84, NULL,  &op86, &op87, &op88, &op89, &op8a, &op8b, &op8c, NULL,  &op8e, &op8f,// 8
&op90, NULL,  &op92, NULL,  &op94, &op95, &op96, NULL,  &op98, NULL,  &op9a, NULL,  NULL,  NULL,  NULL,  NULL, // 9
NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  &opa5, &opa7, &opa8, &opa9, &opaa, &opab, NULL,  &opad, NULL,  &opaf,// a
&opb0, NULL,  &opb2, NULL,  &opb4, NULL,  &opb5, &opb6, &opb8, &opb9, &opba, NULL,  &opbc, &opbd, &opbe, &opbf,// b
NULL,  NULL,  &opc2, NULL,  NULL,  NULL,  &opc6, NULL,  &opc8, NULL,  &opca, NULL,  NULL,  NULL,  &opce, NULL, // c
&opd0, NULL,  &opd2, NULL,  &opd4, NULL,  NULL,  NULL,  &opd8, NULL,  NULL,  NULL,  &opdc, NULL,  &opde, NULL, // d
NULL,  NULL,  &ope2, NULL,  NULL,  NULL,  &ope6, NULL,  &ope8, NULL,  &opea, NULL,  NULL,  NULL,  &opee, NULL, // e
&opf0, NULL,  &opf2, NULL,  &opf4, NULL,  NULL,  NULL,  &opf8, NULL,  NULL,  NULL,  &opfc, NULL,  &opfe, NULL  // f
};
#endif

#ifdef WDC_65C02
#endif

void tick(j65_t* cpu) {
	if(cpu->ITC == 1) {
		// TODO run code cycle end
		// possibly have to rework cpu->ITC as a table?
		// + extensible
		// + condenses code a lot
		// - page bounds?
		// - takes a long time
		// - harder to read maybe?
		//
		// huh, turns out fake6502 also uses an cpu->ITC table
		uint8_t copc = cpu->memmap[cpu->PC];
		if(opcodes[copc] != NULL) {
			#ifndef NOLIBC
			printf("Opcode 0x%X at 0x%X\n", copc, cpu->PC);
			#endif
			opcodes[copc](cpu);
		} else {
			#ifndef NOLIBC
			printf("NULL Ocpu->PCODE: 0x%X. It will be replaced by NOP. This *will* break the program counter!\ncpu->PC: 0x%X\n", (uint8_t) cpu->memmap[cpu->PC+1], cpu->PC);
			#endif
			opea(cpu);
		}
	}
	cpu->ITC--;
}
