#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/mman.h>

#define binchar(b) (b & 0x80 ? '1' : '0' ), (b & 0x40 ? '1' : '0' ), (b & 0x20 ? '1' : '0' ), (b & 0x10 ? '1' : '0' ), (b & 0x08 ? '1' : '0' ), (b & 0x04 ? '1' : '0' ), (b & 0x02 ? '1' : '0' ), (b & 0x01 ? '1' : '0' )

uint8_t chkautorun = 0, lastc = 's', i = 0, running = 1;

void sigint(int sig) {
	printf("\nGot signal %i, stopping autorun\n", sig);
	chkautorun = 0;
}

void sysrq(uint8_t caller, uint8_t data) {
	printf("Recieved unexpected SYSRQ! Aborting in case of error.\nC 0x%02X D 0x%02X\n", caller, data);
	abort();
}

int main(int argc, char **argv) {
	assert(argc > 1);
	int fd = open(argv[1], O_RDONLY);
	signal(SIGINT, sigint);
	j65_t cpu;
	j65_init(&cpu);
	uint8_t *mainmem = mmap(NULL, 0xFFFF, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	// show the binary
	// TODO only shows FFE0 bytes
	/*
	for(int x = 0; x < 0x1000; x++) {
		if((x % 0x1F) == 0) printf("\n");
		if(mainmem[x] != 0) printf("\e[1m");
		printf("%02X ", mainmem[x]);
		if(mainmem[x] != 0) printf("\e[0m");
	}
	printf("\n");
	*/
	cpu.memmap = mainmem;
	j65_set_stack(&cpu);
	while(running) {
		if(chkautorun == 0) {
			printf("> ");
			i = getchar();

			switch(i) {
				case 'a':
					chkautorun = 1;
					break;
				case 'h':
					printf(
					"a - autorun\n"
					"q - quit\n"
					"s/enter - step\n"
					"r - show register state\n"
					);
					break;
				case 'r':
					printf(
					"A  - 0x%02X\n"
					"X  - 0x%02X\n"
					"Y  - 0x%02X\n"
					"S  - 0x%02X\n"
					"PC - 0x%04X\n"
					"P  - 0b%c%c%c%c%c%c%c%c (0x%02X)\n"
					"       NVUBDIZC\n",
					cpu.A, cpu.X, cpu.Y, cpu.S, cpu.PC, binchar(cpu.P), cpu.P
					);
					break;
				case 's':
					tick(&cpu);
					break;
				case 'q':
					running = 0;
					break;
				default:
					tick(&cpu);
					break;
			}

		} else tick(&cpu);
	}
	return 0;
}
