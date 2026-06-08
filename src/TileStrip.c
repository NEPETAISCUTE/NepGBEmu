#include "TileStrip.h"

#include <stdio.h>
#include <stdlib.h>

TileStrip* TileStripCreate(PPU* ppu, u8 tileID, u8 scanline, u16 blockOrigin) {
	TileStrip* tileStrip = malloc(sizeof(TileStrip));
	if (tileStrip == NULL) return NULL;

	if (tileID >= 128) blockOrigin = 0x8800;
	u8 relY = scanline % 8;
	u8 byteHigh = MemoryBusRead(ppu->bus, blockOrigin + tileID * 0x10 + relY * 2, false);
	u8 byteLow = MemoryBusRead(ppu->bus, blockOrigin + tileID * 0x10 + relY * 2 + 1, false);
	for (u8 i = 0; i < 8; i++) {
		u8 colorIndex = BuildU2(GetBit(byteHigh, i), GetBit(byteLow, i));
		// printf("tileID = %d, relY = %d, relX = %d, colorIndex = %d\n", tileID, relY, 7 - i, colorIndex);
		tileStrip->pixelArray[7 - i] = colorIndex;	// apparently it's backwards
	}
	return tileStrip;
}
void TileStripDestroy(TileStrip* tileStrip) { free(tileStrip); }

u8 TileStripGetColorIndex(TileStrip* tileStrip, u8 x) { return tileStrip->pixelArray[x % 8]; }
