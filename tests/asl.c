#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

#define TRUE 1
#define FALSE 0

#define GET_C (P & SET_P_CARRY)
#define GET_N (P & SET_P_NEGATIVE)

void test_asl(uint8_t value, int _PC, uint8_t _C, uint8_t _N) {
	PC = 0;
	uint8_t result = (value << 1) & 0xFF;
	while(PC < _PC) tick();
	assert(A==result);	
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
	int test_id = 1;
	PC = 0;
	ITC = 1;
	P = 0;	

	// test ASL 
	A = 0xab;
	memmap[0] = 0x0A;
	test_asl(0xab, 1, TRUE, FALSE);
	TEST_RESULT()

	A = 0x7f;
	memmap[0] = 0x0A;
	test_asl(0x7f, 1, FALSE, TRUE);
	TEST_RESULT()

	A = 0xff;
	memmap[0] = 0x0A;
	test_asl(0xff, 1, TRUE, TRUE);
	TEST_RESULT()

	memmap[0] = 0x06;
	memmap[1] = 0x2;
	memmap[2] = 0xab;
	test_asl(0xab, 2, TRUE, FALSE);
	TEST_RESULT()

	memmap[0] = 0x06;
	memmap[1] = 0x2;
	memmap[2] = 0x7f;
	test_asl(0x7f, 2, FALSE, TRUE);
	TEST_RESULT()

	memmap[0] = 0x06;
	memmap[1] = 0x2;
	memmap[2] = 0xff;
	test_asl(0xff, 2, TRUE, TRUE);
	TEST_RESULT()

	memmap[0] = 0x0e;
	memmap[1] = 0x0;
	memmap[2] = 0x2;
	memmap[0x200] = 0xab;
	test_asl(0xab, 3, TRUE, FALSE);
	TEST_RESULT()

	memmap[0] = 0x0e;
	memmap[1] = 0x0;
	memmap[2] = 0x2;
	memmap[0x200] = 0x7f;
	test_asl(0x7f, 3, FALSE, TRUE);
	TEST_RESULT()

	memmap[0] = 0x0e;
	memmap[1] = 0x0;
	memmap[2] = 0x2;
	memmap[0x200] = 0xff;
	test_asl(0xff, 3, TRUE, TRUE);
	TEST_RESULT()

	printf("ASL test passed.\n");
}
