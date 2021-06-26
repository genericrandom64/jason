#include <stdint.h>
#include <stddef.h>

// No libc functions
//#define NOLIBC

// No support for binary-coded decimal
//#define NOBCD

// NES specific functions
//#define NES

#ifdef NOLIBC
#warning While j65 is mostly not reliant on libc, it requires working implementations of:
#warning memcpy.
#warning You will have to provide the above functions or patch the code.
#else
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

// CPU REGISTERS

typedef struct cpu {
	// same as the 6502 reg names
	uint16_t PC;
	uint8_t	S;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t P;

	/* Interrupt flag
	 * VALUE	INTERRUPT TYPE		INTERRUPT VECTOR (LSB,MSB)
	 * 0 		no interrupt		N/A
	 * 1		Non-Maskable Interrupt	0xFFFA,0xFFFB
	 * 2		Processor Reset		0xFFFC,0xFFFD
	 * 4		Interrupt Request	0xFFFE,0xFFFF
	 */
	uint8_t I;

	uint8_t ITC;
	uint8_t *memmap;
	uint8_t *stack;
} j65_t;

void j65_init(j65_t* cpu);
void j65_set_stack(j65_t* cpu);
void op08(j65_t* cpu);
	
// PROGRAM DEFINES AND FUNCTION PROTOTYPES

typedef void(*function_pointer_array)(j65_t*);

// STATUS FLAGS

#define SET_P_CARRY	1 << 0 // An operation needs an extra bit, which is stored here
#define SET_P_ZERO	1 << 1 // An operation left the terget with value 0
#define SET_P_INT	1 << 2 // Disables interrupts if we are currently servicing one
#define SET_P_DECIMAL	1 << 3 // Binary-coded decimal (only used for ADC/SBC)

#define SET_P_B1	1 << 4 // Flags that seem to be entirely unused and inaccessible
#define SET_P_B2	1 << 5

#define SET_P_OVERFLOW	1 << 6 // An operation under/overflowed a number
#define SET_P_NEGATIVE	1 << 7 // An operation has set the highest bit to 1 (10000000)


#define MASK_P_CARRY	0b11111110
#define MASK_P_ZERO	0b11111101
#define MASK_P_INT	0b11111011
#define MASK_P_DECIMAL	0b11110111

#define MASK_P_B1	0b11101111
#define MASK_P_B2	0b11011111

#define MASK_P_OVERFLOW	0b10111111
#define MASK_P_NEGATIVE	0b01111111
