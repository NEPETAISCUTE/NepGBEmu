#include "PPURenderer.h"

#include "MasterPalette.h"
#include "PPUObject.h"
#include "TileStrip.h"
#include "common.h"

#define VRAM_TILE_MAP 0x9800

#include <stdio.h>

u32 PPURendererDrawScanlineBG(PPU* ppu) {
	u32 screenY = ppu->scanline;
	u32 y = screenY + ppu->bus->ppuRegs.rSCY;

	u32 x = 0;
	u32 scrollX = ppu->bus->ppuRegs.rSCX;
	u8 fineXOffset = scrollX % 8;
	// u32 renderCycles = fineXOffset + 12;

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
			// renderCycles++;
			if (x >= PPU_SCREEN_WIDTH) break;
		}
		fineXOffset = 0;
		TileStripDestroy(strip);
	}

	return 172;	 // renderCycles;  // temporary
}

u32 PPURendererDrawScanlineWindow(PPU* ppu) { return 0; }

u32 PPURendererDrawScanlineOBJ(PPU* ppu) {
	u8 objRenderCnt = 0;

	// TODO: implement OBJ selection
	// TODO: add drawing priority

	size_t penalty = 0;

	for (size_t i = 0; i < 40; i++) {
		if (objRenderCnt >= 10) break;
		u8 objY = MemoryBusRead(ppu->bus, OAM_START + 4 * i, false);
		bool is8x16 = GetFlag(MemoryBusRead(ppu->bus, 0xFF40, false), 2);

		if ((ppu->scanline - (objY - 16)) >= ((is8x16) ? 16 : 8)) continue;

		PPUObject* obj = PPUObjectCreate(ppu, i);
		// penalty calculation:
		// TODO: take in account the window
		if (obj->x == 0) {
			penalty += 11;
		} else {
			// u8 tileX = (obj->x - 8 + ppu->bus->ppuRegs.rSCX) / 8;
			// u8 tileY = (ppu->scanline + ppu->bus->ppuRegs.rSCY) / 8;

			// TODO: calculate penalty
		}

		for (size_t objX = 0; objX < 8; objX++) {
			u8 colorIndex;
			if (obj->useOBP1) {
				colorIndex = GetBits(ppu->bus->ppuRegs.rOBP1, TileStripGetColorIndex(obj->strip, objX) * 2, 2);
			} else {
				colorIndex = GetBits(ppu->bus->ppuRegs.rOBP0, TileStripGetColorIndex(obj->strip, objX) * 2, 2);
			}

			if (colorIndex != 3) {
				Color c = ppu->framebuffer[ppu->scanline * 160 + obj->x + objX];
				Color transparent = DMG_MASTER_PALETTE[0];
				if (!obj->BGPriority || (transparent.r == c.r && transparent.g == c.g && transparent.b == c.b && transparent.a == c.a)) {
					PPUPlot(ppu, obj->x + objX - 8, ppu->scanline, DMG_MASTER_PALETTE[colorIndex]);
				}
			}
		}
		PPUObjectDestroy(obj);
		objRenderCnt++;
	}

	return 0;
}

void PPURendererDrawTilesetScanline(PPU* ppu) {
	u32 screenY = ppu->scanline;
	u32 y = ppu->scanline;

	u32 tileVert = y / 8;

	u16 tileMapBase = VRAM_TILE_MAP;
	if (GetFlag(ppu->bus->ppuRegs.rLCDC, 3)) tileMapBase = 0x9C00;

	for (u32 tileHorz = 0; tileHorz < 0x10 /*(PPU_SCREEN_WIDTH / 8)*/; tileHorz++) {
		u32 tileIndex = tileVert * 0x10 + tileHorz;

		u8 tileId = tileIndex;

		// TODO: add support for LCDC.4 = 1
		TileStrip* strip = TileStripCreate(ppu, tileIndex, y % 8, (GetFlag(ppu->bus->ppuRegs.rLCDC, 4)) ? 0x8000 : 0x9000);

		for (u8 fineX = 0; fineX < 8; fineX++) {
			u32 x = tileHorz * 8 + fineX;

			u8 colorIndex = TileStripGetColorIndex(strip, fineX);
			Color c = DMG_MASTER_PALETTE[colorIndex % 4];
			// printf("displaying pixel %X%X%X%X at x = %d, y = %d\n", c.a, c.b, c.g, c.r, x, y);

			PPUPlot(ppu, x, screenY, c);
		}
		TileStripDestroy(strip);
	}
}

// returns the number of cycles it took
// TODO: add support for windows, etc.
u32 PPURendererDrawScanline(PPU* ppu) {
	u32 cycleTime = PPURendererDrawScanlineBG(ppu);
	if (GetFlag(ppu->bus->ppuRegs.rLCDC, 1)) {
		cycleTime += PPURendererDrawScanlineOBJ(ppu);
	}

	return cycleTime;
}