#ifndef NEPGB_PPU_REGISTERS_H
#define NEPGB_PPU_REGISTERS_H

#include "common.h"

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

#endif	// NEPGB_PPU_REGISTERS_H