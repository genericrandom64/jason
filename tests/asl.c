#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

#define TRUE 1
#define FALSE 0

#define GET_C (cpu->P & SET_P_CARRY)
#define GET_N (cpu->P & SET_P_NEGATIVE)

void test_asl(j65_t* cpu, uint8_t value, int _PC, uint8_t _C, uint8_t _N) {
	cpu->PC = 0;
	uint8_t result = (value << 1) & 0xFF;
	while(cpu->PC < _PC) tick(cpu);
	assert(cpu->A==result);	
	if (_C) {
		assert(GET_C!=0);	
	} else {
		assert(GET_C==0);	
	}
	if (_N) {
		assert(GET_N!=0);	
	} else {
		assert(GET_N==0);	
	}
}

#define TEST_RESULT() printf("Test: %d passed\n", test_id++);

int main() {
	j65_t cpu;
	j65_init(&cpu);
	int test_id = 1;
	cpu.PC = 0;
	cpu.ITC = 1;
	cpu.P = 0;	

	// test ASL 
	cpu.A = 0xab;
	cpu.memmap[0] = 0x0A;
	test_asl(&cpu, 0xab, 1, TRUE, FALSE);
	TEST_RESULT()

	cpu.A = 0x7f;
	cpu.memmap[0] = 0x0A;
	test_asl(&cpu, 0x7f, 1, FALSE, TRUE);
	TEST_RESULT()

	cpu.A = 0xff;
	cpu.memmap[0] = 0x0A;
	test_asl(&cpu, 0xff, 1, TRUE, TRUE);
	TEST_RESULT()

	cpu.memmap[0] = 0x06;
	cpu.memmap[1] = 0x2;
	cpu.memmap[2] = 0xab;
	test_asl(&cpu, 0xab, 2, TRUE, FALSE);
	TEST_RESULT()

	cpu.memmap[0] = 0x06;
	cpu.memmap[1] = 0x2;
	cpu.memmap[2] = 0x7f;
	test_asl(&cpu, 0x7f, 2, FALSE, TRUE);
	TEST_RESULT()

	cpu.memmap[0] = 0x06;
	cpu.memmap[1] = 0x2;
	cpu.memmap[2] = 0xff;
	test_asl(&cpu, 0xff, 2, TRUE, TRUE);
	TEST_RESULT()

	cpu.memmap[0] = 0x0e;
	cpu.memmap[1] = 0x0;
	cpu.memmap[2] = 0x2;
	cpu.memmap[0x200] = 0xab;
	test_asl(&cpu, 0xab, 3, TRUE, FALSE);
	TEST_RESULT()

	cpu.memmap[0] = 0x0e;
	cpu.memmap[1] = 0x0;
	cpu.memmap[2] = 0x2;
	cpu.memmap[0x200] = 0x7f;
	test_asl(&cpu, 0x7f, 3, FALSE, TRUE);
	TEST_RESULT()

	cpu.memmap[0] = 0x0e;
	cpu.memmap[1] = 0x0;
	cpu.memmap[2] = 0x2;
	cpu.memmap[0x200] = 0xff;
	test_asl(&cpu, 0xff, 3, TRUE, TRUE);
	TEST_RESULT()

	printf("ASL test passed.\n");
}
