/* j65: 6502 chip emulator
 *
 * Supported chips:
 * 	NMOS_6502	(KIM-1, PET, VIC-20 emulation)
 *	(TODO does not support ROR bug)
 * 	+	This is the current focus and is supported except for
 * 		interrupts and BCD. Bugfixes for it will be accepted.
 *
 * WIP chips:
 *
 *	NMOS_6507	(Atari VCS)
 *	(TODO address width hack)
 * 	
 * 	WDC_65C02	(Modern 65c02 system emulation)
 *
 * 	WDC_65SC02
 * 	+	This will be implemented with an opcode map. It's
 *		secondary but should be done when the 6502 is done.
 *
 * 	RICOH_2A03	(Is the code still heavily based on NES memory mappings?)
 *
 * Planned support:
 * 	WDC_65C816	(SNES emulation)
 *
 * by genericrandom64
 * Public-domain software
 */

// TODO might have added pagebound ITCs where unnecessary? check *9 and *D
#define NMOS_6502
#include "cpu.h"

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

void j65_init(j65_t* cpu) {
	cpu->PC = 0; // TODO set correct entry point with an indirection
	cpu->ITC = 6;
	cpu->P = 0;
	cpu->S = 255;
	cpu->I = 0;
	// TODO the stack funcs are wrong?
}

void j65_set_stack(j65_t* cpu) {
	cpu->stack = (uint8_t *)cpu->memmap+0x100;
}

void (*srqh)(uint8_t a, uint8_t b) = NULL;

#include "internal/common.h"

// TODO verify nolibc mode
// TODO interrupts
// TODO create document detailing every 6502 opcode
// TODO review all abs,Z functions with pagebounds to check for errors in page handling
// TODO does the 65c02 have different cycle times?
// TODO 2 stage pipeline?
// https://retrocomputing.stackexchange.com/questions/5369/how-does-the-6502-implement-its-branch-instructions
// TODO simulate open bus on unmapped addresses
// TODO formulate a not-too-hacky way of simulating mirrored memory
// TODO check that all ops handle P correctly

void op00(j65_t* cpu) {
	return; // TODO test this
	cpu->P |= SET_P_B1; // j65 cannot use this but things built with it might be able to
	cpu->I |= 4; // software interrupt (IRQ)
	interrupt(cpu, 0xFFFE);
}

void op01(j65_t* cpu) {
	or(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void op02(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opea(j65_t* cpu) {
}

void op04(j65_t* cpu) {
}

void op05(j65_t* cpu) {
	or(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op06(j65_t* cpu) {
	asl(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op08(j65_t* cpu) {
	cpu->stack[cpu->S--] = cpu->P;
}

void op09(j65_t* cpu) {
	or(cpu, cpu->memmap[cpu->PC+1]);
}

void op0a(j65_t* cpu) {
	asl(cpu, cpu->A);
}

void op0c(j65_t* cpu) {
}

void op0d(j65_t* cpu) {
	or(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op0e(j65_t* cpu) {
	asl(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);

}

void op10(j65_t* cpu) {
	if((cpu->P & SET_P_NEGATIVE) == 0) branch(cpu);
}

void op11(j65_t* cpu) {
	or(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void op12(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op14(j65_t* cpu) {
}

void op15(j65_t* cpu) {
	// dear god
	or(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
}

void op16(j65_t* cpu) {
	asl(cpu, zpz(cpu->X, cpu->memmap[cpu->memmap[cpu->PC+1]]));
}

void op18(j65_t* cpu) {
	cpu->P &= MASK_P_CARRY;
}

void op19(j65_t* cpu) {
	or(cpu, short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]) + (uint16_t)cpu->Y);
}

void op1a(j65_t* cpu) {
}

void op1c(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void op1d(j65_t* cpu) {
	or(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op1e(j65_t* cpu) {
	asl(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op4c(j65_t* cpu) {
	cpu->PC = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]);
}

void op20(j65_t* cpu) {
	// TODO i have no idea if this is right
	push_stack(cpu);
	op4c(cpu);
}

void op21(j65_t* cpu) {
	and(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void op22(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op24(j65_t* cpu) {
	bit(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op25(j65_t* cpu) {
	cpu->A &= cpu->memmap[cpu->memmap[cpu->PC+1]];
	chkzero(cpu, cpu->A)
}

void op26(j65_t* cpu) {
	rol(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op28(j65_t* cpu) {
	cpu->P = cpu->stack[cpu->S++];
}

void op29(j65_t* cpu) {
	cpu->A &= cpu->memmap[cpu->PC+1];
	chkzero(cpu, cpu->A)
}

void op2a(j65_t* cpu) {
	rol(cpu, cpu->A);
}

void op2c(j65_t* cpu) {
	bit(cpu, cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)]);
}

void op2d(j65_t* cpu) {
	and(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op2e(j65_t* cpu) {
	rol(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op30(j65_t* cpu) {
	if((cpu->P & SET_P_NEGATIVE) != 0) branch(cpu);
}

void op31(j65_t* cpu) {
	and(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void op32(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op34(j65_t* cpu) {
}

void op35(j65_t* cpu) {
	cpu->A = cpu->memmap[cpu->memmap[cpu->PC+1]] & cpu->X;
	chkzero(cpu, cpu->A)
}

void op36(j65_t* cpu) {
	rol(cpu, (uint8_t)(cpu->X+cpu->memmap[cpu->memmap[cpu->PC+1]]));
}

void op38(j65_t* cpu) {
	cpu->P |= SET_P_CARRY;
}

void op39(j65_t* cpu) {
	// TODO right cpu->PC+i for cycle?
	if(new_page(cpu->PC+1, (uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	and(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op3a(j65_t* cpu) {
}

void op3c(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void op3d(j65_t* cpu) {
	// TODO right cpu->PC+i for cycle?
	if(new_page(cpu->PC+1, (uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	and(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op3e(j65_t* cpu) {
	rol(cpu, cpu->memmap[(uint16_t)(short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X)]);
}

void op60(j65_t* cpu) {
	cpu->PC = (cpu->stack[cpu->S+1]) | (cpu->stack[cpu->S+2] << 8);
	cpu->S += 2;
}

void op40(j65_t* cpu) {
	// TODO this seems *really, really* wrong to me, figure this out
	op28(cpu);
	op60(cpu);
}

void op41(j65_t* cpu) {
	xor(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void op42(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op44(j65_t* cpu) {
}

void op45(j65_t* cpu) {
	xor(cpu,  cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op46(j65_t* cpu) {
	lsr(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op48(j65_t* cpu) {
	cpu->stack[cpu->S--] = cpu->A;
}

void op49(j65_t* cpu) {
	xor(cpu,  cpu->memmap[cpu->PC+1]);
}

void op4a(j65_t* cpu) {
	lsr(cpu, cpu->A);
}

void op4d(j65_t* cpu) {
	xor(cpu,  short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]));
}

void op4e(j65_t* cpu) {
	lsr(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op50(j65_t* cpu) {
	if((cpu->P & SET_P_OVERFLOW) == 0) branch(cpu);
}

void op51(j65_t* cpu) {
	xor(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void op52(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op54(j65_t* cpu) {
}

void op55(j65_t* cpu) {
	xor(cpu,  cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
}

void op56(j65_t* cpu) {
	lsr(cpu, (uint8_t) (cpu->X+cpu->memmap[cpu->memmap[cpu->PC+1]]));
}

void op58(j65_t* cpu) {
	cpu->P &= MASK_P_INT;
}

void op59(j65_t* cpu) {
	if(new_page(cpu->PC+1, (uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	xor(cpu,  cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op5a(j65_t* cpu) {
	opea(cpu);
}

void op5c(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void op5d(j65_t* cpu) {
	if(new_page(cpu->PC+1, (uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])) == 1) cpu->ITC++;
	xor(cpu,  cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op5e(j65_t* cpu) {
	lsr(cpu, (uint16_t) (cpu->X+cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]));
}

void op61(j65_t* cpu) {
	adc(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void op62(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op64(j65_t* cpu) {
}

void op66(j65_t* cpu) {
	ror(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op68(j65_t* cpu) {
	cpu->A = cpu->stack[cpu->S++];
	chkzero(cpu, cpu->A)
}

void op65(j65_t* cpu) {
	adc(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void op69(j65_t* cpu) {
	adc(cpu, cpu->memmap[cpu->PC+1]);
}

void op6a(j65_t* cpu) {
	ror(cpu, cpu->A);
}

void op6c(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), acp = addr;
	#ifdef NMOS_6502
	// only do this on a 6502. 65c02 fixes this
	if(((addr+1) % 0x100) == 0) addr-=0x100;
	#endif
	cpu->PC = short2addr(cpu->memmap[acp], cpu->memmap[addr+1]);
}

void op6d(j65_t* cpu) {
	adc(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op6e(j65_t* cpu) {
	ror(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void op70(j65_t* cpu) {
	if((cpu->P & SET_P_OVERFLOW) != 0) branch(cpu);
}

void op71(j65_t* cpu) {
	adc(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void op72(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op74(j65_t* cpu) {
}

void op75(j65_t* cpu) {
	adc(cpu, cpu->memmap[cpu->X+cpu->memmap[cpu->PC+1]]);
}

void op76(j65_t* cpu) {
	ror(cpu, cpu->memmap[(uint8_t)(cpu->X+cpu->memmap[cpu->PC+1])]);
}

void op78(j65_t* cpu) {
	cpu->P |= SET_P_INT;
}

void op79(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addry = addr+cpu->Y;
	if(new_page(addr, addry) == 1) cpu->ITC++;
	adc(cpu, cpu->memmap[addry]);
}

void op7a(j65_t* cpu) {
}

void op7c(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void op7d(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	adc(cpu, cpu->memmap[addrx]);
}

void op7e(j65_t* cpu) {
	ror(cpu, cpu->memmap[(uint16_t)(cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]))]);
}

void op80(j65_t* cpu) {
}

void op81(j65_t* cpu) {
	cpu->A = cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])];
}

void op82(j65_t* cpu) {
}

void op84(j65_t* cpu) {
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->Y;
}

void op85(j65_t* cpu) {
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->A;
}

void op86(j65_t* cpu) {
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->X;
}

void op87(j65_t* cpu) {
	cpu->memmap[cpu->memmap[cpu->PC+1]] = cpu->A & cpu->X;
}

void op88(j65_t* cpu) {
	cpu->Y--;
	chknegative(cpu, cpu->Y);
}

void op89(j65_t* cpu) {
}

void op8a(j65_t* cpu) {
	cpu->A = cpu->X;
	chknegative(cpu, cpu->A);
	chkzero(cpu, cpu->A)
}

void op8b(j65_t* cpu) {
	cpu->A = cpu->X & cpu->A & cpu->memmap[cpu->PC+1];
	system_request(cpu, 0x8B, cpu->memmap[cpu->PC+1]);
}

void op8c(j65_t* cpu) {
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)] = cpu->Y;
}

void op8d(j65_t* cpu) {
	cpu->A = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
}

void op8e(j65_t* cpu) {
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)] = cpu->X;
}

void op8f(j65_t* cpu) {
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] = cpu->A & cpu->X;
}

void op90(j65_t* cpu) {
	if((cpu->P & SET_P_CARRY) == 0) branch(cpu);
}

void op91(j65_t* cpu) {
	cpu->A = cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])];
}

void op92(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void op94(j65_t* cpu) {
	cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]] = cpu->Y;
}

void op95(j65_t* cpu) {
	cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]] = cpu->A;
}

void op96(j65_t* cpu) {
	cpu->memmap[cpu->memmap[zpz(cpu->Y, cpu->memmap[cpu->PC+1])]] = cpu->X;
}

void op98(j65_t* cpu) {
	cpu->A = cpu->Y;
	chkzero(cpu, cpu->A)
	chknegative(cpu, cpu->A);
}

void op99(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]) + (uint16_t)cpu->Y;
	cpu->memmap[addr] = cpu->A;
}

void op9a(j65_t* cpu) {
	cpu->S = cpu->X;
}

void op9d(j65_t* cpu) {
	cpu->A = cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
}

void opa0(j65_t* cpu) {
	ldy(cpu, cpu->memmap[cpu->PC+1]);
}

void opa1(j65_t* cpu) {
	lda(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void opa2(j65_t* cpu) {
	ldx(cpu, cpu->memmap[cpu->PC+1]);
}

void opa4(j65_t* cpu) {
	ldy(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opa5(j65_t* cpu) {
	lda(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opa6(j65_t* cpu) {
	ldx(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opa7(j65_t* cpu) {
	lax(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opa8(j65_t* cpu) {
	cpu->Y = cpu->A;
	chknegative(cpu, cpu->Y);
	chkzero(cpu, cpu->Y)
}

void opa9(j65_t* cpu) {
	lda(cpu, cpu->memmap[cpu->PC+1]);
}

void opaa(j65_t* cpu) {
	cpu->X = cpu->A;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
}

void opab(j65_t* cpu) {
	cpu->A &= cpu->memmap[cpu->PC+1];
	cpu->X = cpu->A & cpu->memmap[cpu->PC+1];
	system_request(cpu, 0xAB, cpu->memmap[cpu->PC+1]);
}

void opac(j65_t* cpu) {
	ldy(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+1])]);
}

void opad(j65_t* cpu) {
	lda(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opae(j65_t* cpu) {
	ldx(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opaf(j65_t* cpu) {
	cpu->A = cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)];
	cpu->X = cpu->A;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
}

void opb0(j65_t* cpu) {
	if((cpu->P & SET_P_CARRY) != 0) branch(cpu);
}

void opb1(j65_t* cpu) {
	lda(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void opb2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opb4(j65_t* cpu) {
	ldy(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
}

void opb5(j65_t* cpu) {
	lda(cpu, cpu->memmap[cpu->memmap[zpz(cpu->X, cpu->memmap[cpu->PC+1])]]);
}

void opb6(j65_t* cpu) {
	ldx(cpu, cpu->memmap[cpu->memmap[zpz(cpu->Y, cpu->memmap[cpu->PC+1])]]);
}

void opb8(j65_t* cpu) {
	cpu->P &= MASK_P_OVERFLOW;
}

void opb9(j65_t* cpu) {
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	cpu->A = cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
	chknegative(cpu, cpu->A);
	chkzero(cpu, cpu->A)
}

void opba(j65_t* cpu) {
	cpu->X = cpu->S;
	chknegative(cpu, cpu->X);
	chkzero(cpu, cpu->X)
}

void opbc(j65_t* cpu) {
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	cpu->Y = cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])];
	chknegative(cpu, cpu->Y);
	chkzero(cpu, cpu->Y)
}

void opbd(j65_t* cpu) {
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	lda(cpu, cpu->memmap[(uint16_t)cpu->X+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opbe(j65_t* cpu) {
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	ldx(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opbf(j65_t* cpu) {
	if(new_page(cpu->PC+1, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]) == 1) cpu->ITC++;
	lda(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
	ldx(cpu, cpu->memmap[(uint16_t)cpu->Y+short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opc0(j65_t* cpu) {
	cpy(cpu, cpu->memmap[cpu->PC+1]);
}

void opc1(j65_t* cpu) {
	cmp(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void opc2(j65_t* cpu) {
}

void opc4(j65_t* cpu) {
	cpy(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opc5(j65_t* cpu) {
	cmp(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void opc6(j65_t* cpu) {
	uint8_t tmp = cpu->memmap[cpu->memmap[cpu->PC+1]];
	tmp--;
	cpu->memmap[cpu->memmap[cpu->PC+1]]--;
	chknegative(cpu, tmp);
	chkzero(cpu, tmp);
}

void opc8(j65_t* cpu) {
	cpu->Y++;
	chknegative(cpu, cpu->Y);
}

void opc9(j65_t* cpu) {
	cmp(cpu, cpu->memmap[cpu->PC+1]);
}

void opca(j65_t* cpu) {
	cpu->X--;
	chknegative(cpu, cpu->X);
}

void opcc(j65_t* cpu) {
	cpy(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opcd(j65_t* cpu) {
	cmp(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void opce(j65_t* cpu) {
	uint8_t tmp = cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)];
	tmp--;
	cpu->memmap[short2addr(cpu->PC+1, cpu->PC+2)]--;
	chknegative(cpu, tmp);
	chkzero(cpu, tmp);
}

void opd1(j65_t* cpu) {
	cmp(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void opd2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opd4(j65_t* cpu) {
}

void opd5(j65_t* cpu) {
	cmp(cpu, cpu->memmap[(uint8_t)(cpu->X+cpu->memmap[cpu->PC+1])]);
}

void opd6(j65_t* cpu) {
	cpu->memmap[(uint8_t)(cpu->memmap[cpu->PC+1]+cpu->X)]--;
	uint8_t t = cpu->memmap[(uint8_t)(cpu->memmap[cpu->PC+1]+cpu->X)];
	chkzero(cpu, t);
	chknegative(cpu, t);
}

void opd8(j65_t* cpu) {
	cpu->P &= MASK_P_DECIMAL;
}

void opd0(j65_t* cpu) {
	if((cpu->P & SET_P_ZERO) == 0) branch(cpu);
}

void opd9(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addry = addr+cpu->Y;
	if(new_page(addr, addry) == 1) cpu->ITC++;
	cmp(cpu, cpu->memmap[addry]);
}

void opda(j65_t* cpu) {
}

void opdc(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void opdd(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	cmp(cpu, cpu->memmap[addrx]);
}

void opde(j65_t* cpu) {
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X]--;
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X];
	chkzero(cpu, tmp);
	chknegative(cpu, tmp);
}

void ope0(j65_t* cpu) {
	cpx(cpu, cpu->memmap[cpu->PC+1]);
}

void ope1(j65_t* cpu) {
	sbc(cpu, cpu->memmap[indexed_indirect(cpu, cpu->memmap[cpu->PC+1])]);
}

void ope2(j65_t* cpu) {
}

void ope4(j65_t* cpu) {
	cpx(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void ope5(j65_t* cpu) {
	sbc(cpu, cpu->memmap[cpu->memmap[cpu->PC+1]]);
}

void ope6(j65_t* cpu) {
	uint8_t tmp = cpu->memmap[cpu->PC+1];
	tmp++;
	cpu->memmap[cpu->PC+1]++;
	chknegative(cpu, tmp);
	chkzero(cpu, tmp);
}

void ope8(j65_t* cpu) {
	cpu->X++;
	chknegative(cpu, cpu->X);
}

void ope9(j65_t* cpu) {
	sbc(cpu, cpu->memmap[cpu->PC+1]);
}

void opec(j65_t* cpu) {
	cpx(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])]);
}

void oped(j65_t* cpu) {
	sbc(cpu, cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+1])]);
}

void opee(j65_t* cpu) {
	// TODO 90% sure something here is wrong
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] + 1;
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])] = tmp;
	chknegative(cpu, tmp);
	chkzero(cpu, tmp);
}

void opf0(j65_t* cpu) {
	if((cpu->P & SET_P_ZERO) != 0) branch(cpu);
}

void opf1(j65_t* cpu) {
	sbc(cpu, cpu->memmap[indirect_indexed(cpu, cpu->memmap[cpu->PC+1])]);
}

void opf2(j65_t* cpu) {
	system_request(cpu, cpu->memmap[cpu->PC], cpu->memmap[cpu->PC+1]);
}

void opf4(j65_t* cpu) {
}

void opf5(j65_t* cpu) {
	sbc(cpu, cpu->memmap[(uint8_t)(cpu->X+cpu->memmap[cpu->PC+1])]);
}

void opf6(j65_t* cpu) {
	cpu->memmap[(uint8_t)(cpu->memmap[cpu->PC+1]+cpu->X)]++;
	uint8_t t = cpu->memmap[(uint8_t)(cpu->memmap[cpu->PC+1]+cpu->X)];
	chkzero(cpu, t);
	chknegative(cpu, t);
}

void opf8(j65_t* cpu) {
	cpu->P |= SET_P_DECIMAL;
}

void opf9(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addry = addr+cpu->Y;
	if(new_page(addr, addry) == 1) cpu->ITC++;
	sbc(cpu, cpu->memmap[addry]);
}

void opfa(j65_t* cpu) {
}

void opfc(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
}

void opfd(j65_t* cpu) {
	uint16_t addr = short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2]), addrx = addr+cpu->X;
	if(new_page(addr, addrx) == 1) cpu->ITC++;
	sbc(cpu, cpu->memmap[addrx]);
}

void opfe(j65_t* cpu) {
	cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X]++;
	uint8_t tmp = cpu->memmap[short2addr(cpu->memmap[cpu->PC+1], cpu->memmap[cpu->PC+2])+cpu->X];
	chkzero(cpu, tmp);
	chknegative(cpu, tmp);
}

#ifdef NMOS_6502
#undef WDC_65C02
// opcode map for tick() to use
function_pointer_array opcodes[] = {
//  0  1      2      3      4      5      6      7      8      9      a      b      c      d      e      f
&op00, &op01, &op02, NULL,  &op04, &op05, &op06, NULL,  &op08, &op09, &op0a, NULL,  &op0c, &op0d, &op0e, NULL, // 0
&op10, &op11, &op12, NULL,  &op14, &op15, &op16, NULL,  &op18, &op19, &op1a, NULL,  &op1c, &op1d, &op1e, NULL, // 1
&op20, &op21, &op22, NULL,  &op24, &op25, &op26, NULL,  &op28, &op29, &op2a, NULL,  &op2c, &op2d, &op2e, NULL, // 2
&op30, &op31, &op32, NULL,  &op34, &op35, &op36, NULL,  &op38, &op39, &op3a, NULL,  &op3c, &op3d, &op3e, NULL, // 3
&op40, &op41, &op42, NULL,  &op44, &op45, &op46, NULL,  &op48, &op49, &op4a, NULL,  &op4c, &op4d, &op4e, NULL, // 4
&op50, &op51, &op52, NULL,  &op54, &op55, &op56, NULL,  &op58, &op59, &op5a, NULL,  &op5c, &op5d, &op5e, NULL, // 5
&op60, &op61, &op62, NULL,  &op64, &op65, &op66, NULL,  &op68, &op69, &op6a, NULL,  &op6c, &op6d, &op6e, NULL, // 6
&op70, &op71, &op72, NULL,  &op74, &op75, &op76, NULL,  &op78, &op79, &op7a, NULL,  &op7c, &op7d, &op7e, NULL, // 7
&op80, &op81, &op82, NULL,  &op84, &op85, &op86, &op87, &op88, &op89, &op8a, &op8b, &op8c, &op8d, &op8e, &op8f,// 8
&op90, &op91, &op92, NULL,  &op94, &op95, &op96, NULL,  &op98, &op99, &op9a, NULL,  NULL,  &op9d, NULL,  NULL, // 9
&opa0, &opa1, &opa2, NULL,  &opa4, &opa5, &opa6, &opa7, &opa8, &opa9, &opaa, &opab, &opac, &opad, &opae, &opaf,// a
&opb0, &opb1, &opb2, NULL,  &opb4, &opb5, &opb6, &opb6, &opb8, &opb9, &opba, NULL,  &opbc, &opbd, &opbe, &opbf,// b
&opc0, &opc1, &opc2, NULL,  &opc4, &opc5, &opc6, NULL,  &opc8, &opc9, &opca, NULL,  &opcc, &opcd, &opce, NULL, // c
&opd0, &opd1, &opd2, NULL,  &opd4, &opd5, &opd6, NULL,  &opd8, &opd9, &opda, NULL,  &opdc, &opdd, &opde, NULL, // d
&ope0, &ope1, &ope2, NULL,  &ope4, &ope5, &ope6, NULL,  &ope8, &ope9, &opea, NULL,  &opec, &oped, &opee, NULL, // e
&opf0, &opf1, &opf2, NULL,  &opf4, &opf5, &opf6, NULL,  &opf8, &opf9, &opfa, NULL,  &opfc, &opfd, &opfe, NULL  // f
};

uint32_t times[] = {
7, 6, 0xFFFFFFFF, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
2, 5, 0xFFFFFFFF, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
6, 6, 0xFFFFFFFF, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
2, 5, 0xFFFFFFFF, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
6, 6, 0xFFFFFFFF, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
2, 5, 0xFFFFFFFF, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
6, 6, 0xFFFFFFFF, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
2, 5, 0xFFFFFFFF, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
2, 6, 0xFFFFFFFF, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
2, 5, 0xFFFFFFFF, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 3, 6,
2, 5, 0xFFFFFFFF, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
2, 5, 0xFFFFFFFF, 4, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

uint8_t pcinc[] = {
/*
0d 1d 2d 3d 4d 5d 6d 7d 8d 9d ad bd cd dd  e  fd
*/
1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, //d
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, //d
0, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 0, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 0, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
2, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
2, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3, 
2, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3, 
1, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3
};
#endif

#ifdef WDC_65C02

// TODO
#error no 65c02 map

#endif

void tick(j65_t* cpu) {
	if(cpu->ITC == 1) {
		// TODO run code cycle end
		if((cpu->I) != 0 && (cpu->P & SET_P_INT) == 0) {
			// TODO this should not trigger given an IRQ
			// TODO emulate 6502 interrupt bugs like hijacking
			#ifndef NOLIBC
			printf("INTERRUPT\n");
			#endif
			uint16_t vec = 0xFFFC;
			if((cpu->I & 4) != 0) vec = 0xFFFC;
			interrupt(cpu, vec);
		}
		uint8_t copc = cpu->memmap[cpu->PC];
		cpu->ITC = times[copc];
		if(opcodes[copc] != NULL) {
			#ifndef NOLIBC
			printf("Opcode 0x%02X at 0x%04X\n", copc, cpu->PC);
			#endif
			opcodes[copc](cpu);
		} else {
			#ifndef NOLIBC
			printf("NULL OPCODE: 0x%02X. It will be replaced by NOP.\ncpu->PC: 0x%04X\n", copc, cpu->PC);
			#endif
		}
		cpu->PC+=pcinc[copc];
	}
	cpu->ITC--;
}
