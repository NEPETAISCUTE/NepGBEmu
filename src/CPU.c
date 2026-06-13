#include "CPU.h"

#include <stdlib.h>

#include "Instructions.h"

CPU* CPUCreate(MemoryBus* bus) {
	CPU* cpu = malloc(sizeof(CPU));
	if (cpu == NULL) return NULL;

	cpu->bus = bus;

	cpu->pc = 0x0000;

	cpu->f = 0x00;
	cpu->imeInstructionTimer = 0;
	cpu->lowPower = false;
	cpu->veryLowPower = false;

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

size_t CPUDebugPrintRegister8(RegisterID regSel) {
	switch (regSel) {
		case REG8_A: printf("a"); return 1;
		case REG8_B: printf("b"); return 1;
		case REG8_C: printf("c"); return 1;
		case REG8_D: printf("d"); return 1;
		case REG8_E: printf("e"); return 1;
		case REG8_H: printf("h"); return 1;
		case REG8_L: printf("l"); return 1;
		case REG8_DEREF_HL: printf("[hl]"); return 4;
		default: return 0;
	}
}

size_t CPUDebugPrintRegister16(RegisterID regSel) {
	switch (regSel) {
		case REG16_BC: printf("bc"); return 2;
		case REG16_DE: printf("de"); return 2;
		case REG16_HL: printf("hl"); return 2;
		case REG16_SP: printf("sp"); return 2;
		default: return 0;
	}
}

size_t CPUDebugPrintRegister16STK(RegisterID regSel) {
	switch (regSel) {
		case REG16STK_BC: printf("bc"); return 2;
		case REG16STK_DE: printf("de"); return 2;
		case REG16STK_HL: printf("hl"); return 2;
		case REG16STK_AF: printf("af"); return 2;
		default: return 0;
	}
}

size_t CPUDebugPrintRegister16Mem(RegisterID regSel) {
	switch (regSel) {
		case REG16MEM_BC: printf("[bc]"); return 4;
		case REG16MEM_DE: printf("[de]"); return 4;
		case REG16MEM_HL_INC: printf("[hl+]"); return 5;
		case REG16MEM_HL_DEC: printf("[hl-]"); return 5;
		default: return 0;
	}
}

size_t CPUDebugPrintCond(Condition cond) {
	switch (cond) {
		case CONDITION_C: printf(" c, "); return 4;
		case CONDITION_Z: printf(" z, "); return 4;
		case CONDITION_NC: printf(" nc, "); return 5;
		case CONDITION_NZ: printf(" nz, "); return 5;
		default: return 0;
	}
}

void CPUDebugPrintInstruction(CPU* cpu, u8 opcode) {
	u8 selector = GetBits(opcode, 6, 2);

	size_t charCount = 0;

	// if (cpu->bus->isBootRomLoaded) printf("opcode = %02X\n", opcode);

	printf("ROM00:%04X\t", cpu->pc);

	// different blocks
	switch (selector) {
		case 0:
			// block 0 instructions
			u8 subSelector = GetBits(opcode, 0, 4);
			// eliminating all the outliers first
			if (opcode == 0) {
				printf("nop");
				charCount += 3;
			} else if (GetBits(opcode, 0, 3) == 0b111) {
				switch (opcode) {
					case 0b00000111:
						printf("rlca");
						charCount += 4;
						break;
					case 0b00001111:
						printf("rrca");
						charCount += 4;
						break;
					case 0b00010111:
						printf("rla");
						charCount += 3;
						break;
					case 0b00011111:
						printf("rra");
						charCount += 3;
						break;
					case 0b00100111:
						printf("daa");
						charCount += 3;
						break;
					case 0b00101111:
						printf("cpl");
						charCount += 3;
						break;
					case 0b00110111:
						printf("scf");
						charCount += 3;
						break;
					case 0b00111111:
						printf("ccf");
						charCount += 3;
						break;
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
						printf("inc ");
						charCount += 4;
						charCount += CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						break;
					case 0b101:
						printf("dec ");
						charCount += 4;
						charCount += CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						break;

					case 0b110:
						printf("ld ");
						charCount += 3;
						charCount += CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						printf(", $%02X", getImm8(cpu));
						charCount += 5;
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
				printf("stop");
				charCount += 4;
			} else if (opcode == 0b00011000) {
				printf("jr ");
				charCount += 3;
				printf("$%04X", cpu->pc + (s8)(getImm8(cpu) + 2));
				charCount += 5;
			} else if (GetFlag(opcode, 5) && GetBits(opcode, 0, 3) == 0b000) {
				printf("jr");
				charCount += 2;
				charCount += CPUDebugPrintCond(getOperandCond(cpu, opcode));
				printf("$%04X", cpu->pc + (s8)(getImm8(cpu) + 2));
				charCount += 5;
			} else if (!GetFlag(subSelector, 2)) {
				switch (subSelector) {
					case 0b0001:
						printf("ld ");
						charCount += 3;
						charCount += CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						printf(", $%04X", getImm16(cpu));
						charCount += 7;
						break;
					case 0b0010:
						printf("ld ");
						charCount += 3;
						charCount += CPUDebugPrintRegister16Mem(getOperandr16(cpu, opcode));
						printf(", a");
						charCount += 3;
						break;
					case 0b1010:
						printf("ld ");
						charCount += 3;
						printf("a, ");
						charCount += 3;
						charCount += CPUDebugPrintRegister16Mem(getOperandr16(cpu, opcode));
						break;
					case 0b1000:
						printf("ld ");
						charCount += 3;
						printf("[$%04X], sp", getImm16(cpu));
						charCount += 11;
						break;

					case 0b0011:
						printf("inc ");
						charCount += 4;
						charCount += CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						break;
					case 0b1011:
						printf("dec ");
						charCount += 4;
						charCount += CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
						break;
					case 0b1001:
						printf("add hl, ");
						charCount += 8;
						charCount += CPUDebugPrintRegister16(getOperandr16(cpu, opcode));
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
						printf("halt");
						charCount += 4;
					} else {
						printf("ld ");
						charCount += 3;
						charCount += CPUDebugPrintRegister8(getOperandDestr8(cpu, opcode));
						printf(", ");
						charCount += 2;
						charCount += CPUDebugPrintRegister8(getOperandSrcr8(cpu, opcode));
					}
					break;
				case 2:
					RegisterID srcReg = getOperandSrcr8(cpu, opcode);
					switch (GetBits(opcode, 3, 3)) {
						case 0b000:
							printf("add a, ");
							charCount += 7;
							break;
						case 0b001:
							printf("adc a, ");
							charCount += 7;
							break;
						case 0b010:
							printf("sub a, ");
							charCount += 7;
							break;
						case 0b011:
							printf("sbc a, ");
							charCount += 7;
							break;
						case 0b100:
							printf("and a, ");
							charCount += 7;
							break;
						case 0b101:
							printf("xor a, ");
							charCount += 7;
							break;
						case 0b110:
							printf("or a, ");
							charCount += 6;
							break;
						case 0b111:
							printf("cp a, ");
							charCount += 6;
							break;

						default:
							printf("\n");
							printf("subselector = %02X\n");
							printf("opcode = %02X\n", opcode);
							while (true);
							break;
					}
					charCount += CPUDebugPrintRegister8(srcReg);
					break;
				case 3:

					if (opcode == 0xCB) {
						opcode = MemoryBusReadCPU(cpu->bus, cpu->pc + 1);
						RegisterID reg8 = getOperandSrcr8(cpu, opcode);
						if (GetBits(opcode, 6, 2) == 0b00) {
							switch (GetBits(opcode, 3, 3)) {
								case 0b000:
									printf("rlc ");
									charCount += 4;
									break;
								case 0b001:
									printf("rrc ");
									charCount += 4;
									break;
								case 0b010:
									printf("rl ");
									charCount += 3;
									break;
								case 0b011:
									printf("rr ");
									charCount += 3;
									break;
								case 0b100:
									printf("sla ");
									charCount += 4;
									break;
								case 0b101:
									printf("rra ");
									charCount += 4;
									break;
								case 0b110:
									printf("swap ");
									charCount += 5;
									break;
								case 0b111:
									printf("srl ");
									charCount += 4;
									break;
								default:
									printf("\n");
									printf("subselector = %02X\n");
									printf("opcode = %02X\n", opcode);
									while (true);
									break;
							}
							charCount += CPUDebugPrintRegister8(reg8);
						} else {
							u8 bitIdx = getOperandTgtBitIdx(cpu, opcode);
							switch (GetBits(opcode, 6, 2)) {
								case 0b01:
									printf("bit ");
									charCount += 4;
									break;
								case 0b10:
									printf("res ");
									charCount += 4;
									break;
								case 0b11:
									printf("set ");
									charCount += 4;
									break;
								default:
									printf("\n");
									printf("subselector = %02X\n");
									printf("opcode = %02X\n", opcode);
									while (true);
									break;
							}
							printf("%u, ", bitIdx);
							charCount += 3;
							charCount += CPUDebugPrintRegister8(getOperandSrcr8(cpu, opcode));
						}
					} else if (GetBits(opcode, 0, 3) == 0b110) {
						u8 imm8 = getImm8(cpu);
						switch (GetBits(opcode, 3, 3)) {
							case 0b000:
								printf("add a, ");
								charCount += 7;
								break;
							case 0b001:
								printf("adc a, ");
								charCount += 7;
								break;
							case 0b010:
								printf("sub a, ");
								charCount += 7;
								break;
							case 0b011:
								printf("sbc a, ");
								charCount += 7;
								break;

							case 0b100:
								printf("and a, ");
								charCount += 7;
								break;
							case 0b101:
								printf("xor a, ");
								charCount += 7;
								break;
							case 0b110:
								printf("or a, ");
								charCount += 6;
								break;
							case 0b111:
								printf("cp a, ");
								charCount += 6;
								break;

							default:
								printf("\n");
								printf("subselector = %02X\n");
								printf("opcode = %02X\n", opcode);
								while (true);
								break;
						}
						printf("$%02X", imm8);
						charCount += 3;
					} else if (GetBits(opcode, 0, 4) == 0b0001) {
						printf("pop ");
						charCount += 4;
						charCount += CPUDebugPrintRegister16STK(getOperandr16(cpu, opcode));
					} else if (GetBits(opcode, 0, 4) == 0b0101) {
						printf("push ");
						charCount += 5;
						charCount += CPUDebugPrintRegister16STK(getOperandr16(cpu, opcode));
					} else {
						switch (opcode) {
							case 0b11001001:
								printf("ret");
								charCount += 3;
								break;
							case 0b11011001:
								printf("reti");
								charCount += 4;
								break;
							case 0b11000011:
								printf("jp $%04X", getImm16(cpu));
								charCount += 8;
								break;
							case 0b11101001:
								printf("jp hl");
								charCount += 5;
								break;
							case 0b11001101:
								printf("call $%04X", getImm16(cpu));
								charCount += 10;
								break;

							case 0b11100010:
								printf("ldh [c], a");
								charCount += 10;
								break;
							case 0b11100000:
								printf("ldh [$%04X], a", 0xFF00 + getImm8(cpu));
								charCount += 14;
								break;
							case 0b11101010:
								printf("ld [$%04X], a", getImm16(cpu));
								charCount += 13;
								break;
							case 0b11110010:
								printf("ldh a, [c]");
								charCount += 10;
								break;
							case 0b11110000:
								printf("ldh a, [$%04X]", 0xFF00 + getImm8(cpu));
								charCount += 14;
								break;
							case 0b11111010:
								printf("ld a, [$%04X]", getImm16(cpu));
								charCount += 13;
								break;

							case 0b11101000:
								printf("add sp, $%02X", getImm8(cpu));
								charCount += 11;
								break;
							case 0b11111000:
								printf("ld hl, sp + $%02X", getImm8(cpu));
								charCount += 15;
								break;
							case 0b11111001:
								printf("ld sp, hl");
								charCount += 9;
								break;

							case 0b11110011:
								printf("di");
								charCount += 2;
								break;
							case 0b11111011:
								printf("ei");
								charCount += 2;
								break;

							default: {
								u8 idx = GetBits(opcode, 0, 3);
								switch (idx) {
									case 0b000:
										printf("ret");
										charCount += 3;
										charCount += CPUDebugPrintCond(getOperandCond(cpu, opcode));
										break;
									case 0b010:
										printf("jp");
										charCount += 2;
										charCount += CPUDebugPrintCond(getOperandCond(cpu, opcode));
										printf(" $%04X", getImm16(cpu));
										charCount += 6;
										break;
									case 0b100:
										printf("call");
										charCount += 4;
										charCount += CPUDebugPrintCond(getOperandCond(cpu, opcode));
										printf(" $%04X", getImm16(cpu));
										charCount += 6;
										break;
									case 0b111:
										printf("rst $%04X", getOperandTgtBitIdx(cpu, opcode) * 8);
										charCount += 9;
										break;

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
	for (size_t i = 0; i < 32 - charCount; i++) {
		putchar(' ');
	}
}

void CPUDebugPrintState(CPU* cpu) {
	printf("BC=%04X ", cpu->bc);
	printf("DE=%04X ", cpu->de);
	printf("HL=%04X ", cpu->hl);
	printf("AF=%04X ", cpu->af);
	printf("SP=%04X ", cpu->sp);
	printf("PC=%04X\n", cpu->pc);
	/*printf("F = ");
	(GetFlag(cpu->f, FLAG_ZERO)) ? putchar('Z') : putchar('0');
	(GetFlag(cpu->f, FLAG_SUBTRACT)) ? putchar('N') : putchar('0');
	(GetFlag(cpu->f, FLAG_HALFCARRY)) ? putchar('H') : putchar('0');
	(GetFlag(cpu->f, FLAG_CARRY)) ? putchar('C') : putchar('0');
	putchar('\n');*/
	/*
	printf("LCDC = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFF40));
	printf("STAT = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFF41));
	printf("LY = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFF44));
	printf("DIV = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFF04));
	printf("IE = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFFFF));
	printf("IF = $%02X\n", MemoryBusReadCPU(cpu->bus, 0xFF0F));
	*/
}

void CPUHandleInterrupt(CPU* cpu) {
	u8 interruptFlags = MemoryBusReadCPU(cpu->bus, 0xFF0F);
	u8 interruptEnable = MemoryBusReadCPU(cpu->bus, 0xFFFF);
	for (size_t i = 0; i <= 4; i++) {
		if (GetFlag(interruptFlags, i) && GetFlag(interruptEnable, i)) {
			cpu->lowPower = false;
			// if IME = 0, just wake up the CPU, but no handling
			if (!cpu->isInterruptEnabled) return;

			// else gotta handle it
			interruptFlags = ClearBit(interruptFlags, i);
			MemoryBusWriteCPU(cpu->bus, 0xFF0F, interruptFlags);
			// printf("interrupt %d called\n", i);
			cpu->isInterruptEnabled = false;
			CPUPush16(cpu, cpu->pc);
			cpu->pc = 0x40 + i * 0x8;  // call the handler basically
			cpu->extraCycle = 2;
			return;
		}
	}
}

void CPURunInstruction(CPU* cpu) {
	if (cpu->bus->isBootRomLoaded && cpu->pc >= 0x100) cpu->bus->isBootRomLoaded = false;
	if (cpu->extraCycle > 0) {
		cpu->extraCycle--;
		cpu->cycle++;
		return;
	} else if (cpu->imeInstructionTimer > 0) {
		cpu->imeInstructionTimer--;
		if (cpu->imeInstructionTimer == 0) cpu->isInterruptEnabled = true;
	}

	CPUHandleInterrupt(cpu);
	if (cpu->lowPower) return;

	u8 opcode = MemoryBusReadCPU(cpu->bus, cpu->pc);

#ifdef DEBUG
	if (!cpu->bus->isBootRomLoaded || cpu->pc >= 0x100) {
		CPUDebugPrintInstruction(cpu, opcode);
		CPUDebugPrintState(cpu);
	}
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
	cpu->cycle++;
	cpu->extraCycle--;
}
