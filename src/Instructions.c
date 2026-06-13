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
		case REG8_DEREF_HL:
			cpu->extraCycle++;
			MemoryBusWriteCPU(cpu->bus, cpu->hl, value);
			break;

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
		case REG8_DEREF_HL:
			cpu->extraCycle++;
			return MemoryBusReadCPU(cpu->bus, cpu->hl);
			break;

		default: return 0xFF;
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
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 7);
	val = (val << 1);
	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerShiftRight(CPU* cpu, RegisterID id, bool affectZFlag, bool isArithmetical) {
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 0);
	val = (val >> 1);
	val = AssignBit(val, 7, GetFlag(val, 6));
	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateLeft(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 7);
	bool currentCarryFlag = GetFlag(cpu->f, FLAG_CARRY);
	val = (val << 1) | (((currentCarryFlag) ? 1 : 0));

	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateRight(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 0);
	bool currentCarryFlag = GetFlag(cpu->f, FLAG_CARRY);
	val = (val >> 1) | (((currentCarryFlag) ? 1 : 0) << 7);

	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateLeftC(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 7);
	val = (val << 1) | (((carryFlag) ? 1 : 0));

	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

static void registerRotateRightC(CPU* cpu, RegisterID id, bool affectZFlag) {
	u8 val = readFromRegister8(cpu, id);
	bool carryFlag = GetFlag(val, 0);
	val = (val >> 1) | (((carryFlag) ? 1 : 0) << 7);

	writeToRegister8(cpu, val, id);
	if (affectZFlag) {
		CPUSetFlags(cpu, val == 0, false, false, carryFlag);
	} else {
		CPUSetFlags(cpu, false, false, false, carryFlag);
	}
}

void NOP([[maybe_unused]] CPU* cpu) {
	cpu->extraCycle = 1;
	return;
}

// no flags
void LDImm16Tor16(CPU* cpu, RegisterID regId, u16 value) {
	cpu->extraCycle = 3;
	writeToRegister16(cpu, value, regId, false);
}
void LDFromAToMem(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister8(cpu, REG8_A);
	writeToRegister16Memory(cpu, value, regId);
}
void LDfromMemToA(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister16Memory(cpu, regId);
	writeToRegister8(cpu, value, REG8_A);
}
void LDFromSPToMem(CPU* cpu, u16 value) {
	cpu->extraCycle = 5;
	MemoryBusWriteCPU(cpu->bus, value, GetLowByte(cpu->sp));
	MemoryBusWriteCPU(cpu->bus, value + 1, GetHighByte(cpu->sp));
}

// no flags
void INCr16(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	writeToRegister16(cpu, readFromRegister16(cpu, regId, false) + 1, regId, false);
}
void DECr16(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	writeToRegister16(cpu, readFromRegister16(cpu, regId, false) - 1, regId, false);
}
// H and C
// added flag support
void ADDr16Tohl(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	u16 currentValue = readFromRegister16(cpu, REG16_HL, false);
	u16 addend = readFromRegister16(cpu, regId, false);
	u32 sum = currentValue + addend;
	writeToRegister16(cpu, sum, REG16_HL, false);

	CPUSetFlags(cpu, GetFlag(cpu->f, FLAG_ZERO), false, (currentValue & 0x0FFF) + (addend & 0x0FFF) > 0x0FFF, sum > 0xFFFF);
}

// Z, N and H TODO: implement flag handling
void INCr8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;
	u8 regVal = readFromRegister8(cpu, regId);
	u8 val = regVal + 1;
	writeToRegister8(cpu, val, regId);
	CPUSetFlags(cpu, val == 0, false, GetLowNybble(regVal) + 1 > 0xF, GetFlag(cpu->f, FLAG_CARRY));
}
void DECr8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;
	u8 regVal = readFromRegister8(cpu, regId);
	u8 val = regVal - (u8)(1);
	writeToRegister8(cpu, val, regId);
	CPUSetFlags(cpu, val == 0, true, 1 > GetLowNybble(regVal), GetFlag(cpu->f, FLAG_CARRY));
}

// no flags
void LDImm8Tor8(CPU* cpu, RegisterID regId, u8 value) {
	cpu->extraCycle = 2;
	writeToRegister8(cpu, value, regId);
}

// C is touched, Z N and H are cleared
void RLCA(CPU* cpu) {
	cpu->extraCycle = 1;
	registerRotateLeftC(cpu, REG8_A, false);
}
// C is touched, Z N and H are cleared
void RRCA(CPU* cpu) {
	cpu->extraCycle = 1;
	registerRotateLeftC(cpu, REG8_A, false);
}
// flags already handled by static funcs
void RLA(CPU* cpu) {
	cpu->extraCycle = 1;
	registerRotateLeft(cpu, REG8_A, false);
}
// flags already handled by static funcs
void RRA(CPU* cpu) {
	cpu->extraCycle = 1;
	registerRotateRight(cpu, REG8_A, false);
}
// fixed DAA, flags are now correct
void DAA(CPU* cpu) {
	cpu->extraCycle = 1;
	bool nFlag = GetFlag(cpu->f, FLAG_SUBTRACT);
	bool carry = false;

	u8 value = cpu->a;
	if (nFlag) {
		if (GetFlag(cpu->f, FLAG_HALFCARRY)) value -= 0x06;
		if (GetFlag(cpu->f, FLAG_CARRY)) value -= 0x60;
	} else {
		if (GetFlag(cpu->f, FLAG_HALFCARRY) || ((cpu->a & 0xF) > 0x9)) value += 0x06;
		if (GetFlag(cpu->f, FLAG_CARRY) || (cpu->a > 0x99)) {
			value += 0x60;
			carry = true;
		}
	}
	cpu->a = value;
	// carry flag not properly implemented probably
	CPUSetFlags(cpu, cpu->a == 0, nFlag, false, carry);
}
// N and H set, rest is unchanged
void CPL(CPU* cpu) {
	cpu->extraCycle = 1;
	cpu->a = ~(cpu->a);
	bool cFlag = GetFlag(cpu->f, FLAG_CARRY);
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	CPUSetFlags(cpu, zFlag, true, true, cFlag);
}
// C set, N and H cleared
void SCF(CPU* cpu) {
	cpu->extraCycle = 1;
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	CPUSetFlags(cpu, zFlag, false, false, true);
}
// C = !C, N and H cleared
void CCF(CPU* cpu) {
	cpu->extraCycle = 1;
	bool zFlag = GetFlag(cpu->f, FLAG_ZERO);
	bool cFlag = GetFlag(cpu->f, FLAG_CARRY);
	CPUSetFlags(cpu, zFlag, false, false, !cFlag);
}

void JRImm8(CPU* cpu, s8 value) {
	cpu->extraCycle = 3;
	u16 address = (u16)((cpu->pc + (value)) + 2);
	cpu->pc = address;
	cpu->instructionByteAdvance = 0;
}
void JRCondImm8(CPU* cpu, Condition cond, u8 value) {
	cpu->extraCycle = 2;
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
	cpu->extraCycle = 3;
}

void STOP(CPU* cpu) {
	cpu->extraCycle = 1;
	cpu->veryLowPower = true;
}

// works with everything but ld [hl], [hl] (which encodes STOP)
void LDr8Tor8(CPU* cpu, RegisterID dest, RegisterID src) {
	cpu->extraCycle = 1;
	u8 val = readFromRegister8(cpu, src);
	writeToRegister8(cpu, val, dest);
}
void HALT(CPU* cpu) {
	cpu->extraCycle = 1;
	cpu->lowPower = true;
}

void ADDreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;

	u8 currentValue = cpu->a;
	u8 addend = readFromRegister8(cpu, regId);
	u16 sum = currentValue + addend;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, GetLowNybble(currentValue) + GetLowNybble(addend) > 0xF, sum > 0xFF);
}
void ADCreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;

	u8 currentValue = cpu->a;
	u8 addend = readFromRegister8(cpu, regId);
	u8 carry = GetFlag(cpu->f, FLAG_CARRY) ? 1 : 0;
	u16 sum = currentValue + addend + carry;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, GetLowNybble(currentValue) + GetLowNybble(addend) + carry > 0xF, sum > 0xFF);
}
void SUBreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;

	u8 currentValue = cpu->a;
	u8 subtrahend = readFromRegister8(cpu, regId);
	s16 sum = cpu->a - subtrahend;

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, GetLowNybble(subtrahend) > GetLowNybble(currentValue), subtrahend > currentValue);
}
void SBCreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;

	u8 currentValue = cpu->a;
	u8 subtrahend = readFromRegister8(cpu, regId);
	u8 carry = GetFlag(cpu->f, FLAG_CARRY) ? 1 : 0;
	s16 sum = cpu->a - (subtrahend + carry);

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, GetLowNybble(subtrahend) + carry > GetLowNybble(currentValue),
				subtrahend + carry > currentValue);
}

void ANDreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;
	cpu->a = cpu->a & readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void XORreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;
	cpu->a = cpu->a ^ readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, false, false);
}
void ORreg8(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 1;
	cpu->a = cpu->a | readFromRegister8(cpu, regId);
	CPUSetFlags(cpu, cpu->a == 0, false, false, false);
}
void CPreg8(CPU* cpu, RegisterID regId) {
	u8 a = cpu->a;
	SUBreg8(cpu, regId);
	cpu->extraCycle = 1;
	cpu->a = a;
}

void ADDimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;

	u8 currentValue = cpu->a;
	u8 addend = value;
	u16 sum = currentValue + addend;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, GetLowNybble(currentValue) + GetLowNybble(addend) > 0xF, sum > 0xFF);
}
void ADCimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;

	u8 currentValue = cpu->a;
	u8 addend = value;
	u8 carry = GetFlag(cpu->f, FLAG_CARRY) ? 1 : 0;
	u16 sum = currentValue + addend + carry;
	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, false, GetLowNybble(currentValue) + GetLowNybble(addend) + carry > 0xF, sum > 0xFF);
}
// not sure how to handle half carry
void SUBimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;

	u8 currentValue = cpu->a;
	u8 subtrahend = value;
	s16 sum = cpu->a - subtrahend;

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, GetLowNybble(subtrahend) > GetLowNybble(currentValue), subtrahend > currentValue);
}
void SBCimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;

	u8 currentValue = cpu->a;
	u8 subtrahend = value;
	u8 carry = GetFlag(cpu->f, FLAG_CARRY) ? 1 : 0;
	s16 sum = cpu->a - (subtrahend + carry);

	cpu->a = GetLowByte(sum);

	CPUSetFlags(cpu, GetLowByte(sum) == 0, true, GetLowNybble(subtrahend) + carry > GetLowNybble(currentValue),
				subtrahend + carry > currentValue);
}
void ANDimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;
	cpu->a = cpu->a & value;
	CPUSetFlags(cpu, cpu->a == 0, false, true, false);
}
void XORimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;
	cpu->a = cpu->a ^ value;
	CPUSetFlags(cpu, cpu->a == 0, false, false, false);
}
void ORimm8(CPU* cpu, u8 value) {
	cpu->extraCycle = 2;
	cpu->a = cpu->a | value;
	CPUSetFlags(cpu, cpu->a == 0, false, false, false);
}
void CPimm8(CPU* cpu, u8 value) {
	u8 a = cpu->a;
	SUBimm8(cpu, value);
	cpu->extraCycle = 1;
	cpu->a = a;
}

void RETcond(CPU* cpu, Condition cond) {
	cpu->extraCycle = 2;
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
	cpu->extraCycle = 5;
}
void RET(CPU* cpu) {
	cpu->extraCycle = 4;
	cpu->pc = CPUPop16(cpu);
	cpu->instructionByteAdvance = 0;
}
void RETI(CPU* cpu) {
	EI(cpu);
	RET(cpu);
	cpu->extraCycle = 4;
}
void JPcondImm16(CPU* cpu, Condition cond, u16 value) {
	cpu->extraCycle = 3;
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
	JPImm16(cpu, value);
	cpu->extraCycle = 4;
}
void JPImm16(CPU* cpu, u16 value) {
	cpu->extraCycle = 4;
	cpu->pc = value;
	cpu->instructionByteAdvance = 0;
}
void JPhl(CPU* cpu) {
	cpu->extraCycle = 1;
	JPImm16(cpu, cpu->hl);
}
void CALLcondImm16(CPU* cpu, Condition cond, u16 value) {
	cpu->extraCycle = 3;
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
	cpu->extraCycle = 6;
}
void CALLImm16(CPU* cpu, u16 value) {
	// instruction is 3 byte long, so skip those 3 bytes, because you want to return to the instruction after this one
	CPUPush16(cpu, cpu->pc + 3);
	JPImm16(cpu, value);
	cpu->extraCycle = 6;
	cpu->instructionByteAdvance = 0;
}
void RST(CPU* cpu, u8 value) {
	CPUPush16(cpu, cpu->pc + 1);
	JPImm16(cpu, value * 8);
	cpu->extraCycle = 4;
	cpu->instructionByteAdvance = 0;
}

void POPreg16Stk(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 3;
	writeToRegister16(cpu, CPUPop16(cpu), regId, true);
}
void PUSHreg16Stk(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 4;
	CPUPush16(cpu, readFromRegister16(cpu, regId, true));
}

// all of those instructions are prefixed
void LDHDerefCToa(CPU* cpu) {
	cpu->extraCycle = 2;
	cpu->a = MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, cpu->c));
	// printf("LDH A, [C] called, C = %02X, [C] = %02X\n", cpu->c, MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, cpu->c)));
}
void LDHDerefImm8Toa(CPU* cpu, u8 value) {
	cpu->extraCycle = 3;
	cpu->a = MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, value));
	// printf("LDH A, [n8] called, n8 = %02X, [n8] = %02X\n", value, MemoryBusReadCPU(cpu->bus, BuildU16(0xFF, value)));
}
void LDDerefImm16Toa(CPU* cpu, u16 value) {
	// printf("LD A, [a16] called\n");
	cpu->extraCycle = 4;
	cpu->a = MemoryBusReadCPU(cpu->bus, value);
}
void LDHaToDerefC(CPU* cpu) {
	// printf("LDH [C], A called\n");
	cpu->extraCycle = 2;
	MemoryBusWriteCPU(cpu->bus, BuildU16(0xFF, cpu->c), cpu->a);
}
void LDHaToDerefImm8(CPU* cpu, u8 value) {
	// printf("LDH [n8], A called\n");
	cpu->extraCycle = 3;
	MemoryBusWriteCPU(cpu->bus, BuildU16(0xFF, value), cpu->a);
}
void LDaToDerefImm16(CPU* cpu, u16 value) {
	// printf("LD [a16], A called\n");
	cpu->extraCycle = 4;
	MemoryBusWriteCPU(cpu->bus, value, cpu->a);
}

// sure that flags are now correct
void ADDsp(CPU* cpu, s8 value) {
	cpu->extraCycle = 4;
	s16 sum = cpu->sp + value;
	u8 lowNybbleValue = ((cpu->sp & 0xF) + (value & 0xF));
	cpu->sp = sum;
	bool isHalfCarry = lowNybbleValue > 0xF;
	u16 carrySum = GetLowByte(cpu->sp) + value;
	CPUSetFlags(cpu, false, false, isHalfCarry, carrySum > 0xFF);
}
// same as above
void LDspPlusImm8ToHL(CPU* cpu, u8 value) {
	cpu->extraCycle = 3;
	s16 sum = cpu->sp + value;
	u8 lowNybbleValue = ((cpu->sp & 0xFF) + (value & 0xF));
	cpu->sp = sum;
	bool isHalfCarry = lowNybbleValue < 0 || lowNybbleValue > 0xFF;
	CPUSetFlags(cpu, false, false, isHalfCarry, value < 0 || value > 0xFFFF);
}
void LDhlToSP(CPU* cpu) {
	cpu->extraCycle = 2;
	cpu->sp = cpu->hl;
}

void DI(CPU* cpu) {
	cpu->extraCycle = 1;
	cpu->isInterruptEnabled = false;
}
void EI(CPU* cpu) {
	cpu->extraCycle = 1;
	cpu->imeInstructionTimer = 2;
}

void RLC(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerRotateLeftC(cpu, regId, true);
}
void RRC(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerRotateRightC(cpu, regId, true);
}
void RL(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerRotateLeft(cpu, regId, true);
}
void RR(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerRotateRight(cpu, regId, true);
}
void SLA(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerShiftLeft(cpu, regId, true);
}
void SRA(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerShiftRight(cpu, regId, true, true);
}
void SWAP(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister8(cpu, regId);
	u8 result = MakeByte(GetLowNybble(value), GetHighNybble(value));
	writeToRegister8(cpu, regId, result);

	CPUSetFlags(cpu, result == 0, false, false, false);
}
void SRL(CPU* cpu, RegisterID regId) {
	cpu->extraCycle = 2;
	registerShiftRight(cpu, regId, true, false);
}

void BIT(CPU* cpu, RegisterID regId, u8 bitIdx) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister8(cpu, regId);
	bool zFlag = !GetFlag(value, bitIdx);

	CPUSetFlags(cpu, zFlag, false, true, GetFlag(cpu->f, FLAG_CARRY));
}
void RES(CPU* cpu, RegisterID regId, u8 bitIdx) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister8(cpu, regId);
	value = ClearBit(value, bitIdx);
	writeToRegister8(cpu, regId, value);
}
void SET(CPU* cpu, RegisterID regId, u8 bitIdx) {
	cpu->extraCycle = 2;
	u8 value = readFromRegister8(cpu, regId);
	value = SetBit(value, bitIdx);
	writeToRegister8(cpu, regId, value);
}

void HardLock(CPU* cpu) { cpu->isHardLocked = true; }