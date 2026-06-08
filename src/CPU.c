#include "CPU.h"

#include <stdlib.h>

#include "Instructions.h"

CPU* CPUCreate(MemoryBus* bus) {
	CPU* cpu = malloc(sizeof(CPU));
	if (cpu == NULL) return NULL;

	cpu->bus = bus;

	cpu->pc = 0x0000;

	return cpu;
}
void CPUDestroy(CPU* cpu) { free(cpu); }

void CPUSetFlags(CPU* cpu, bool z, bool n, bool h, bool c) {
	cpu->f = AssignBit(cpu->f, FLAG_ZERO, z);
	cpu->f = AssignBit(cpu->f, FLAG_ZERO, n);
	cpu->f = AssignBit(cpu->f, FLAG_ZERO, h);
	cpu->f = AssignBit(cpu->f, FLAG_ZERO, c);
}

void CPUPush8(CPU* cpu, u8 value) {
	cpu->sp--;
	MemoryBusWriteCPU(cpu->bus, cpu->sp, value);
}
u8 CPUPop8(CPU* cpu) {
	u8 result = MemoryBusReadCPU(cpu->bus, cpu->sp);
	cpu->sp++;
	return result;
}

void CPUPush16(CPU* cpu, u16 value) {
	CPUPush8(cpu, GetHighByte(value));
	CPUPush8(cpu, GetLowByte(value));
}
u16 CPUPop16(CPU* cpu) {
	u8 lowByte = CPUPop8(cpu);
	u8 highByte = CPUPop8(cpu);
	return BuildU16(highByte, lowByte);
}

static RegisterID getOperandDestr8(CPU* cpu, u8 opcode) { return GetBits(opcode, 3, 3); }
static RegisterID getOperandSrcr8(CPU* cpu, u8 opcode) { return GetBits(opcode, 0, 3); }
static RegisterID getOperandr16(CPU* cpu, u8 opcode) { return GetBits(opcode, 4, 2); }
static Condition getOperandCond(CPU* cpu, u8 opcode) { return GetBits(opcode, 3, 2); }
static u8 getOperandTgtBitIdx(CPU* cpu, u8 opcode) { return GetBits(opcode, 3, 3); }

static u8 getImm8(CPU* cpu) { return MemoryBusReadCPU(cpu->bus, cpu->pc + 1); }
static u16 getImm16(CPU* cpu) { return BuildU16(MemoryBusReadCPU(cpu->bus, cpu->pc + 2), MemoryBusReadCPU(cpu->bus, cpu->pc + 1)); }

void CPURunInstruction(CPU* cpu) {
	if (cpu->bus->isBootRomLoaded && cpu->pc >= 0x100) cpu->bus->isBootRomLoaded = false;

	u8 opcode = MemoryBusReadCPU(cpu->bus, cpu->pc);

	u8 selector = GetBits(opcode, 6, 2);

	// different blocks
	switch (selector) {
		case 0:
			// block 0 instructions
			u8 subSelector = GetBits(opcode, 0, 4);
			// eliminating all the outliers first
			if (opcode == 0) {
				NOP(cpu);
			} else if (GetFlag(subSelector, 2)) {
				switch (GetBits(opcode, 0, 2)) {
					case 0b00: INCr8(cpu, getOperandDestr8(cpu, opcode)); break;
					case 0b01: DECr8(cpu, getOperandDestr8(cpu, opcode)); break;

					case 0b10: LDImm8Tor8(cpu, getOperandDestr8(cpu, opcode), getImm8(cpu)); break;
					default: break;
				}
			} else if (GetBits(opcode, 0, 3) == 0b111) {
				switch (opcode) {
					case 0b00000111: RLCA(cpu); break;
					case 0b00001111: RRCA(cpu); break;
					case 0b00010111: RLA(cpu); break;
					case 0b00011111: RRA(cpu); break;
					case 0b00100111: DAA(cpu); break;
					case 0b00101111: CPL(cpu); break;
					case 0b00110111: SCF(cpu); break;
					case 0b00111111: CCF(cpu); break;
					default: break;
				}
			} else if (opcode == 0b00010000) {
				STOP(cpu);
			} else if (opcode == 0b00011000) {
				JRImm8(cpu, getImm8(cpu));
			} else if (GetFlag(opcode, 5) && GetBits(opcode, 0, 3) == 0b000) {
				JRCondImm8(cpu, getOperandCond(cpu, opcode), getImm8(cpu));
			} else if (!GetFlag(subSelector, 2)) {
				switch (subSelector) {
					case 0b0000:

					case 0b0001: LDImm16Tor16(cpu, getOperandr16(cpu, opcode), getImm16(cpu)); break;
					case 0b0010: LDFromAToMem(cpu, getOperandr16(cpu, opcode)); break;
					case 0b1010: LDfromMemToA(cpu, getOperandr16(cpu, opcode)); break;
					case 0b1000: LDFromSPToMem(cpu, getImm16(cpu));

					default: break;
				}
			}
		case 1:
			if (opcode == 0b01110110) {
				HALT(cpu);
			} else {
				LDr8Tor8(cpu, getOperandDestr8(cpu, opcode), getOperandSrcr8(cpu, opcode));
			}
		case 2:
			RegisterID srcReg = getOperandSrcr8(cpu, opcode);
			switch (GetBits(opcode, 3, 3)) {
				case 0b000: ADDreg8(cpu, srcReg); break;
				case 0b001: ADCreg8(cpu, srcReg); break;
				case 0b010: SUBreg8(cpu, srcReg); break;
				case 0b011: SBCreg8(cpu, srcReg); break;
				case 0b100: ANDreg8(cpu, srcReg); break;
				case 0b101: XORreg8(cpu, srcReg); break;
				case 0b110: ORreg8(cpu, srcReg); break;
				case 0b111: CPreg8(cpu, srcReg); break;

				default: break;
			}
		case 3:

			if (opcode == 0xCB) {
				opcode = MemoryBusReadCPU(cpu->bus, cpu->pc + 1);
				RegisterID reg8 = getOperandSrcr8(cpu, opcode);
				if (GetBits(opcode, 6, 2) == 0b00) {
					switch (GetBits(opcode, 3, 3)) {
						case 0b000: RLC(cpu, reg8); break;
						case 0b001: RRC(cpu, reg8); break;
						case 0b010: RL(cpu, reg8); break;
						case 0b011: RR(cpu, reg8); break;
						case 0b100: SLA(cpu, reg8); break;
						case 0b101: SRA(cpu, reg8); break;
						case 0b110: SWAP(cpu, reg8); break;
						case 0b111: SRL(cpu, reg8); break;
						default: break;
					}
				} else {
					u8 bitIdx = getOperandTgtBitIdx(cpu, opcode);
					switch (GetBits(opcode, 6, 2)) {
						case 0b01: BIT(cpu, reg8, bitIdx); break;
						case 0b10: RES(cpu, reg8, bitIdx); break;
						case 0b11: SET(cpu, reg8, bitIdx); break;
						default: break;
					}
				}
			} else if (GetBits(opcode, 3, 3) == 0b110) {
				u8 imm8 = getImm8(cpu);
				switch (GetBits(opcode, 3, 3)) {
					case 0b000: ADDimm8(cpu, imm8); break;
					case 0b001: ADCimm8(cpu, imm8); break;
					case 0b010: SUBimm8(cpu, imm8); break;
					case 0b011: SBCimm8(cpu, imm8); break;

					case 0b100: ANDimm8(cpu, imm8); break;
					case 0b101: XORimm8(cpu, imm8); break;
					case 0b110: ORimm8(cpu, imm8); break;
					case 0b111: CPimm8(cpu, imm8); break;
				}
			} else if (GetBits(opcode, 0, 4) == 0b0001) {
				POPreg16Stk(cpu, getOperandr16(cpu, opcode));
			} else if (GetBits(opcode, 0, 4) == 0b0101) {
				PUSHreg16Stk(cpu, getOperandr16(cpu, opcode));
			} else {
				switch (opcode) {
					case 0b11001001: RET(cpu); break;
					case 0b11011001: RETI(cpu); break;
					case 0b11000011: JPImm16(cpu, getImm16(cpu)); break;
					case 0b11101001: JPhl(cpu); break;
					case 0b11001101: CALLImm16(cpu, getImm16(cpu)); break;

					case 0b11100010: LDHaToDerefC(cpu); break;
					case 0b11100000: LDHaToDerefImm8(cpu, getImm8(cpu)); break;
					case 0b11101010: LDaToDerefImm16(cpu, getImm16(cpu)); break;
					case 0b11110010: LDHDerefCToa(cpu); break;
					case 0b11110000: LDHDerefImm8Toa(cpu, getImm8(cpu)); break;
					case 0b11111010: LDDerefImm16Toa(cpu, getImm16(cpu)); break;

					case 0b11101000: ADDsp(cpu, getImm8(cpu)); break;
					case 0b11111000: LDspPlusImm8ToHL(cpu, getImm8(cpu)); break;
					case 0b11111001: LDhlToSP(cpu); break;

					case 0b11110011: DI(cpu); break;
					case 0b11111011: EI(cpu); break;

					default: {
						u8 idx = GetBits(opcode, 0, 3);
						switch (idx) {
							case 0b000: RETcond(cpu, getOperandCond(cpu, opcode));
							case 0b010: JPcondImm16(cpu, getOperandCond(cpu, opcode), getImm16(cpu));
							case 0b100: CALLcondImm16(cpu, getOperandCond(cpu, opcode), getImm16(cpu));
							case 0b111: RST(cpu, getOperandTgtBitIdx(cpu, opcode));

							default:
						}
					}
				}
			}
	}
}