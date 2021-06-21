#include "cpu/cpu.h"
#include "cpu/func_proto.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>

void sysrq(uint8_t caller, uint8_t data) {
	printf("Recieved SYSRQ! Aborting in case of error.\n");
	abort();
}

int main(int argc, char **argv) {
	int fd = open(argv[1], O_RDONLY);
	j65_t cpu;
	j65_init(&cpu);
	uint8_t *mainmem = mmap(NULL, 0xFFFF, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	// show the binary
	// TODO only shows FFE0 bytes
	for(int x = 0; x < 0x1000; x++) {
		if((x % 0x1F) == 0) printf("\n");
		if(mainmem[x] != 0) printf("\e[1m");
		printf("%02X ", mainmem[x]);
		if(mainmem[x] != 0) printf("\e[0m");
	}
	printf("\n");
	cpu.memmap = mainmem;
	while(1) {
		tick(&cpu);
	}
	return 0;
}
