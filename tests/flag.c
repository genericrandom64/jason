#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	PC = 0;
	ITC = 1;
	P = 0b01000000;
	// test SE*
	memmap[0] = 0x38;
	memmap[1] = 0xF8;
	memmap[2] = 0x78;
	while(PC < 3) tick();
	assert(P & 0b00001101 == 0b00001101);
	// test CLV by setting overflow manually
	P |= 0b01000000;
	// test CL*
	memmap[3] = 0x18;
	memmap[4] = 0xD8;
	memmap[5] = 0x58;
	memmap[6] = 0xB8;
	while(PC < 7) tick();
	assert(P == 0);
	printf("CL*/SE* test passed.\n");
}
