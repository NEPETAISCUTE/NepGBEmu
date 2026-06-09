#ifndef NEPGB_PPU_REGISTERS_H
#define NEPGB_PPU_REGISTERS_H

#include "common.h"

typedef enum PPURegisterAddress {
	PPUREGS_ADDRESS_LCDC = 0xFF40,
	PPUREGS_ADDRESS_STAT = 0xFF41,
	PPUREGS_ADDRESS_SCY = 0xFF42,
	PPUREGS_ADDRESS_SCX = 0xFF43,
	PPUREGS_ADDRESS_LY = 0xFF44,
	PPUREGS_ADDRESS_LYC = 0xFF45,
	PPUREGS_ADDRESS_DMA = 0xFF46,
	PPUREGS_ADDRESS_BGP = 0xFF47,
	PPUREGS_ADDRESS_OBP0 = 0xFF48,
	PPUREGS_ADDRESS_OBP1 = 0xFF49,
	PPUREGS_ADDRESS_WY = 0xFF4A,
	PPUREGS_ADDRESS_WX = 0xFF4B,
} PPURegisterAddress;

typedef struct PPURegisters {
	// joypad

	// serial

	// divider

	// interrupts

	// audio

	// video
	// DMG mode only
	u8 rLCDC;
	u8 rSTAT;
	u8 rSCX;
	u8 rSCY;
	u8 rLY;	 // for PPU to be able to update it, so that it can return the correct value, too lazy to implement pointers going back and forth
	u8 rLYC;
	u8 rDMA;
	u8 rBGP;
	u8 rOBP0;
	u8 rOBP1;
	u8 rWY;
	u8 rWX;

	// CGB only

} PPURegisters;

u8 PPURegistersRead(PPURegisters* regs, u16 address);
void PPURegistersWrite(PPURegisters* regs, u16 address, u8 value);

#endif	// NEPGB_PPU_REGISTERS_H
