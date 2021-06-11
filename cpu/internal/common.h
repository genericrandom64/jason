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

uint16_t short2addr(uint8_t lo, uint8_t hi) {
	uint16_t addr = (uint16_t) ((hi << 8) | (uint16_t) lo);

	#ifdef NES
	// we have to mirror the wram and ppu registers, redirect reads
	// TODO optimize this, this is quite honestly shitty
	// wram mirrors
	while(addr > 0x7FF && addr < 0x2000) addr -= 0x800;
	// PPU reg mirrors
	while(addr > 0x2007 && addr < 0x4000) addr -= 8;
	#endif

	return addr;
}

void lax(uint8_t i) {
	A = i;
	X = i;
}

void or_com() {
	if((A & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	}
	chkzero()
}

void xor(uint8_t x) {
	// TODO is this correct?
	A ^= x;
	or_com();
}

void or(uint8_t o) {
	A |= o;
	or_com();
}

void adc8(uint8_t src, uint8_t *addreg) {
	// TODO sign this and set overflow accordingly
	// DO you sign this? i hate addition and subtraction now
	uint16_t res = 0;
	int cmpval = 0;
	int8_t res8 = 0;
	#ifndef NOBCD
	if((P & SET_P_DECIMAL) != 0)
	#endif
	{
		res = *addreg + memmap[PC+1];
		cmpval = *addreg + memmap[PC+1];
		res8 = res & 0b0000000011111111;
		if((res & 0b0000000100000000) != 0) {
			P |= SET_P_CARRY;
		} else {
			P &= MASK_P_CARRY;
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
			P |= SET_P_CARRY;
		} else {
			P &= MASK_P_CARRY;
		}
		res8 = int2bcd(res8);
	}
	#endif
	// TODO is the result stored to A?
	A = res8;
	chkzero()
	// TODO make sure cmp works
	if(cmpval != res8) P |= SET_P_OVERFLOW;
	// TODO THIS DOES NOT WORK. cannot believe i missed this because it breaks absolute address opcodes
	PC+=2;
}

void bit(uint8_t src) {
	if((src & 0b01000000) != 0) {
		P |= SET_P_OVERFLOW;
	} else {
		P &= MASK_P_OVERFLOW;
	} // bit 6

	if((src & 0b10000000) != 0) {
		P |= SET_P_NEGATIVE;
	} else {
		P &= MASK_P_NEGATIVE;
	} // bit 7
	// TODO is this ANDed correctly?
	// no? check
	if((src & A) == 0) {
		P |= SET_P_ZERO;
	} else {
		P &= MASK_P_ZERO;
	} // accumulator mask

}

void system_request(uint8_t caller, uint8_t data) {
	if(srqh == NULL) {
		printf("Unhandled System Request: 0x%X 0x%X\n", caller, data);
		// simulate jams
		if((memmap[PC] & 00001111) == 0x2) {
			// TODO we need a MCM
			// yknow that clip from like carnival night zone? well thats what we're doing
			printf("JAM! If there was a machine code monitor, it would be opened now.\n");
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

void branch() {
	ITC++;
	PC++;
	// TODO will this work on a page bound ( DO | 01)?
	if(new_page(PC-1, PC+(int8_t)memmap[PC])) ITC++;
	PC+=(int8_t)memmap[PC];
	PC++;
}

uint8_t zpz(uint8_t base, uint8_t index) {
	// base is a register, index is 00h-FFh which *presumably* overflows and isnt signed?
	// TODO that
	// TODO am i using it right?
	return base + index;
}

void lda(uint8_t dest) {
	A = dest;
	or_com();
}

void ldx(uint8_t dest) {
	X = dest;
	or_com();
}

void ldy(uint8_t dest) {
	Y = dest;
	or_com();
}