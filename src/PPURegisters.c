#include "PPURegisters.h"

u8 PPURegistersRead(PPURegisters* regs, u16 address) {
	switch (address) {
		case PPUREGS_ADDRESS_LCDC: return regs->rLCDC;
		case PPUREGS_ADDRESS_STAT: return AssignBit(regs->rSTAT, 2, regs->rLYC == regs->rLY);
		case PPUREGS_ADDRESS_SCY: return regs->rSCY;
		case PPUREGS_ADDRESS_SCX: return regs->rSCX;
		case PPUREGS_ADDRESS_LY: return (GetFlag(regs->rLCDC, 7)) ? regs->rLY : 0;	// rLYC keeps its value after PPU off, but rLY returns 0
		case PPUREGS_ADDRESS_LYC: return regs->rLYC;
		case PPUREGS_ADDRESS_DMA: return regs->rDMA;
		case PPUREGS_ADDRESS_BGP: return regs->rBGP;
		case PPUREGS_ADDRESS_OBP0: return regs->rOBP0;
		case PPUREGS_ADDRESS_OBP1: return regs->rOBP1;
		case PPUREGS_ADDRESS_WY: return regs->rWY;
		case PPUREGS_ADDRESS_WX: return regs->rWX;

		default: return 0;
	}
}
void PPURegistersWrite(PPURegisters* regs, u16 address, u8 value) {
	switch (address) {
		case PPUREGS_ADDRESS_LCDC: regs->rLCDC = value; return;
		case PPUREGS_ADDRESS_STAT: regs->rSTAT = GetBits(value, 3, 4); return;
		case PPUREGS_ADDRESS_SCY: regs->rSCY = value; return;
		case PPUREGS_ADDRESS_SCX: regs->rSCX = value; return;
		case PPUREGS_ADDRESS_LYC: regs->rLYC = value; return;
		case PPUREGS_ADDRESS_DMA: regs->rDMA = value; return;  // TODO: implement DMA behaviour
		case PPUREGS_ADDRESS_BGP: regs->rBGP = value; return;
		case PPUREGS_ADDRESS_OBP0: regs->rOBP0 = value; return;
		case PPUREGS_ADDRESS_OBP1: regs->rOBP1 = value; return;
		case PPUREGS_ADDRESS_WY: regs->rWY = value; return;
		case PPUREGS_ADDRESS_WX: regs->rWX = value; return;

		default: return;
	}
}