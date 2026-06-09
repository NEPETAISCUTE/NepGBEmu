#ifndef NEPGB_PPU_H
#define NEPGB_PPU_H

#include "MasterPalette.h"
#include "MemoryBus.h"

#define PPU_SCREEN_WIDTH 160
#define PPU_SCREEN_HEIGHT 144

typedef struct PPU {
	MemoryBus* bus;

	Color framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];
	Texture frameTex;

	size_t frame;
	size_t scanline;
	size_t cycle;

	bool isOff;
} PPU;

PPU* PPUCreate(MemoryBus* bus);
void PPUDestroy(PPU* ppu);

void PPUUpdate(PPU* ppu);

void PPUPlot(PPU* ppu, u8 x, u8 y, Color c);

#endif	// NEPGB_PPU_H
