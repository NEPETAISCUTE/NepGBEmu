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
	cpu->f = AssignBit(cpu->f, FLAG_SUBTRACT, n);
	cpu->f = AssignBit(cpu->f, FLAG_HALFCARRY, h);
	cpu->f = AssignBit(cpu->f, FLAG_CARRY, c);
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

static u8 getImm8(CPU* cpu) {
	cpu->instructionByteAdvance++;
	return MemoryBusReadCPU(cpu->bus, cpu->pc + 1);
}
static u16 getImm16(CPU* cpu) {
	cpu->instructionByteAdvance += 2;
	return BuildU16(MemoryBusReadCPU(cpu->bus, cpu->pc + 2), MemoryBusReadCPU(cpu->bus, cpu->pc + 1));
}

void CPUDebugPrintRegister8(RegisterID regSel) {
	switch (regSel) {
		case REG8_A: printf("A"); return;
		case REG8_B: printf("B"); return;
		case REG8_C: printf("C"); return;
		case REG8_D: printf("D"); return;
		case REG8_E: printf("E"); return;
		case REG8_H: printf("H"); return;
		case REG8_L: printf("L"); return;
		case REG8_DEREF_HL: printf("[HL]"); return;
	}
}

void CPUDebugPrintRegister16(RegisterID regSel) {
	switch (regSel) {
		case REG16_BC: printf("BC"); return;
		case REG16_DE: printf("DE"); return;
		case REG16_HL: printf("HL"); return;
		case REG16_SP: printf("SP"); return;
	}
}

void CPUDebugPrintRegister16STK(RegisterID regSel) {
	switch (regSel) {
		case REG16STK_BC: printf("BC"); return;
		case REG16STK_DE: printf("DE"); return;
		case REG16STK_HL: printf("HL"); return;
		case REG16STK_AF: printf("AF"); return;
	}
}

void CPUDebugPrintRegister16Mem(RegisterID regSel) {
	switch (regSel) {
		case REG16MEM_BC: printf("[BC]"); return;
		case REG16MEM_DE: printf("[DE]"); return;
		case REG16MEM_HL_INC: printf("[HL+]"); return;
		case REG16MEM_HL_DEC: printf("[HL-]"); return;
	}
}

void CPUDebugPrintCond(Condition cond) {
	switch (cond) {
		case CONDITION_C: printf(",C "); break;
		case CONDITION_Z: printf(",Z "); break;
		case CONDITION_NC: printf(",NC "); break;
		case CONDITION_NZ: printf(",NZ "); break;
	}
}

void CPUDebugPrintInstruction(CPU* cpu, u8 opcode) {
	u8 selector = GetBits(opcode, 6, 2);

	if (cpu->bus->isBootRomLoaded) printf("opcode = %02X\n", opcode);

	// different blocks
	switch (selector) {
		case 0:
			// block 0 instructions
			u8 subSelector = GetBits(opcode, 0, 4);
			// eliminating all the outliers first
			if (opcode == 0) {
				printf("NOP\n");
			} else if (GetBits(opcode, 0, 3) == 0b111) {
				switch (opcode) {
					case 0b00000111: printf("RLCA\n"); break;
					case 0b00001111: printf("RRCA\n"); break;
					case 0b00010111: printf("RLA\n"); break;
					case 0b00011111: printf("RRA\n"); break;
					case 0b00100111: printf("DAA\n"); break;
					case 0b00101111: printf("CPL\n"); break;
					case 0b00110111: printf("SCF\n"); break;
					case 0b00111111: printf("CCF\n"); break;
					default:
						printf("\n");
						printf("subselector = %02X\n");
						printf("opcode = %02X\n", opcode);
						while (true);
						break;
				}
			} else if (GetFlag(subSelector, 2)) {
				switch (GetBits(subSelector, 0, 3)) {
					case 0b100:
						printf("INC ");
						CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						putchar('\n');
						break;
					case 0b101:
						printf("DEC ");
						CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						putchar('\n');
						break;

					case 0b110:
						printf("LD ");
						CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						printf(", $%02X", getImm8(cpu));
						putchar('\n');
						break;
					default:
						printf("\n");
						printf("subselector = %02X\n");
						printf("opcode = %02X\n", opcode);
						printf("3 first bits: %u\n", GetBits(opcode, 0, 3));
						while (true);
						break;
				}
			} else if (opcode == 0b00010000) {
				printf("STOP\n");
			} else if (opcode == 0b00011000) {
				printf("JR ");
				printf("%hhd\n", getImm8(cpu));
			} else if (GetFlag(opcode, 5) && GetBits(opcode, 0, 3) == 0b000) {
				printf("JR");
				CPUDebugPrintCond(getOperandCond(cpu, opcode));
				printf("%hhd\n", getImm8(cpu));
			} else if (!GetFlag(subSelector, 2)) {
				switch (subSelector) {
					case 0b0001:
						printf("LD ");
						CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						printf(", $%04X\n", getImm16(cpu));
						break;
					case 0b0010:
						printf("LD ");
						CPUDebugPrintRegister16Mem(getOperandr16(cpu, opcode));
						printf(", A\n");
						break;
					case 0b1010:
						printf("LD ");
						printf("A, ");
						CPUDebugPrintRegister16Mem(getOperandr16(cpu, opcode));
						putchar('\n');
						break;
					case 0b1000:
						printf("LD ");
						printf("[$%04X], SP\n", getImm16(cpu));
						break;

					case 0b0011:
						printf("INC ");
						CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						putchar('\n');
						break;
					case 0b1011:
						printf("DEC ");
						CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						putchar('\n');
						break;
					case 0b1001:
						printf("ADD HL, ");
						CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						putchar('\n');
						break;

					default:
						printf("\n");
						printf("subselector = %02X\n");
						printf("opcode = %02X\n", opcode);
						while (true);
						break;
				}
				break;
				case 1:
					if (opcode == 0b01110110) {
						printf("HALT\n");
					} else {
						printf("LD ");
						CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						printf(", ");
						CPUDebugPrintRegister8(getOperandSrcr8(cpu, opcode));
						putchar('\n');
					}
					break;
				case 2:
					RegisterID srcReg = getOperandSrcr8(cpu, opcode);
					switch (GetBits(opcode, 3, 3)) {
						case 0b000: printf("ADD A, "); break;
						case 0b001: printf("ADC A, "); break;
						case 0b010: printf("SUB A, "); break;
						case 0b011: printf("ABC A, "); break;
						case 0b100: printf("AND A, "); break;
						case 0b101: printf("XOR A, "); break;
						case 0b110: printf("OR A, "); break;
						case 0b111: printf("CP A, "); break;

						default:
							printf("\n");
							printf("subselector = %02X\n");
							printf("opcode = %02X\n", opcode);
							while (true);
							break;
					}
					CPUDebugPrintRegister8(srcReg);
					putchar('\n');
					break;
				case 3:

					if (opcode == 0xCB) {
						opcode = MemoryBusReadCPU(cpu->bus, cpu->pc + 1);
						RegisterID reg8 = getOperandSrcr8(cpu, opcode);
						if (GetBits(opcode, 6, 2) == 0b00) {
							switch (GetBits(opcode, 3, 3)) {
								case 0b000: printf("RLC "); break;
								case 0b001: printf("RRC "); break;
								case 0b010: printf("RL "); break;
								case 0b011: printf("RR "); break;
								case 0b100: printf("SLA "); break;
								case 0b101: printf("RRA "); break;
								case 0b110: printf("SWAP "); break;
								case 0b111: printf("SRL "); break;
								default:
									printf("\n");
									printf("subselector = %02X\n");
									printf("opcode = %02X\n", opcode);
									while (true);
									break;
							}
							CPUDebugPrintRegister8(reg8);
							putchar('\n');
						} else {
							u8 bitIdx = getOperandTgtBitIdx(cpu, opcode);
							switch (GetBits(opcode, 6, 2)) {
								case 0b01: printf("BIT "); break;
								case 0b10: printf("RES "); break;
								case 0b11: printf("SET "); break;
								default:
									printf("\n");
									printf("subselector = %02X\n");
									printf("opcode = %02X\n", opcode);
									while (true);
									break;
							}
							printf("%u, ", bitIdx);
							CPUDebugPrintRegister8(getOperandSrcr8(cpu, opcode));
							putchar('\n');
						}
					} else if (GetBits(opcode, 0, 3) == 0b110) {
						u8 imm8 = getImm8(cpu);
						switch (GetBits(opcode, 3, 3)) {
							case 0b000: printf("ADD "); break;
							case 0b001: printf("ADC "); break;
							case 0b010: printf("SUB "); break;
							case 0b011: printf("SBC "); break;

							case 0b100: printf("AND "); break;
							case 0b101: printf("XOR "); break;
							case 0b110: printf("OR "); break;
							case 0b111: printf("CP "); break;

							default:
								printf("\n");
								printf("subselector = %02X\n");
								printf("opcode = %02X\n", opcode);
								while (true);
								break;
						}
						printf("A, $%02X\n", imm8);
						return;
					} else if (GetBits(opcode, 0, 4) == 0b0001) {
						printf("POP ");
						CPUDebugPrintRegister16STK(getOperandr16(cpu, opcode));
						putchar('\n');
					} else if (GetBits(opcode, 0, 4) == 0b0101) {
						printf("PUSH ");
						CPUDebugPrintRegister16STK(getOperandr16(cpu, opcode));
						putchar('\n');
					} else {
						switch (opcode) {
							case 0b11001001: printf("RET\n"); break;
							case 0b11011001: printf("RETI\n"); break;
							case 0b11000011: printf("JP $%04X\n", getImm16(cpu)); break;
							case 0b11101001: printf("JP HL\n"); break;
							case 0b11001101: printf("CALL $%04X\n", getImm16(cpu)); break;

							case 0b11100010: printf("LDH [C], A\n"); break;
							case 0b11100000: printf("LDH [$%02X], A\n", getImm8(cpu)); break;
							case 0b11101010: printf("LD [$%04X], A\n", getImm16(cpu)); break;
							case 0b11110010: printf("LDH A, [C]\n"); break;
							case 0b11110000: printf("LDH A, [$%02X]\n", getImm8(cpu)); break;
							case 0b11111010: printf("LD A, [$%04X]\n", getImm16(cpu)); break;

							case 0b11101000: printf("ADD SP, $%02X\n", getImm8(cpu)); break;
							case 0b11111000: printf("LD HL, SP + $%02X\n", getImm8(cpu)); break;
							case 0b11111001: printf("LD SP, HL\n"); break;

							case 0b11110011: printf("DI\n"); break;
							case 0b11111011: printf("EI\n"); break;

							default: {
								u8 idx = GetBits(opcode, 0, 3);
								switch (idx) {
									case 0b000:
										printf("RET");
										CPUDebugPrintCond(getOperandCond(cpu, opcode));
										putchar('\n');
										break;
									case 0b010:
										printf("JP");
										CPUDebugPrintCond(getOperandCond(cpu, opcode));
										printf(" $%04X\n", getImm16(cpu));
										printf("opcode = %02X\n", opcode);
										while (true);
										break;
									case 0b100:
										printf("CALL");
										CPUDebugPrintCond(getOperandCond(cpu, opcode));
										printf(" $%04X\n", getImm16(cpu));
										break;
									case 0b111: printf("RST $%04X\n", getOperandTgtBitIdx(cpu, opcode) * 8); break;

									default:
										printf("\n");
										printf("subselector = %02X\n");
										printf("opcode = %02X\n", opcode);
										while (true);
										break;
								}
							}
						}
					}
			}
	}
	cpu->instructionByteAdvance = 1;
}

void CPUDebugPrintState(CPU* cpu) {
	printf("A = $%02X\n", cpu->a);
	printf("B = $%02X\n", cpu->b);
	printf("C = $%02X\n", cpu->c);
	printf("D = $%02X\n", cpu->d);
	printf("E = $%02X\n", cpu->e);
	printf("F = ");
	(GetFlag(cpu->f, FLAG_ZERO)) ? putchar('Z') : putchar('0');
	(GetFlag(cpu->f, FLAG_SUBTRACT)) ? putchar('N') : putchar('0');
	(GetFlag(cpu->f, FLAG_HALFCARRY)) ? putchar('H') : putchar('0');
	(GetFlag(cpu->f, FLAG_CARRY)) ? putchar('C') : putchar('0');
	putchar('\n');
	printf("HL = $%04X\n", cpu->hl);

	printf("SP = $%04X\n", cpu->sp);
	printf("PC = $%04X", cpu->pc);
	if (cpu->pc < 0x100 && cpu->bus->isBootRomLoaded)
		printf(" (BootROM)\n");
	else
		putchar('\n');
	putchar('\n');
}

void CPURunInstruction(CPU* cpu) {
	if (cpu->bus->isBootRomLoaded && cpu->pc >= 0x100) cpu->bus->isBootRomLoaded = false;

	u8 opcode = MemoryBusReadCPU(cpu->bus, cpu->pc);

#ifdef DEBUG
	CPUDebugPrintState(cpu);
	CPUDebugPrintInstruction(cpu, opcode);
#endif

	u8 selector = GetBits(opcode, 6, 2);

	cpu->instructionByteAdvance = 1;

	// different blocks
	switch (selector) {
		case 0:
			// block 0 instructions
			u8 subSelector = GetBits(opcode, 0, 4);
			// eliminating all the outliers first
			if (opcode == 0) {
				NOP(cpu);
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
			} else if (GetFlag(subSelector, 2)) {
				switch (GetBits(opcode, 0, 3)) {
					case 0b100: INCr8(cpu, getOperandDestr8(cpu, opcode)); break;
					case 0b101: DECr8(cpu, getOperandDestr8(cpu, opcode)); break;

					case 0b110: LDImm8Tor8(cpu, getOperandDestr8(cpu, opcode), getImm8(cpu)); break;
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

					case 0b0011: INCr16(cpu, getOperandr16(cpu, opcode)); break;
					case 0b1011: DECr16(cpu, getOperandr16(cpu, opcode)); break;
					case 0b1001: ADDr16Tohl(cpu, getOperandr16(cpu, opcode)); break;

					default: break;
				}
			}
			break;
		case 1:
			if (opcode == 0b01110110) {
				HALT(cpu);
			} else {
				LDr8Tor8(cpu, getOperandDestr8(cpu, opcode), getOperandSrcr8(cpu, opcode));
			}
			break;
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
			break;
		case 3:

			if (opcode == 0xCB) {
				cpu->instructionByteAdvance++;
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
			} else if (GetBits(opcode, 0, 3) == 0b110) {
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
							case 0b000: RETcond(cpu, getOperandCond(cpu, opcode)); break;
							case 0b010: JPcondImm16(cpu, getOperandCond(cpu, opcode), getImm16(cpu)); break;
							case 0b100: CALLcondImm16(cpu, getOperandCond(cpu, opcode), getImm16(cpu)); break;
							case 0b111: RST(cpu, getOperandTgtBitIdx(cpu, opcode)); break;

							default:
						}
					}
				}
			}
	}
	cpu->pc += cpu->instructionByteAdvance;
}
