#ifndef NEPGB_TILE_STRIP_H
#define NEPGB_TILE_STRIP_H

#include "MemoryBus.h"
#include "PPU.h"

typedef struct TileStrip {
	u8 pixelArray[8];
} TileStrip;

TileStrip* TileStripCreate(PPU* ppu, u8 tileID, u8 scanline, u16 blockOrigin);
void TileStripDestroy(TileStrip* tileStrip);

u8 TileStripGetColorIndex(TileStrip* tileStrip, u8 x);

#endif	// NEPGB_TILE_STRIP_H
