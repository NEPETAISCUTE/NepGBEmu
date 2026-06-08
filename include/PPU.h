#ifndef NEPGB_PPU_H
#define NEPGB_PPU_H

#include <raylib.h>
#include <stdlib.h>

#include "MemoryBus.h"
#include "common.h"

typedef struct PPU {
	MemoryBus* bus;

	RenderTexture2D* framebuffer;

	size_t frame;
	size_t scanline;
	size_t cycle;
} PPU;

#define PPU_SCREEN_WIDTH 160
#define PPU_SCREEN_HEIGHT 144

PPU* PPUCreate(RenderTexture2D* framebuffer, MemoryBus* bus);
void PPUDestroy(PPU* ppu);

void PPUUpdate(PPU* ppu);

void PPUPlot(PPU* ppu, u8 x, u8 y, Color c);

#endif	// NEPGB_PPU_H