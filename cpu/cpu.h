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

extern uint16_t PC;	// program counter
extern uint8_t	S,	// stack pointer
	A,		// accumulator
	X,		// X register
	Y,		// Y register
	P,		// status register
	ITC;		// instruction timer to waste cycles until the instruction is supposed to be done

// MEMORY
#ifdef NES
extern char	vram[2048],	// 2kb of video ram
		oam[256],	// sprite data
		palette[28];	// on the ppu chip
#endif
extern uint8_t
	memmap[0xFFFF],	// system memory
	*stack;	// convenience pointer for the stack on page 1

// PROGRAM DEFINES AND FUNCTION PROTOTYPES

#define UOP // We want undefined opcodes
#define NOBCD // we do *not* want decimal mode in an NES emulator

typedef void(*function_pointer_array)();

#ifdef NES
// MEMORY MAPPED REGISTERS
// https://wiki.nesdev.com/w/index.php/2A03

#define SQ1_VOL 0x4000
#define SQ1_SWEEP 0x4001
#define SQ1_LO 0x4002
#define SQ1_HI 0x4003

#define SQ2_VOL 0x4004
#define SQ2_SWEEP 0x4005
#define SQ2_LO 0x4006
#define SQ2_HI 0x4007

#define TRI_LINEAR 0x4008
#define TRI_LO 0x400a
#define TRI_HI 0x400b

#define NOISE_VOL 0x400c
#define NOISE_LO 0x400e
#define NOISE_HI 0x400f

#define UM1 0x4009
#define UM2 0x400d

#define DMC_FREQ 0x4010
#define DMC_RAW 0x4011
#define DMC_START 0x4012
#define DMC_LEN 0x4013

#define OAMDMA 0x4014

#define SND_CHN 0x4015

#define JOY1 0x4016
#define JOY2 0x4017
#endif

// STATUS FLAGS
#define SET_P_CARRY	1 << 0
#define SET_P_ZERO	1 << 1
#define SET_P_INT	1 << 2
#define SET_P_DECIMAL	1 << 3

#define SET_P_B1	1 << 4
#define SET_P_B2	1 << 5

#define SET_P_OVERFLOW	1 << 6
#define SET_P_NEGATIVE	1 << 7


#define MASK_P_CARRY	0b11111110
#define MASK_P_ZERO	0b11111101
#define MASK_P_INT	0b11111011
#define MASK_P_DECIMAL	0b11110111

#define MASK_P_B1	0b11101111
#define MASK_P_B2	0b11011111

#define MASK_P_OVERFLOW	0b10111111
#define MASK_P_NEGATIVE	0b01111111

// OPCODES
// Names are defined as such:
// IX - Indirect, X register
// IY - Indirect, Y register
// ZP - Zero Page (memory addresses 0-255)
// IM - IMmediate
// AC - ACcumulator
// AB - uses ABsolute addresses

/*
#define OP_BRK		0x00 // break, PC++; time 7
#define OP_ORA_IX	0x01 // OR with accumulator, PC+=2; time 6
#define OP_		0x02
#define OP_		0x03
#define OP_		0x04
#define OP_ORA_ZP	0x05 // OR with accumulator and zero page (?), PC+=2; time 3
#define OP_ASL_ZP	0x06 // LSH with zero page (?), PC+=2; time 5
#define OP_		0x07
#define OP_PHP		0x08 // push reg P to stack, PC++; time 3
#define OP_ORA_IM	0x09 // immediate OR with accumulator, PC+=2; time 2
#define OP_ASL_AC	0x0A // LSH accumulator, PC++; time 2, IMPLEMENTED
#define OP_		0x0B 
#define OP_		0x0C
#define OP_ORA_AB	0x0D // TODO
#define OP_ASL_AB	0x0E
#define OP_		0x0F
#define OP_BPL		0x10 // branch on plus, PC+=2; if not taken time 2, if taken time 3, if taken and crosses page boundary time 4
#define OP_11		0x11
#define OP_12		0x12
#define OP_13		0x13
#define OP_14		0x14
#define OP_15		0x15
#define OP_16		0x16
#define OP_17		0x17
#define OP_18		0x18
#define OP_19		0x19
#define OP_1A		0x1A
#define OP_1B		0x1B
#define OP_1C		0x1C
#define OP_1D		0x1D
#define OP_1E		0x1E
#define OP_1F		0x1F
#define OP_20		0x20
#define OP_21		0x21
#define OP_22		0x22
#define OP_23		0x23
#define OP_24		0x24
#define OP_25		0x25
#define OP_26		0x26
#define OP_27		0x27
#define OP_28		0x28
#define OP_29		0x29
#define OP_2A		0x2A
#define OP_2B		0x2B
#define OP_2C		0x2C
#define OP_2D		0x2D
#define OP_2E		0x2E
#define OP_2F		0x2F
#define OP_30		0x30 
#define OP_31		0x31
#define OP_32		0x32
#define OP_33		0x33
#define OP_34		0x34
#define OP_35		0x35
#define OP_36		0x36
#define OP_37		0x37
#define OP_38		0x38
#define OP_39		0x39
#define OP_3A		0x3A
#define OP_3B		0x3B
#define OP_3C		0x3C
#define OP_3D		0x3D
#define OP_3E		0x3E
#define OP_3F		0x3F
#define OP_40		0x40
#define OP_41		0x41
#define OP_42		0x42
#define OP_43		0x43
#define OP_44		0x44
#define OP_45		0x45
#define OP_46		0x46
#define OP_47		0x47
#define OP_48		0x48
#define OP_49		0x49
#define OP_4A		0x4A
#define OP_4B		0x4B
#define OP_4C		0x4C
#define OP_4D		0x4D
#define OP_4E		0x4E
#define OP_4F		0x4F
#define OP_50		0x50
#define OP_51		0x51
#define OP_52		0x52
#define OP_53		0x53
#define OP_54		0x54
#define OP_55		0x55
#define OP_56		0x56
#define OP_57		0x57
#define OP_58		0x58
#define OP_59		0x59
#define OP_5A		0x5A
#define OP_5B		0x5B
#define OP_5C		0x5C
#define OP_5D		0x5D
#define OP_5E		0x5E
#define OP_5F		0x5F
#define OP_60		0x60
#define OP_61		0x61
#define OP_62		0x62
#define OP_63		0x63
#define OP_64		0x64
#define OP_65		0x65
#define OP_66		0x66
#define OP_67		0x67
#define OP_68		0x68
#define OP_69		0x69
#define OP_6A		0x6A
#define OP_6B		0x6B
#define OP_6C		0x6C
#define OP_6D		0x6D
#define OP_6E		0x6E
#define OP_6F		0x6F
#define OP_70		0x70
#define OP_71		0x71
#define OP_72		0x72
#define OP_73		0x73
#define OP_74		0x74
#define OP_75		0x75
#define OP_76		0x76
#define OP_77		0x77
#define OP_78		0x78
#define OP_79		0x79
#define OP_7A		0x7A
#define OP_7B		0x7B
#define OP_7C		0x7C
#define OP_7D		0x7D
#define OP_7E		0x7E
#define OP_7F		0x7F
#define OP_80		0x80
#define OP_81		0x81
#define OP_82		0x82
#define OP_83		0x83
#define OP_84		0x84
#define OP_85		0x85
#define OP_86		0x86
#define OP_87		0x87
#define OP_88		0x88
#define OP_89		0x89
#define OP_8A		0x8A
#define OP_8B		0x8B
#define OP_8C		0x8C
#define OP_8D		0x8D
#define OP_8E		0x8E
#define OP_8F		0x8F
#define OP_90		0x90
#define OP_91		0x91
#define OP_92		0x92
#define OP_93		0x93
#define OP_94		0x94
#define OP_95		0x95
#define OP_96		0x96
#define OP_97		0x97
#define OP_98		0x98
#define OP_99		0x99
#define OP_9A		0x9A
#define OP_9B		0x9B
#define OP_9C		0x9C
#define OP_9D		0x9D
#define OP_9E		0x9E
#define OP_9F		0x9F
#define OP_A0		0xA0
#define OP_A1		0xA1
#define OP_A2		0xA2
#define OP_A3		0xA3
#define OP_A4		0xA4
#define OP_A5		0xA5
#define OP_A6		0xA6
#define OP_A7		0xA7
#define OP_A8		0xA8
#define OP_A9		0xA9
#define OP_AA		0xAA
#define OP_AB		0xAB
#define OP_AC		0xAC
#define OP_AD		0xAD
#define OP_AE		0xAE
#define OP_AF		0xAF
#define OP_B0		0xB0
#define OP_B1		0xB1
#define OP_B2		0xB2
#define OP_B3		0xB3
#define OP_B4		0xB4
#define OP_B5		0xB5
#define OP_B6		0xB6
#define OP_B7		0xB7
#define OP_B8		0xB8
#define OP_B9		0xB9
#define OP_BA		0xBA
#define OP_BB		0xBB
#define OP_BC		0xBC
#define OP_BD		0xBD
#define OP_BE		0xBE
#define OP_BF		0xBF
#define OP_C0		0xC0
#define OP_C1		0xC1
#define OP_C2		0xC2
#define OP_C3		0xC3
#define OP_C4		0xC4
#define OP_C5		0xC5
#define OP_C6		0xC6
#define OP_C7		0xC7
#define OP_C8		0xC8
#define OP_C9		0xC9
#define OP_CA		0xCA
#define OP_CB		0xCB
#define OP_CC		0xCC
#define OP_CD		0xCD
#define OP_CE		0xCE
#define OP_CF		0xCF
#define OP_D0		0xD0
#define OP_D1		0xD1
#define OP_D2		0xD2
#define OP_D3		0xD3
#define OP_D4		0xD4
#define OP_D5		0xD5
#define OP_D6		0xD6
#define OP_D7		0xD7
#define OP_D8		0xD8
#define OP_D9		0xD9
#define OP_DA		0xDA
#define OP_DB		0xDB
#define OP_DC		0xDC
#define OP_DD		0xDD
#define OP_DE		0xDE
#define OP_DF		0xDF
#define OP_E0		0xE0
#define OP_E1		0xE1
#define OP_E2		0xE2
#define OP_E3		0xE3
#define OP_E4		0xE4
#define OP_E5		0xE5
#define OP_E6		0xE6
#define OP_E7		0xE7
#define OP_E8		0xE8
#define OP_E9		0xE9
#define OP_EA		0xEA
#define OP_EB		0xEB
#define OP_EC		0xEC
#define OP_ED		0xED
#define OP_EE		0xEE
#define OP_EF		0xEF
#define OP_D0		0xD0
#define OP_D1		0xD1
#define OP_D2		0xD2
#define OP_D3		0xD3
#define OP_D4		0xD4
#define OP_D5		0xD5
#define OP_D6		0xD6
#define OP_D7		0xD7
#define OP_D8		0xD8
#define OP_D9		0xD9
#define OP_DA		0xDA
#define OP_DB		0xDB
#define OP_DC		0xDC
#define OP_DD		0xDD
#define OP_DE		0xDE
#define OP_DF		0xDF
#define OP_E0		0xE0
#define OP_E1		0xE1
#define OP_E2		0xE2
#define OP_E3		0xE3
#define OP_E4		0xE4
#define OP_E5		0xE5
#define OP_E6		0xE6
#define OP_E7		0xE7
#define OP_E8		0xE8
#define OP_E9		0xE9
#define OP_EA		0xEA
#define OP_EB		0xEB
#define OP_EC		0xEC
#define OP_ED		0xED
#define OP_EE		0xEE
#define OP_EF		0xEF
#define OP_F0		0xF0
#define OP_F1		0xF1
#define OP_F2		0xF2
#define OP_F3		0xF3
#define OP_F4		0xF4
#define OP_F5		0xF5
#define OP_F6		0xF6
#define OP_F7		0xF7
#define OP_F8		0xF8
#define OP_F9		0xF9
#define OP_FA		0xFA
#define OP_FB		0xFB
#define OP_FC		0xFC
#define OP_FD		0xFD
#define OP_FE		0xFE
#define OP_FF		0xFF
*/
