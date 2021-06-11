#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

void test() {
	PC = 0;
	ITC = 1;
	for(uint8_t i = 0; i < 20; i++) {tick(); assert(PC != 0x2 && PC != 0x3);}
	assert(PC < 8);
}

void testz() {
	PC = 0;
	ITC = 1;
	for(uint8_t i = 0; i < 10; i++) {tick();}
}

void fail(uint8_t caller, uint8_t data) {
	printf("Branch did not fail.\n");
	abort();
}

int main() {
	P = 0;
	register_system_request(fail);
	for(uint16_t i; i < 0xFFFF; i++) {
		memmap[i] = 0x75;
	}
	memmap[0] = 0x50;
	memmap[1] = 2;
	memmap[4] = 0x4c;
	memmap[5] = 0;
	memmap[6] = 0;
	test();
	printf("BVC passed.\n");

	memmap[0] = 0x70;
	P |= SET_P_OVERFLOW;
	test();
	printf("BVS passed.\n");

	memmap[0] = 0x10;
	test();
	printf("BPL passed.\n");

	memmap[0] = 0x30;
	P |= SET_P_NEGATIVE;
	test();
	printf("BMI passed.\n");

	memmap[0] = 0xD0;
	test();
	printf("BNE passed.\n");

	memmap[0] = 0xF0;
	P |= SET_P_ZERO;
	test();
	printf("BEQ passed.\n");

	memmap[0] = 0x90;
	test();
	printf("BCC passed.\n");

	memmap[0] = 0xB0;
	P |= SET_P_CARRY;
	test();
	printf("BCS passed.\n");
	printf("Succeeding branch test passed.\n");
	
	memmap[1] = 3;
	memmap[2] = 0x4c;
	memmap[3] = 0x10;
	memmap[4] = 0x10;
	memmap[5] = 0x8B;

	memmap[0] = 0x90;
	P |= SET_P_CARRY;
	testz();
	printf("Untaken BCC passed.\n");

	memmap[0] = 0xB0;
	P &= MASK_P_CARRY;
	testz();
	printf("Untaken BCS passed.\n");

	memmap[0] = 0x70;
	P &= MASK_P_OVERFLOW;
	testz();
	printf("Untaken BVS passed.\n");

	memmap[0] = 0x30;
	P &= MASK_P_NEGATIVE;
	testz();
	printf("Untaken BMI passed.\n");

	memmap[0] = 0xD0;
	P |= SET_P_ZERO;
	testz();
	printf("Untaken BNE passed.\n");

	memmap[0] = 0x10;
	P |= SET_P_NEGATIVE;
	testz();
	printf("Untaken BPL passed.\n");

	memmap[0] = 0x50;
	P |= SET_P_OVERFLOW;
	testz();
	printf("Untaken BVC passed.\n");
	printf("Failing branch test passed.\n");

	P = 0;
	PC = 0;
	ITC = 1;
	memmap[0] = 0x4C;
	memmap[1] = 0x10;
	memmap[2] = 0x10;
	memmap[0x1010] = 0x50;
	memmap[0x1011] = 0xFE;
	
	for(uint8_t i = 0; i < 10; i++) {tick(); assert(PC < 0x1011 && PC > 0x100F);}

	printf("Negative branch test passed.\n");

	printf("All branch tests passed.\n");

}
