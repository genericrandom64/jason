#ifndef CPU_COMMON_H
#define CPU_COMMON_H

#define chkzero(cpu, V) if(V == 0) {cpu->P |= SET_P_ZERO;} else {cpu->P &= MASK_P_ZERO;}

#ifndef NOBCD
uint8_t bcd2int(uint8_t in) {
	uint8_t l = (in >> 4)*10, r = in & 0b00001111;
	return l + r;
}

uint8_t int2bcd(uint8_t in) {
	uint8_t l = ((in / 10) % 10) << 4, r = in % 10;
	return l | r;
}
#endif

void chkcarry(j65_t* cpu, uint8_t i) {
        if((i & 0b10000000) != 0) {
                cpu->P |= SET_P_CARRY;
        } else {
                cpu->P &= MASK_P_CARRY;
        }
}

void chknegative(j65_t* cpu, uint8_t i) {
        if((i & 0b10000000) != 0) {
                cpu->P |= SET_P_NEGATIVE;
        } else {
                cpu->P &= MASK_P_NEGATIVE;
        }
}

uint16_t short2addr(uint8_t lo, uint8_t hi) {
	uint16_t addr = (uint16_t) ((hi << 8) | (uint16_t) lo);
	return addr;
}

void lax(j65_t* cpu, uint8_t i) {
	cpu->A = i;
	cpu->X = i;
}

void or_com(j65_t* cpu, uint8_t i) {
	chknegative(cpu, i);
	chkzero(cpu, i);
}

void xor(j65_t* cpu, uint8_t x) {
	// TODO is this correct?
	cpu->A ^= x;
	or_com(cpu, cpu->A);
}

void or(j65_t* cpu, uint8_t o) {
	cpu->A |= o;
	or_com(cpu, cpu->A);
}
/*
void adc8(j65_t* cpu, uint8_t src, uint8_t *addreg) {
	// TODO sign this and set overflow accordingly
	// DO you sign this? i hate addition and subtraction now
	uint16_t res = 0;
	int cmpval = 0;
	int8_t res8 = 0;
	#ifndef NOBCD
	if((cpu->P & SET_P_DECIMAL) != 0)
	#endif
	{
		res = *addreg + cpu->memmap[cpu->PC+1];
		cmpval = *addreg + cpu->memmap[cpu->PC+1];
		res8 = res & 0b0000000011111111;
		if((res & 0b0000000100000000) != 0) {
			cpu->P |= SET_P_CARRY;
		} else {
			cpu->P &= MASK_P_CARRY;
		}
	}
	#ifndef NOBCD
	else {
		// TODO does bcd have sign?
		int8_t AB = bcd2int(*addreg), CB = bcd2int(memmap[PC+1]);
		res = AB + CB;
		cmpval = AB + CB;
		res8 = res & 0b0000000011111111;
		// TODO set carry correctly
		if((res & 0b0000000100000000) != 0) {
			cpu->P |= SET_P_CARRY;
		} else {
			cpu->P &= MASK_P_CARRY;
		}
		res8 = int2bcd(res8);
	}
	#endif
	// TODO is the result stored to A?
	cpu->A = res8;
	chkzero(cpu, cpu->A);
	// TODO make sure cmp works
	if(cmpval != res8) cpu->P |= SET_P_OVERFLOW;
	// TODO THIS DOES NOT WORK. cannot believe i missed this because it breaks absolute address opcodes
	cpu->PC+=2;
}
*/
void bit(j65_t* cpu, uint8_t src) {
	if((src & 0b01000000) != 0) {
		cpu->P |= SET_P_OVERFLOW;
	} else {
		cpu->P &= MASK_P_OVERFLOW;
	} // bit 6

	if((src & 0b10000000) != 0) {
		cpu->P |= SET_P_NEGATIVE;
	} else {
		cpu->P &= MASK_P_NEGATIVE;
	} // bit 7
	// TODO is this ANDed correctly?
	// no? check
	if((src & cpu->A) == 0) {
		cpu->P |= SET_P_ZERO;
	} else {
		cpu->P &= MASK_P_ZERO;
	} // accumulator mask

}

void system_request(j65_t* cpu, uint8_t caller, uint8_t data) {
	if(srqh == NULL) {
		#ifndef NOLIBC
		printf("Unhandled System Request: 0x%X 0x%X\n", caller, data);
		#endif
		// TODO give greater indication to program that processor has jammed
		// as it is, jams just don't change the PC, but in the future they should do more
		// simulate jams
		if((cpu->memmap[cpu->PC] & 0x0F) == 0x2) {
			// TODO we need a MCM
			// yknow that clip from like carnival night zone? well thats what we're doing
			#ifndef NOLIBC
			printf("JAM! If there was a machine code monitor, it would be opened now.\n");
			#endif
		}
	}
	else (*srqh)(caller, data);
}

void register_system_request(void *func) {
	srqh = func;
}

uint8_t new_page(int addr1, int addr2) {
	if(addr1/0x100 == addr2/0x100) return 1;
	return 0;
}

void branch(j65_t* cpu) {
	cpu->ITC++;
	cpu->PC++;
	// TODO will this work on a page bound ( DO | 01)?
	if(new_page(cpu->PC-1, cpu->PC+(int8_t)cpu->memmap[cpu->PC])) cpu->ITC++;
	cpu->PC+=(int8_t)cpu->memmap[cpu->PC];
	cpu->PC++;
}

uint8_t zpz(uint8_t base, uint8_t index) {
	// base is a register, index is 00h-FFh which *presumably* overflows and isnt signed?
	// TODO that
	// TODO am i using it right?
	return base + index;
}

void lda(j65_t* cpu, uint8_t dest) {
	cpu->A = dest;
	or_com(cpu, cpu->A);
}

void ldx(j65_t* cpu, uint8_t dest) {
	cpu->X = dest;
	or_com(cpu, cpu->X);
}

void ldy(j65_t* cpu, uint8_t dest) {
	cpu->Y = dest;
	or_com(cpu, cpu->Y);
}

void asl(j65_t* cpu, uint8_t i) {
	cpu->A = i;
	chkcarry(cpu, cpu->A);
	cpu->A<<=1;
	or_com(cpu, cpu->A);
}

void and(j65_t* cpu, uint8_t i) {
	cpu->A&=i;
	or_com(cpu, cpu->A);
}

// TODO add BCD
void sbc(j65_t* cpu, uint8_t i) {
	uint16_t carry = (cpu->P & MASK_P_CARRY) ? 0x00 : 0x01;
	uint16_t A = cpu->A;
	uint16_t result = A - i - (1 - carry); //TODO: Verify the carry handling
	if (result > 0xFF) {
		cpu->P |= SET_P_CARRY;	
	}
	chkzero(cpu, result);
	chknegative(cpu, result);
	cpu->A = (0xFF & result);
}

void adc(j65_t* cpu, uint8_t i) {
	uint16_t carry = (cpu->P & MASK_P_CARRY) ? 0x00 : 0x01;
	uint16_t A = cpu->A;
	uint16_t result = A + i + carry; //TODO: Verify the carry handling
	if (result > 0xFF) {
		cpu->P |= SET_P_CARRY;	
	}
	chkzero(cpu, result);
	chknegative(cpu, result);
	cpu->A = (0xFF & result);
}

void compare(j65_t* cpu, uint8_t i, uint8_t j) {
	uint8_t result = i - j;
	chknegative(cpu, result);
        if (i >= j) {
		cpu->P |= SET_P_CARRY;	
        }
	chkzero(cpu, result);
}

#define cmp(C, i) compare(C, C->A, i)
#define cpx(C, i) compare(C, C->X, i)
#define cpy(C, i) compare(C, C->Y, i)

#endif
