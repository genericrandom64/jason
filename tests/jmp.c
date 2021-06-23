#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	j65_t cpu;
	j65_init(&cpu);
	// fill memory with garbage data to eliminate false positives
	for(uint16_t i = 0; i < 0xFFFF; i++) {
		cpu.memmap[i] = 0x75;
	}
	cpu.PC = 0;
	cpu.ITC = 1;
	cpu.memmap[0] = 0x4c;
	cpu.memmap[1] = 0xce;
	cpu.memmap[2] = 0xfa;
	cpu.memmap[0xFACE] = 0x4c;
	cpu.memmap[0xFACF] = 3;
	cpu.memmap[0xFAD0] = 0;
	cpu.memmap[3] = 0x6c;
	cpu.memmap[4] = 0x21;
	cpu.memmap[5] = 0x20;
	cpu.memmap[0x2021] = 0;
	cpu.memmap[0x2022] = 0;
	/* hmm yes
	memmap[0x2021] = 0x30;
	memmap[0x2022] = 0x30;
	memmap[0x3030] = 0;
	memmap[0x3031] = 0;
	*/
	for(unsigned int i = 0; i < 0x20000; i++) {
		tick(&cpu);
		// these are the 3 places the cpu can jump given the above bincode
		assert(cpu.PC == 0 || cpu.PC == 3 || cpu.PC == 0xFACE);
	}
	printf("JMP test passed.\n");
}
