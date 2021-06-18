#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

uint16_t i = 0;

void syshandle(uint8_t caller, uint8_t data) {
	printf("Request handled: 0x%X 0x%X\n", caller, data);
	i+=data;
}

int main() {
	j65_t cpu;
	j65_init(&cpu);
	cpu.PC = 0;
	cpu.ITC = 1;
	for(uint16_t i; i < 0xFFFF; i++) {
		cpu.memmap[i] = 0x75;
	}
	register_system_request(syshandle);
	cpu.memmap[0] = 0x8B;
	cpu.memmap[1] = 12;
	cpu.memmap[2] = 0xAB;
	cpu.memmap[3] = 13;
	cpu.memmap[4] = 0x4C;
	cpu.memmap[5] = 0;
	cpu.memmap[6] = 0;
	for(unsigned int i = 0; i < 0x200; i++) {
		tick(&cpu);
	}
	assert(i == 3200);
	printf("SYSRQ test passed.\n");
}
