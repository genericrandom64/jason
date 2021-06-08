#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	// test 6502 jmp bug
	PC = 0;
	ITC = 1;
	for(uint16_t i; i < 0xFFFF; i++) {
		memmap[i] = 0x75;
	}
	memmap[0] = 0x6C;
	memmap[1] = 0xFF;
	memmap[2] = 0x30;
	memmap[0x3000] = 0x40;
	memmap[0x3100] = 0x50;
	memmap[0x30FF] = 0x80;
	memmap[0x4080] = 0x4c;
	memmap[0x4081] = 0;
	memmap[0x4082] = 0;
	tick();
	printf("PC %X\n", PC);
	assert(PC == 0x4080);
	printf("JMP on page boundary failed successfully.\n");

}
