#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	// fill memory with garbage data to eliminate false positives
	for(uint16_t i; i < 0xFFFF; i++) {
		memmap[i] = 0x75;
	}
	PC = 0;
	ITC = 1;
	memmap[0] = 0x4c;
	memmap[1] = 0xce;
	memmap[2] = 0xfa;
	memmap[0xFACE] = 0x4c;
	memmap[0xFACF] = 3;
	memmap[0xFAD0] = 0;
	memmap[3] = 0x6c;
	memmap[4] = 0x21;
	memmap[5] = 0x20;
	memmap[0x2021] = 0;
	memmap[0x2022] = 0;
	/* hmm yes
	memmap[0x2021] = 0x30;
	memmap[0x2022] = 0x30;
	memmap[0x3030] = 0;
	memmap[0x3031] = 0;
	*/
	for(unsigned int i = 0; i < 0x20000; i++) {
		tick();
		// these are the 3 places the cpu can jump given the above bincode
		assert(PC == 0 || PC == 3 || PC == 0xFACE);
	}
	printf("JMP test passed.\n");
}
