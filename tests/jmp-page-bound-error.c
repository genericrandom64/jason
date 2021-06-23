#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	j65_t cpu;
	j65_init(&cpu);
	// test 6502 jmp bug
	cpu.PC = 0;
	cpu.ITC = 1;
	for(uint16_t i = 0; i < 0xFFFF; i++) {
		cpu.memmap[i] = 0x75;
	}
	cpu.memmap[0] = 0x6C;
	cpu.memmap[1] = 0xFF;
	cpu.memmap[2] = 0x30;
	cpu.memmap[0x3000] = 0x40;
	cpu.memmap[0x3100] = 0x50;
	cpu.memmap[0x30FF] = 0x80;
	cpu.memmap[0x4080] = 0x4c;
	cpu.memmap[0x4081] = 0;
	cpu.memmap[0x4082] = 0;
	tick(&cpu);
	printf("PC %X\n", cpu.PC);
	assert(cpu.PC == 0x4080);
	printf("JMP on page boundary failed successfully.\n");

}
