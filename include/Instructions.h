#ifndef NEPGB_INSTRUCTIONS_H
#define NEPGB_INSTRUCTIONS_H

#include "CPU.h"
#include "common.h"

typedef enum RegisterID {
	REG8_B = 0,
	REG8_C = 1,
	REG8_D = 2,
	REG8_E = 3,
	REG8_H = 4,
	REG8_L = 5,
	REG8_DEREF_HL = 6,
	REG8_A = 7,

	REG16_BC = 0,
	REG16_DE = 1,
	REG16_HL = 2,
	REG16_SP = 3,

	REG16STK_BC = 0,
	REG16STK_DE = 1,
	REG16STK_HL = 2,
	REG16STK_AF = 3,

	REG16MEM_BC = 0,
	REG16MEM_DE = 1,
	REG16MEM_HL_INC = 2,
	REG16MEM_HL_DEC = 3,

} RegisterID;

typedef enum RegisterMode {
	REGMODE_REG8 = 0,
	REGMODE_REG16 = 1,
	REGMODE_REG16STK = 2,
	REGMODE_REG16MEM = 3,
} RegisterMode;

typedef enum Condition {
	CONDITION_NZ = 0,
	CONDITION_Z = 1,
	CONDITION_NC = 2,
	CONDITION_C = 3,
} Condition;

// Block 0

void NOP([[maybe_unused]] CPU* cpu);

void LDImm16Tor16(CPU* cpu, RegisterID regId, u16 value);
void LDFromAToMem(CPU* cpu, RegisterID regId);
void LDfromMemToA(CPU* cpu, RegisterID regId);
void LDFromSPToMem(CPU* cpu, u16 value);

void INCr16(CPU* cpu, RegisterID regId);
void DECr16(CPU* cpu, RegisterID regId);
void ADDr16Tohl(CPU* cpu, RegisterID regId);

void INCr8(CPU* cpu, RegisterID regId);
void DECr8(CPU* cpu, RegisterID regId);

void LDImm8Tor8(CPU* cpu, RegisterID regId, u8 value);

void RLCA(CPU* cpu);
void RRCA(CPU* cpu);
void RLA(CPU* cpu);
void RRA(CPU* cpu);
void DAA(CPU* cpu);
void CPL(CPU* cpu);
void SCF(CPU* cpu);
void CCF(CPU* cpu);

void JRImm8(CPU* cpu, s8 value);
void JRCondImm8(CPU* cpu, Condition cond, u8 value);

void STOP(CPU* cpu);

// Block 1

void LDr8Tor8(CPU* cpu, RegisterID dest, RegisterID src);
void HALT(CPU* cpu);

// Block 2

void ADDreg8(CPU* cpu, RegisterID regId);
void ADCreg8(CPU* cpu, RegisterID regId);
void SUBreg8(CPU* cpu, RegisterID regId);
void SBCreg8(CPU* cpu, RegisterID regId);
void ANDreg8(CPU* cpu, RegisterID regId);
void XORreg8(CPU* cpu, RegisterID regId);
void ORreg8(CPU* cpu, RegisterID regId);
void CPreg8(CPU* cpu, RegisterID regId);

// Block 3

void ADDimm8(CPU* cpu, u8 value);
void ADCimm8(CPU* cpu, u8 value);
void SUBimm8(CPU* cpu, u8 value);
void SBCimm8(CPU* cpu, u8 value);
void ANDimm8(CPU* cpu, u8 value);
void XORimm8(CPU* cpu, u8 value);
void ORimm8(CPU* cpu, u8 value);
void CPimm8(CPU* cpu, u8 value);

void RETcond(CPU* cpu, Condition cond);
void RET(CPU* cpu);
void RETI(CPU* cpu);
void JPcondImm16(CPU* cpu, Condition cond, u16 value);
void JPImm16(CPU* cpu, u16 value);
void JPhl(CPU* cpu);
void CALLcondImm16(CPU* cpu, Condition cond, u16 value);
void CALLImm16(CPU* cpu, u16 value);
void RST(CPU* cpu, u8 value);
void POPreg16Stk(CPU* cpu, RegisterID regId);
void PUSHreg16Stk(CPU* cpu, RegisterID regId);
void LDHDerefCToa(CPU* cpu);
void LDHDerefImm8Toa(CPU* cpu, u8 value);
void LDDerefImm16Toa(CPU* cpu, u16 value);
void LDHaToDerefC(CPU* cpu);
void LDHaToDerefImm8(CPU* cpu, u8 value);
void LDaToDerefImm16(CPU* cpu, u16 value);
void ADDsp(CPU* cpu, s8 value);
void LDspPlusImm8ToHL(CPU* cpu, u8 value);
void LDhlToSP(CPU* cpu);

void DI(CPU* cpu);
void EI(CPU* cpu);
void RLC(CPU* cpu, RegisterID regId);
void RRC(CPU* cpu, RegisterID regId);
void RL(CPU* cpu, RegisterID regId);
void RR(CPU* cpu, RegisterID regId);
void SLA(CPU* cpu, RegisterID regId);
void SRA(CPU* cpu, RegisterID regId);
void SWAP(CPU* cpu, RegisterID regId);
void SRL(CPU* cpu, RegisterID regId);
void BIT(CPU* cpu, RegisterID regId, u8 bitIdx);
void RES(CPU* cpu, RegisterID regId, u8 bitIdx);
void SET(CPU* cpu, RegisterID regId, u8 bitIdx);
void HardLock(CPU* cpu);

#endif	// NEPGB_INSTRUCTIONS_H