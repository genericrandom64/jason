#include <stdint.h>
#include <string.h>

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
#define OP_		0x11
#define OP_		0x12
#define OP_		0x13
#define OP_		0x14
#define OP_		0x15
#define OP_		0x16
#define OP_		0x17
#define OP_		0x18
#define OP_		0x19
#define OP_		0x1A
#define OP_		0x1B
#define OP_		0x1C
#define OP_		0x1D
#define OP_		0x1E
#define OP_		0x1F
#define OP_		0x20
#define OP_		0x21
#define OP_		0x22
#define OP_		0x23
#define OP_		0x24
#define OP_		0x25
#define OP_		0x26
#define OP_		0x27
#define OP_		0x28
#define OP_		0x29
#define OP_		0x2A
#define OP_		0x2B
#define OP_		0x2C
#define OP_		0x2D
#define OP_		0x2E
#define OP_		0x2F
#define OP_		0x30 
#define OP_		0x31
#define OP_		0x32
#define OP_		0x33
#define OP_		0x34
#define OP_		0x35
#define OP_		0x36
#define OP_		0x37
#define OP_		0x38
#define OP_		0x39
#define OP_		0x3A
#define OP_		0x3B
#define OP_		0x3C
#define OP_		0x3D
#define OP_		0x3E
#define OP_		0x3F
#define OP_		0x40
#define OP_		0x41
#define OP_		0x42
#define OP_		0x43
#define OP_		0x44
#define OP_		0x45
#define OP_		0x46
#define OP_		0x47
#define OP_		0x48
#define OP_		0x49
#define OP_		0x4A
#define OP_		0x4B
#define OP_		0x4C
#define OP_		0x4D
#define OP_		0x4E
#define OP_		0x4F
#define OP_		0x50
#define OP_		0x51
#define OP_		0x52
#define OP_		0x53
#define OP_		0x54
#define OP_		0x55
#define OP_		0x56
#define OP_		0x57
#define OP_		0x58
#define OP_		0x59
#define OP_		0x5A
#define OP_		0x5B
#define OP_		0x5C
#define OP_		0x5D
#define OP_		0x5E
#define OP_		0x5F
#define OP_		0x60
#define OP_		0x61
#define OP_		0x62
#define OP_		0x63
#define OP_		0x64
#define OP_		0x65
#define OP_		0x66
#define OP_		0x67
#define OP_		0x68
#define OP_		0x69
#define OP_		0x6A
#define OP_		0x6B
#define OP_		0x6C
#define OP_		0x6D
#define OP_		0x6E
#define OP_		0x6F
#define OP_		0x70
#define OP_		0x71
#define OP_		0x72
#define OP_		0x73
#define OP_		0x74
#define OP_		0x75
#define OP_		0x76
#define OP_		0x77
#define OP_		0x78
#define OP_		0x79
#define OP_		0x7A
#define OP_		0x7B
#define OP_		0x7C
#define OP_		0x7D
#define OP_		0x7E
#define OP_		0x7F
#define OP_		0x80
#define OP_		0x81
#define OP_		0x82
#define OP_		0x83
#define OP_		0x84
#define OP_		0x85
#define OP_		0x86
#define OP_		0x87
#define OP_		0x88
#define OP_		0x89
#define OP_		0x8A
#define OP_		0x8B
#define OP_		0x8C
#define OP_		0x8D
#define OP_		0x8E
#define OP_		0x8F
#define OP_		0x90
#define OP_		0x91
#define OP_		0x92
#define OP_		0x93
#define OP_		0x94
#define OP_		0x95
#define OP_		0x96
#define OP_		0x97
#define OP_		0x98
#define OP_		0x99
#define OP_		0x9A
#define OP_		0x9B
#define OP_		0x9C
#define OP_		0x9D
#define OP_		0x9E
#define OP_		0x9F
#define OP_		0xA0
#define OP_		0xA1
#define OP_		0xA2
#define OP_		0xA3
#define OP_		0xA4
#define OP_		0xA5
#define OP_		0xA6
#define OP_		0xA7
#define OP_		0xA8
#define OP_		0xA9
#define OP_		0xAA
#define OP_		0xAB
#define OP_		0xAC
#define OP_		0xAD
#define OP_		0xAE
#define OP_		0xAF
#define OP_		0xB0
#define OP_		0xB1
#define OP_		0xB2
#define OP_		0xB3
#define OP_		0xB4
#define OP_		0xB5
#define OP_		0xB6
#define OP_		0xB7
#define OP_		0xB8
#define OP_		0xB9
#define OP_		0xBA
#define OP_		0xBB
#define OP_		0xBC
#define OP_		0xBD
#define OP_		0xBE
#define OP_		0xBF
#define OP_		0xC0
#define OP_		0xC1
#define OP_		0xC2
#define OP_		0xC3
#define OP_		0xC4
#define OP_		0xC5
#define OP_		0xC6
#define OP_		0xC7
#define OP_		0xC8
#define OP_		0xC9
#define OP_		0xCA
#define OP_		0xCB
#define OP_		0xCC
#define OP_		0xCD
#define OP_		0xCE
#define OP_		0xCF
#define OP_		0xD0
#define OP_		0xD1
#define OP_		0xD2
#define OP_		0xD3
#define OP_		0xD4
#define OP_		0xD5
#define OP_		0xD6
#define OP_		0xD7
#define OP_		0xD8
#define OP_		0xD9
#define OP_		0xDA
#define OP_		0xDB
#define OP_		0xDC
#define OP_		0xDD
#define OP_		0xDE
#define OP_		0xDF
#define OP_		0xE0
#define OP_		0xE1
#define OP_		0xE2
#define OP_		0xE3
#define OP_		0xE4
#define OP_		0xE5
#define OP_		0xE6
#define OP_		0xE7
#define OP_		0xE8
#define OP_		0xE9
#define OP_		0xEA
#define OP_		0xEB
#define OP_		0xEC
#define OP_		0xED
#define OP_		0xEE
#define OP_		0xEF
#define OP_		0xD0
#define OP_		0xD1
#define OP_		0xD2
#define OP_		0xD3
#define OP_		0xD4
#define OP_		0xD5
#define OP_		0xD6
#define OP_		0xD7
#define OP_		0xD8
#define OP_		0xD9
#define OP_		0xDA
#define OP_		0xDB
#define OP_		0xDC
#define OP_		0xDD
#define OP_		0xDE
#define OP_		0xDF
#define OP_		0xE0
#define OP_		0xE1
#define OP_		0xE2
#define OP_		0xE3
#define OP_		0xE4
#define OP_		0xE5
#define OP_		0xE6
#define OP_		0xE7
#define OP_		0xE8
#define OP_		0xE9
#define OP_		0xEA
#define OP_		0xEB
#define OP_		0xEC
#define OP_		0xED
#define OP_		0xEE
#define OP_		0xEF
#define OP_		0xF0
#define OP_		0xF1
#define OP_		0xF2
#define OP_		0xF3
#define OP_		0xF4
#define OP_		0xF5
#define OP_		0xF6
#define OP_		0xF7
#define OP_		0xF8
#define OP_		0xF9
#define OP_		0xFA
#define OP_		0xFB
#define OP_		0xFC
#define OP_		0xFD
#define OP_		0xFE
#define OP_		0xFF
