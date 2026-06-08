#include "PPURenderer.h"

#include "MasterPalette.h"
#include "TileStrip.h"
#include "common.h"

#define VRAM_TILE_MAP 0x9800

#include <stdio.h>

// returns the number of cycles it took
u32 PPURendererDrawScanline(PPU* ppu) {
	const u32 y = ppu->scanline;

	const u32 tileVert = y / 8;

	for (u32 tileHorz = 0; tileHorz < PPU_SCREEN_WIDTH / 8; tileHorz++) {
		u32 tileIndex = tileVert * 0x20 + tileHorz;

		u8 tileId = MemoryBusRead(ppu->bus, VRAM_TILE_MAP + tileIndex, false);

		// TODO: add support for LCDC.4 = 1
		TileStrip* strip = TileStripCreate(ppu, tileId, y, 0x8000);

		for (u8 fineX = 0; fineX < 8; fineX++) {
			u32 x = tileHorz * 8 + fineX;

			u8 colorIndex = TileStripGetColorIndex(strip, fineX);
			Color c = DMG_MASTER_PALETTE[colorIndex % 4];
			// printf("displaying pixel %X%X%X%X at x = %d, y = %d\n", c.a, c.b, c.g, c.r, x, y);

			PPUPlot(ppu, x, y, c);
		}
		TileStripDestroy(strip);
	}

	return 1;  // temporary
}
