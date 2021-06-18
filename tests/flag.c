#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

int main() {
	j65_t cpu;
	j65_init(&cpu);

	cpu.ITC = 1;
	cpu.P = 0b01000000;
	// test SE*
	cpu.memmap[0] = 0x38;
	cpu.memmap[1] = 0xF8;
	cpu.memmap[2] = 0x78;
	while(cpu.PC < 3) tick();
	assert((cpu.P & 0b00001101) == 0b00001101);
	// test CLV by setting overflow manually
	cpu.P |= 0b01000000;
	// test CL*
	cpu.memmap[3] = 0x18;
	cpu.memmap[4] = 0xD8;
	cpu.memmap[5] = 0x58;
	cpu.memmap[6] = 0xB8;
	while(cpu.PC < 7) tick();
	assert(cpu.P == 0);
	printf("CL*/SE* test passed.\n");
}
