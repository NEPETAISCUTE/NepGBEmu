#include "PPURenderer.h"

#include "MasterPalette.h"
#include "TileStrip.h"
#include "common.h"

#define VRAM_TILE_MAP 0x9800

#include <stdio.h>

// returns the number of cycles it took
// TODO: add support for windows, scrolling, etc.
u32 PPURendererDrawScanline(PPU* ppu) {
	u32 screenY = ppu->scanline;
	u32 y = screenY + ppu->bus->ppuRegs.rSCY;

	u32 x = 0;
	u32 scrollX = ppu->bus->ppuRegs.rSCX;
	u8 fineXOffset = scrollX % 8;

	u32 tileVert = y / 8;

	u16 tileMapBaseOffset = 0;
	if (GetFlag(ppu->bus->ppuRegs.rLCDC, 3)) tileMapBaseOffset = 0x0400;

	for (u32 tileHorz = scrollX / 8; tileHorz < PPU_SCREEN_WIDTH / 8 + scrollX / 8 + (((scrollX % 8) > 0) ? 1 : 0); tileHorz++) {
		u32 tileIndex = tileVert * 0x20 + (tileHorz % 0x20);

		u8 tileId = MemoryBusRead(ppu->bus, VRAM_TILE_MAP + (tileMapBaseOffset + tileIndex) % 0x800, false);
		// if (ppu->frame == 0 && tileVert == 2 && y % 8 == 0) printf("tile %X at index %d,%d\n", tileId, tileHorz, tileVert);

		// TODO: add support for LCDC.4 = 1
		TileStrip* strip = TileStripCreate(ppu, tileId, y % 8, (GetFlag(ppu->bus->ppuRegs.rLCDC, 4)) ? 0x8000 : 0x9000);

		for (u8 fineX = fineXOffset; fineX < 8; fineX++) {
			u8 colorIndex = GetBits(ppu->bus->ppuRegs.rBGP, TileStripGetColorIndex(strip, fineX) * 2, 2);
			Color c = DMG_MASTER_PALETTE[colorIndex % 4];
			// printf("displaying pixel %X%X%X%X at x = %d, y = %d\n", c.a, c.b, c.g, c.r, x, y);

			PPUPlot(ppu, x++, screenY, c);
			if (x >= PPU_SCREEN_WIDTH) break;
		}
		fineXOffset = 0;
		TileStripDestroy(strip);
	}

	return 1;  // temporary
}

void PPURendererDrawTilesetScanline(PPU* ppu) {
	u32 screenY = ppu->scanline;
	u32 y = ppu->scanline;

	u32 tileVert = y / 8;

	u16 tileMapBase = VRAM_TILE_MAP;
	if (GetFlag(ppu->bus->ppuRegs.rLCDC, 3)) tileMapBase = 0x9C00;

	for (u32 tileHorz = 0; tileHorz < 0x10 /*(PPU_SCREEN_WIDTH / 8)*/; tileHorz++) {
		u32 tileIndex = tileVert * 0x10 + tileHorz;

		u8 tileId = MemoryBusRead(ppu->bus, tileMapBase + tileIndex, false);
		if (ppu->frame == 0 && tileVert == 3 && y % 8 == 0) printf("tile %d at index %d,%d\n", tileId, tileHorz, tileVert);

		// TODO: add support for LCDC.4 = 1
		TileStrip* strip = TileStripCreate(ppu, tileIndex, y % 8, (GetFlag(ppu->bus->ppuRegs.rLCDC, 4)) ? 0x8000 : 0x9000);

		for (u8 fineX = 0; fineX < 8; fineX++) {
			u32 x = tileHorz * 8 + fineX;

			u8 colorIndex = TileStripGetColorIndex(strip, fineX);
			Color c = DMG_MASTER_PALETTE[colorIndex % 4];
			// printf("displaying pixel %X%X%X%X at x = %d, y = %d\n", c.a, c.b, c.g, c.r, x, y);

			if (GetFlag(ppu->bus->ppuRegs.rLCDC, 0)) {
				PPUPlot(ppu, x, screenY, DMG_MASTER_PALETTE[3]);
			} else {
				PPUPlot(ppu, x, screenY, c);
			}
		}
		TileStripDestroy(strip);
	}
}