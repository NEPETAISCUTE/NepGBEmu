#include "PPU.h"

#include <raylib.h>
#include <stdlib.h>

#include "PPURenderer.h"
/*const Color PALETTE[4] = {
	(Color){0x9a, 0x9e, 0x3f, 0xFF},
	(Color){0x49, 0x6b, 0x22, 0xFF},
	(Color){0x0e, 0x45, 0x0b, 0xFF},
	(Color){0x1b, 0x2a, 0x09, 0xFF},
};*/

PPU* PPUCreate(RenderTexture2D* framebuffer, MemoryBus* bus) {
	PPU* ppu = malloc(sizeof(PPU));
	if (ppu == NULL) return NULL;

	ppu->framebuffer = framebuffer;
	ppu->bus = bus;

	ppu->frame = 0;
	ppu->scanline = 0;
	ppu->cycle = 0;
	return ppu;
}
void PPUDestroy(PPU* ppu) { free(ppu); }

#define SCANLINE_VBLANK_START 144
#define SCANLINE_VBLANK_END 154

#define CYCLE_OAMSCAN_END 80

#include <stdio.h>

void PPUUpdate(PPU* ppu) {
	if (ppu->scanline < SCANLINE_VBLANK_START) {
		if (ppu->cycle < CYCLE_OAMSCAN_END) {
		} else if (ppu->cycle == CYCLE_OAMSCAN_END) {
			ppu->bus->oamLock = true;
			ppu->bus->videoMemLock = true;
			if (ppu->cycle == CYCLE_OAMSCAN_END) ppu->cycle += PPURendererDrawScanline(ppu) - 1;
		} else {
			ppu->bus->oamLock = false;
			ppu->bus->videoMemLock = false;
			// TODO:
			//  take in account SCX penality (SCX % 8)
			//  take in account window fetcher setup (6 dots)
			//  take in account OBJ penalty (6 to 11 dots)
			//  - number of pixels to the right of the pixel to draw if tile not considered before - 2 (if negative, 0 penalty)
			//  - add a base penalty of 6 dots
			//  - if OBJ is completely off-screen, 11 dot penalty regardless of SCX
		}
	} else if (ppu->scanline < SCANLINE_VBLANK_END) {
		ppu->bus->oamLock = false;
		ppu->bus->videoMemLock = false;
	} else {
		ppu->scanline = 0;
		ppu->frame++;

		// render present basically
		BeginDrawing();
		ClearBackground(BLACK);	 // tmp
		DrawTexturePro(ppu->framebuffer->texture,
					   (Rectangle){0, 0, (float)ppu->framebuffer->texture.width, (float)-ppu->framebuffer->texture.height},
					   (Rectangle){0, 0, 600, 400}, (Vector2){0, 0}, 0.0, WHITE);
		DrawFPS(0, 0);
		EndDrawing();
	}

	ppu->cycle++;
	if (ppu->cycle >= 456) {
		ppu->cycle = 0;
		ppu->scanline++;
		ppu->bus->oamLock = true;
		ppu->bus->videoMemLock = false;
	}
	// TODO: uncomment once there are PPURegs
	// ppu->bus->ppuRegs->rLY = ppu->scanline;
}

void PPUPlot(PPU* ppu, u8 x, u8 y, Color c) {
	BeginTextureMode(*(ppu->framebuffer));

	DrawPixel(x, y, c);

	EndTextureMode();
}