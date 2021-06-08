#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

uint16_t i = 0;

void syshandle(uint8_t caller, uint8_t data) {
	printf("Request handled: 0x%X 0x%X\n", caller, data);
	i+=data;
}

int main() {
	PC = 0;
	ITC = 1;
	for(uint16_t i; i < 0xFFFF; i++) {
		memmap[i] = 0x75;
	}
	register_system_request(syshandle);
	memmap[0] = 0x8B;
	memmap[1] = 12;
	memmap[2] = 0xAB;
	memmap[3] = 13;
	memmap[4] = 0x4C;
	memmap[5] = 0;
	memmap[6] = 0;
	for(unsigned int i = 0; i < 0x200; i++) {
		tick();
	}
	assert(i == 3200);
	printf("SYSRQ test passed.\n");
}
