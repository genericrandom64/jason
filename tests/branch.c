#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

void test(j65_t* cpu) {
	cpu->PC = 0;
	cpu->ITC = 1;
	for(uint8_t i = 0; i < 20; i++) {tick(cpu); assert(cpu->PC != 0x2 && cpu->PC != 0x3);}
	assert(cpu->PC < 8);
}

void testz(j65_t* cpu) {
	cpu->PC = 0;
	cpu->ITC = 1;
	for(uint8_t i = 0; i < 10; i++) {tick(cpu);}
}

void fail(uint8_t caller, uint8_t data) {
	printf("Branch did not fail.\n");
	abort();
}

int main() {
	j65_t cpu;
	j65_init(&cpu);
	cpu.P = 0;
	register_system_request(fail);
	for(uint16_t i; i < 0xFFFF; i++) {
		cpu.memmap[i] = 0x75;
	}
	cpu.memmap[0] = 0x50;
	cpu.memmap[1] = 2;
	cpu.memmap[4] = 0x4c;
	cpu.memmap[5] = 0;
	cpu.memmap[6] = 0;
	test(&cpu);
	printf("BVC passed.\n");

	cpu.memmap[0] = 0x70;
	cpu.P |= SET_P_OVERFLOW;
	test(&cpu);
	printf("BVS passed.\n");

	cpu.memmap[0] = 0x10;
	test(&cpu);
	printf("BPL passed.\n");

	cpu.memmap[0] = 0x30;
	cpu.P |= SET_P_NEGATIVE;
	test(&cpu);
	printf("BMI passed.\n");

	cpu.memmap[0] = 0xD0;
	test(&cpu);
	printf("BNE passed.\n");

	cpu.memmap[0] = 0xF0;
	cpu.P |= SET_P_ZERO;
	test(&cpu);
	printf("BEQ passed.\n");

	cpu.memmap[0] = 0x90;
	test(&cpu);
	printf("BCC passed.\n");

	cpu.memmap[0] = 0xB0;
	cpu.P |= SET_P_CARRY;
	test(&cpu);
	printf("BCS passed.\n");
	printf("Succeeding branch test passed.\n");
	
	cpu.memmap[1] = 3;
	cpu.memmap[2] = 0x4c;
	cpu.memmap[3] = 0x10;
	cpu.memmap[4] = 0x10;
	cpu.memmap[5] = 0x8B;

	cpu.memmap[0] = 0x90;
	cpu.P |= SET_P_CARRY;
	testz(&cpu);
	printf("Untaken BCC passed.\n");

	cpu.memmap[0] = 0xB0;
	cpu.P &= MASK_P_CARRY;
	testz(&cpu);
	printf("Untaken BCS passed.\n");

	cpu.memmap[0] = 0x70;
	cpu.P &= MASK_P_OVERFLOW;
	testz(&cpu);
	printf("Untaken BVS passed.\n");

	cpu.memmap[0] = 0x30;
	cpu.P &= MASK_P_NEGATIVE;
	testz(&cpu);
	printf("Untaken BMI passed.\n");

	cpu.memmap[0] = 0xD0;
	cpu.P |= SET_P_ZERO;
	testz(&cpu);
	printf("Untaken BNE passed.\n");

	cpu.memmap[0] = 0x10;
	cpu.P |= SET_P_NEGATIVE;
	testz(&cpu);
	printf("Untaken BPL passed.\n");

	cpu.memmap[0] = 0x50;
	cpu.P |= SET_P_OVERFLOW;
	testz(&cpu);
	printf("Untaken BVC passed.\n");
	printf("Failing branch test passed.\n");

	cpu.P = 0;
	cpu.PC = 0;
	cpu.ITC = 1;
	cpu.memmap[0] = 0x4C;
	cpu.memmap[1] = 0x10;
	cpu.memmap[2] = 0x10;
	cpu.memmap[0x1010] = 0x50;
	cpu.memmap[0x1011] = 0xFE;
	
	for(uint8_t i = 0; i < 10; i++) {tick(&cpu); assert(cpu.PC < 0x1011 && cpu.PC > 0x100F);}

	printf("Negative branch test passed.\n");

	printf("All branch tests passed.\n");

}
