#include "Instructions.h"

#include <stdlib.h>

// to rework and refactor, some flags aren't handled correctly, and the code is crappy

static void writeToRegister8(CPU* cpu, u8 value, RegisterID id) {
	switch (id) {
		case REG8_A: cpu->a = value; break;
		case REG8_B: cpu->b = value; break;
		case REG8_C: cpu->c = value; break;
		case REG8_D: cpu->d = value; break;
		case REG8_E: cpu->e = value; break;
		case REG8_H: cpu->h = value; break;
		case REG8_L: cpu->l = value; break;
		case REG8_DEREF_HL: MemoryBusWriteCPU(cpu->bus, cpu->hl, value); break;

		default: return;
	}
}

static u8 readFromRegister8(CPU* cpu, RegisterID id) {
	switch (id) {
		case REG8_A: return cpu->a;
		case REG8_B: return cpu->b;
		case REG8_C: return cpu->c;
		case REG8_D: return cpu->d;
		case REG8_E: return cpu->e;
		case REG8_H: return cpu->h;
		case REG8_L: return cpu->l;
		case REG8_DEREF_HL: return MemoryBusReadCPU(cpu->bus, cpu->hl); break;

		default: return 0x00;
	}
}

static void writeToRegister16(CPU* cpu, u16 value, RegisterID id, bool isStackOperation) {
	switch (id) {
		case REG16_BC: cpu->bc = value; break;
		case REG16_DE: cpu->de = value; break;
		case REG16_HL: cpu->hl = value; break;
		case REG16_SP:
			if (isStackOperation)
				cpu->af = value;
			else
				cpu->sp = value;
			break;

		default: return;
	}
}

static u16 readFromRegister16(CPU* cpu, RegisterID id, bool isStackOperation) {
	switch (id) {
		case REG16_BC: return cpu->bc; break;
		case REG16_DE: return cpu->de; break;
		case REG16_HL: return cpu->hl; break;
		case REG16_SP: return (isStackOperation) ? cpu->af : cpu->sp; break;

		default: return 0x0000;
	}
}

static void writeToRegister16Memory(CPU* cpu, u16 value, RegisterID id) {
	switch (id) {
		case REG16MEM_BC: MemoryBusWriteCPU(cpu->bus, cpu->bc, value); break;
		case REG16MEM_DE: MemoryBusWriteCPU(cpu->bus, cpu->de, value); break;
		case REG16MEM_HL_INC: MemoryBusWriteCPU(cpu->bus, cpu->hl++, value); break;
		case REG16MEM_HL_DEC: MemoryBusWriteCPU(cpu->bus, cpu->hl--, value); break;

		default: return;
	}
}

static u8 readFromRegister16Memory(CPU* cpu, RegisterID id) {
	switch (id) {
		case REG16MEM_BC: return MemoryBusReadCPU(cpu->bus, cpu->bc);
		case REG16MEM_DE: return MemoryBusReadCPU(cpu->bus, cpu->de);
		case REG16MEM_HL_INC: return MemoryBusReadCPU(cpu->bus, cpu->hl++);
		case REG16MEM_HL_DEC: return MemoryBusReadCPU(cpu->bus, cpu->hl--);

		default: return 0;
	}
}

static void registerShiftLeft(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 7);
			val = (val << 1);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 7);
	val = (val << 1);
	MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerShiftRight(CPU* cpu, RegisterID id, bool affectZFlag, bool isArithmetical) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 0);
			val = (val >> 1) | (GetBit(cpu->f, FLAG_CARRY) << 7);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 0);
	val = (val >> 1);
	if (isArithmetical) val = SetBit(GetBit(val, 6), 7);
	MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateLeft(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 7);
			val = (val << 1) | GetBit(cpu->f, FLAG_CARRY);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 7);
	val = (val << 1) | GetBit(cpu->f, FLAG_CARRY);
	MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
	*reg = val;
}

static void registerRotateRight(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 0);
			val = (val >> 1) | (GetBit(cpu->f, FLAG_CARRY) << 7);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 0);
	val = (val >> 1) | (GetBit(cpu->f, FLAG_CARRY) << 7);
	MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateLeftC(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 7);
			val = (val << 1) | (carryFlag ? 1 : 0);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 7);
	val = (val << 1) | ((carryFlag) ? 1 : 0);

	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
	*reg = val;
}

static void registerRotateRightC(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8* reg;
	bool carryFlag;
	switch (id) {
		case REG8_A: reg = &(cpu->a); break;
		case REG8_B: reg = &(cpu->b); break;
		case REG8_C: reg = &(cpu->c); break;
		case REG8_D: reg = &(cpu->d); break;
		case REG8_E: reg = &(cpu->e); break;
		case REG8_H: reg = &(cpu->h); break;
		case REG8_L: reg = &(cpu->l); break;

		default:
			u8 val = MemoryBusReadCPU(cpu->bus, cpu->hl);
			carryFlag = GetFlag(val, 7);
			val = (val >> 1) | ((carryFlag ? 1 : 0) << 7);
			MemoryBusWriteCPU(cpu->bus, cpu->hl, val);
			if (affectZFlag) {
				CPUSetFlags(cpu, val == 0, false, false, carryFlag);
			} else {
				CPUSetFlags(cpu, false, false, false, carryFlag);
			}
			return;
	}

	u8 val = *reg;
	carryFlag = GetFlag(val, 7);
	val = (val >> 1) | (((carryFlag) ? 1 : 0) << 7);

	CPUSetFlags(cpu, false, false, false, carryFlag);
	cpu->a = (cpu->a << 1) | ((carryFlag) ? 1 : 0);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

void NOP([[maybe_unused]] CPU* cpu) { return; }

// no flags
void LDImm16Tor16(CPU* cpu, RegisterID regId, u16 value) { writeToRegister16(cpu, value, regId, false); }
void LDFromAToMem(CPU* cpu, RegisterID regId) {
	u8 value = readFromRegister8(cpu, REG8_A);
	writeToRegister16Memory(cpu, value, regId);
}
void LDfromMemToA(CPU* cpu, RegisterID regId) {
	u8 value = readFromRegister16Memory(cpu, regId);
	writeToRegister8(cpu, value, REG8_A);
}
void LDFromSPToMem(CPU* cpu, u16 value) {
	MemoryBusWriteCPU(cpu->bus, value, GetLowByte(cpu->sp));
	MemoryBusWriteCPU(cpu->bus, value + 1, GetHighByte(cpu->sp));
}

// no flags
void INCr16(CPU* cpu, RegisterID regId) { writeToRegister16(cpu, readFromRegister16(cpu, regId, false) + 1, regId, false); }
void DECr16(CPU* cpu, RegisterID regId) { writeToRegister16(cpu, readFromRegister16(cpu, regId, false) - 1, regId, false); }
// H and C
void ADDr16Tohl(CPU* cpu, RegisterID regId) {
	u16 value = readFromRegister16(cpu, regId, false) + readFromRegister16(cpu, REG16_HL, false);
	writeToRegister16(cpu, value, REG16_HL, false);
}

// Z, N and H TODO: implement flag handling
void INCr8(CPU* cpu, RegisterID regId) {
	u8 regVal = readFromRegister8(cpu, regId);
	u8 val = regVal + (u8)(1);
	bool halfCarry = val > 0xF;
	writeToRegister8(cpu, val, regId);
	CPUSetFlags(cpu, val == 0, true, halfCarry, GetFlag(cpu->f, FLAG_CARRY));
}
void DECr8(CPU* cpu, RegisterID regId) {
	u8 regVal = readFromRegister8(cpu, regId);
	u8 val = regVal - (u8)(1);
	bool halfCarry = ((regVal & 0xF) - (1)) < 0;
	writeToRegister8(cpu, val, regId);
	CPUSetFlags(cpu, val == 0, true, halfCarry, GetFlag(cpu->f, FLAG_CARRY));
}

// no flags
void LDImm8Tor8(CPU* cpu, RegisterID regId, u8 value) { writeToRegister8(cpu, value, regId); }

// C is touched, Z N and H are cleared
void RLCA(CPU* cpu) { registerRotateLeftC(cpu, REG8_A, false); }
// C is touched, Z N and H are cleared
void RRCA(CPU* cpu) { registerRotateLeftC(cpu, REG8_A, false); }
// flags already handled by static funcs
void RLA(CPU* cpu) { registerRotateLeft(cpu, REG8_A, false); }
// flags already handled by static funcs
void RRA(CPU* cpu) { registerRotateRight(cpu, REG8_A, false); }
// carry is weird on this one, the rest is correct, TODO: look into carry implem here
void DAA(CPU* cpu) {
	bool nFlag = GetFlag(cpu->f, FLAG_SUBTRACT);
	if (nFlag) {
		u8 adjustment = 0;
		if (GetFlag(cpu->f, FLAG_HALFCARRY)) adjustment += 0x06;
		if (GetFlag(cpu->f, FLAG_CARRY)) adjustment += 0x60;
		cpu->a -= adjustment;
	} else {
		u8 adjustment = 0;
		if (GetFlag(cpu->f, FLAG_HALFCARRY) || ((cpu->a & 0xF) > 0x9)) adjustment += 0x06;
		if (GetFlag(cpu->f, FLAG_CARRY) || (cpu->a > 0x99)) adjustment += 0x60;
		cpu->a += adjustment;
	}
	// carry flag not properly implemented probably
	CPUSetFlags(cpu, cpu->a == 0, nFlag, false, false);
}
// N and H set, rest is unchanged
void CPL(CPU* cpu) {
	cpu->a = ~(cpu->a);
	bool cFlag = GetFlag(cpu->f, FLAG_CARRY);
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	CPUSetFlags(cpu, zFlag, true, true, cFlag);
}
// C set, N and H cleared
void SCF(CPU* cpu) {
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	CPUSetFlags(cpu, zFlag, false, false, true);
}
// C = !C, N and H cleared
void CCF(CPU* cpu) {
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	bool cFlag = GetFlag(cpu->f, FLAG_CARRY);
	CPUSetFlags(cpu, zFlag, false, false, !cFlag);
}

void JRImm8(CPU* cpu, s8 value) {
	u16 address = (u16)((cpu->pc + (value)) + 2);
	cpu->pc = address;
	cpu->instructionByteAdvance = 0;
}
void JRCondImm8(CPU* cpu, Condition cond, u8 value) {
	switch (cond) {
		case CONDITION_Z:
			if (!GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_C:
			if (!GetFlag(cpu->f, FLAG_CARRY)) return;
			break;
		case CONDITION_NZ:
			if (GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_NC:
			if (GetFlag(cpu->f, FLAG_CARRY)) return;
			break;

		default: return;
	}

	JRImm8(cpu, value);
}

void STOP(CPU* cpu) { cpu->veryLowPower = true; }

// works with everything but ld [hl], [hl] (which encodes STOP)
void LDr8Tor8(CPU* cpu, RegisterID dest, RegisterID src) {
	u8* destReg = NULL;
	u8* srcReg = NULL;
	u8 tmp;
	bool writeToMem = false;

	switch (dest) {
		case REG8_A: destReg = &(cpu->a); break;
		case REG8_B: destReg = &(cpu->b); break;
		case REG8_C: destReg = &(cpu->c); break;
		case REG8_D: destReg = &(cpu->d); break;
		case REG8_E: destReg = &(cpu->e); break;
		case REG8_H: destReg = &(cpu->h); break;
		case REG8_L: destReg = &(cpu->l); break;
		case REG8_DEREF_HL: writeToMem = true;
		default: return;
	}

	switch (src) {
		case REG8_A: srcReg = &(cpu->a); break;
		case REG8_B: srcReg = &(cpu->b); break;
		case REG8_C: srcReg = &(cpu->c); break;
		case REG8_D: srcReg = &(cpu->d); break;
		case REG8_E: srcReg = &(cpu->e); break;
		case REG8_H: srcReg = &(cpu->h); break;
		case REG8_L: srcReg = &(cpu->l); break;
		case REG8_DEREF_HL: destReg = &tmp; break;
		default: return;
	}

	if (writeToMem) {
		MemoryBusWriteCPU(cpu->bus, cpu->hl, *srcReg);
	} else {
		if (srcReg != NULL && destReg != NULL) *destReg = *srcReg;
	}
}
void HALT(CPU* cpu) { cpu->lowPower = true; }

void ADDreg8(CPU* cpu, RegisterID regId) {
	u16 sum = 0;

	sum = cpu->a;
	sum += readFromRegister8(cpu, regId);
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, sum > 0xF, sum > 0xFF);
}
void ADCreg8(CPU* cpu, RegisterID regId) {
	u16 sum = 0;

	sum = cpu->a;
	sum += readFromRegister8(cpu, regId);
	if (GetFlag(cpu->f, FLAG_CARRY)) sum++;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, sum > 0xF, sum > 0xFF);
}
// not sure how to handle half carry
void SUBreg8(CPU* cpu, RegisterID regId) {
	s16 sum = 0;

	u8 value = readFromRegister8(cpu, regId);

	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) < 0;

	sum = cpu->a;
	sum -= value;

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, halfCarry, sum < 0);
}
void SBCreg8(CPU* cpu, RegisterID regId) {
	s16 sum = 0;

	u8 value = readFromRegister8(cpu, regId);

	bool carry = GetFlag(cpu->f, FLAG_CARRY);

	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) - ((carry) ? 1 : 0) < 0;

	sum = cpu->a;
	sum -= value;
	if (carry) sum--;

	carry = sum < 0;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, halfCarry, carry);
}

void ANDreg8(CPU* cpu, RegisterID regId) {
	cpu->a = cpu->a & readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void XORreg8(CPU* cpu, RegisterID regId) {
	cpu->a = cpu->a ^ readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void ORreg8(CPU* cpu, RegisterID regId) {
	cpu->a = cpu->a | readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void CPreg8(CPU* cpu, RegisterID regId) {
	u8 value = readFromRegister8(cpu, regId);
	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) < 0;
	CPUSetFlags(cpu, value == cpu->a, true, halfCarry, value > cpu->a);
}

void ADDimm8(CPU* cpu, u8 value) {
	u16 sum = 0;

	sum = cpu->a;
	sum += value;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, sum > 0xF, sum > 0xFF);
}
void ADCimm8(CPU* cpu, u8 value) {
	u16 sum = 0;

	sum = cpu->a;
	sum += value;
	if (GetFlag(cpu->f, FLAG_CARRY)) sum++;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, sum > 0xF, sum > 0xFF);
}
// not sure how to handle half carry
void SUBimm8(CPU* cpu, u8 value) {
	s16 sum = 0;

	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) < 0;

	sum = cpu->a;
	sum -= value;

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, halfCarry, sum < 0);
}
void SBCimm8(CPU* cpu, u8 value) {
	s16 sum = 0;

	bool carry = GetFlag(cpu->f, FLAG_CARRY);

	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) - ((carry) ? 1 : 0) < 0;

	sum = cpu->a;
	sum -= value;
	if (carry) sum--;

	carry = sum < 0;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, halfCarry, carry);
}
void ANDimm8(CPU* cpu, u8 value) {
	cpu->a = cpu->a & value;
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void XORimm8(CPU* cpu, u8 value) {
	cpu->a = cpu->a ^ value;
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void ORimm8(CPU* cpu, u8 value) {
	cpu->a = cpu->a | value;
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void CPimm8(CPU* cpu, u8 value) {
	bool halfCarry = ((cpu->a & 0xF) - (value & 0xF)) < 0;
	CPUSetFlags(cpu, value == cpu->a, true, halfCarry, value > cpu->a);
}

void RETcond(CPU* cpu, Condition cond) {
	switch (cond) {
		case CONDITION_Z:
			if (!GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_C:
			if (!GetFlag(cpu->f, FLAG_CARRY)) return;
			break;
		case CONDITION_NZ:
			if (GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_NC:
			if (GetFlag(cpu->f, FLAG_CARRY)) return;
			break;

		default: return;
	}

	RET(cpu);
}
void RET(CPU* cpu) {
	cpu->pc = CPUPop16(cpu);
	cpu->instructionByteAdvance = 0;
}
void RETI(CPU* cpu) {
	EI(cpu);
	RET(cpu);
}
void JPcondImm16(CPU* cpu, Condition cond, u16 value) {
	switch (cond) {
		case CONDITION_Z:
			if (!GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_C:
			if (!GetFlag(cpu->f, FLAG_CARRY)) return;
			break;
		case CONDITION_NZ:
			if (GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_NC:
			if (GetFlag(cpu->f, FLAG_CARRY)) return;
			break;

		default: return;
	}

	cpu->pc = value;
	cpu->instructionByteAdvance = 0;
}
void JPImm16(CPU* cpu, u16 value) {
	cpu->pc = value;
	cpu->instructionByteAdvance = 0;
}
void JPhl(CPU* cpu) { JPImm16(cpu, cpu->hl); }
void CALLcondImm16(CPU* cpu, Condition cond, u16 value) {
	switch (cond) {
		case CONDITION_Z:
			if (!GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_C:
			if (!GetFlag(cpu->f, FLAG_CARRY)) return;
			break;
		case CONDITION_NZ:
			if (GetFlag(cpu->f, FLAG_ZERO)) return;
			break;
		case CONDITION_NC:
			if (GetFlag(cpu->f, FLAG_CARRY)) return;
			break;

		default: return;
	}

	CALLImm16(cpu, value);
}
void CALLImm16(CPU* cpu, u16 value) {
	// instruction is 3 byte long, so skip those 3 bytes, because you want to return to the instruction after this one
	CPUPush16(cpu, cpu->pc + 3);
	JPImm16(cpu, value);
	cpu->instructionByteAdvance = 0;
}
void RST(CPU* cpu, u8 value) { CALLImm16(cpu, value * 8); }

void POPreg16Stk(CPU* cpu, RegisterID regId) { writeToRegister16(cpu, CPUPop16(cpu), regId, true); }
void PUSHreg16Stk(CPU* cpu, RegisterID regId) { CPUPush16(cpu, readFromRegister16(cpu, regId, true)); }

// all of those instructions are prefixed
void LDHDerefCToa(CPU* cpu) { cpu->a = MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, cpu->c)); }
void LDHDerefImm8Toa(CPU* cpu, u8 value) { cpu->a = MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, value)); }
void LDDerefImm16Toa(CPU* cpu, u16 value) { cpu->a = MemoryBusReadCPU(cpu->bus, value); }
void LDHaToDerefC(CPU* cpu) { MemoryBusWriteCPU(cpu->bus, BuildU16(0xFF, cpu->c), cpu->a); }
void LDHaToDerefImm8(CPU* cpu, u8 value) { MemoryBusWriteCPU(cpu->bus, BuildU16(0xFF, value), cpu->a); }
void LDaToDerefImm16(CPU* cpu, u16 value) { MemoryBusWriteCPU(cpu->bus, value, cpu->a); }

// unsure if carry and half carry are implemented correctly, to check
void ADDsp(CPU* cpu, s8 value) {
	s16 sum = cpu->sp + value;
	u8 lowNybbleValue = ((cpu->sp & 0xFF) + (value & 0xF));
	cpu->sp = sum;
	bool isHalfCarry = lowNybbleValue < 0 || lowNybbleValue > 0xFF;
	CPUSetFlags(cpu, false, false, isHalfCarry, value < 0 || value > 0xFFFF);
}
// same as above
void LDspPlusImm8ToHL(CPU* cpu, u8 value) {
	s16 sum = cpu->sp + value;
	u8 lowNybbleValue = ((cpu->sp & 0xFF) + (value & 0xF));
	cpu->sp = sum;
	bool isHalfCarry = lowNybbleValue < 0 || lowNybbleValue > 0xFF;
	CPUSetFlags(cpu, false, false, isHalfCarry, value < 0 || value > 0xFFFF);
}
void LDhlToSP(CPU* cpu) { cpu->sp = cpu->hl; }

void DI(CPU* cpu) { cpu->isInterruptEnabled = false; }
void EI(CPU* cpu) { cpu->isInterruptEnabled = true; }

void RLC(CPU* cpu, RegisterID regId) { registerRotateLeftC(cpu, regId, true); }
void RRC(CPU* cpu, RegisterID regId) { registerRotateRightC(cpu, regId, true); }
void RL(CPU* cpu, RegisterID regId) { registerRotateLeft(cpu, regId, true); }
void RR(CPU* cpu, RegisterID regId) { registerRotateRight(cpu, regId, true); }
void SLA(CPU* cpu, RegisterID regId) { registerShiftLeft(cpu, regId, true); }
void SRA(CPU* cpu, RegisterID regId) { registerShiftRight(cpu, regId, true, true); }
void SWAP(CPU* cpu, RegisterID regId) {
	u8 value = readFromRegister8(cpu, regId);
	u8 result = MakeByte(GetHighNybble(value), GetLowNybble(value));
	writeToRegister8(cpu, regId, result);

	CPUSetFlags(cpu, result == 0, false, false, false);
}
void SRL(CPU* cpu, RegisterID regId) { registerShiftRight(cpu, regId, true, false); }

void BIT(CPU* cpu, RegisterID regId, u8 bitIdx) {
	u8 value = readFromRegister8(cpu, regId);
	bool zFlag = !GetFlag(value, bitIdx);

	CPUSetFlags(cpu, zFlag, false, true, GetFlag(cpu->f, FLAG_CARRY));
}
void RES(CPU* cpu, RegisterID regId, u8 bitIdx) {
	u8 value = readFromRegister8(cpu, regId);
	value = SetBit(value, bitIdx);
	writeToRegister8(cpu, regId, value);
}
void SET(CPU* cpu, RegisterID regId, u8 bitIdx) {
	u8 value = readFromRegister8(cpu, regId);
	value = SetBit(value, bitIdx);
	writeToRegister8(cpu, regId, value);
}

void HardLock(CPU* cpu) { cpu->isHardLocked = true; }