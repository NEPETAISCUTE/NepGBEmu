#include "PPURenderer.h"
/*const Color PALETTE[4] = {
		(Color){0x9a, 0x9e, 0x3f, 0xFF},
		(Color){0x49, 0x6b, 0x22, 0xFF},
		(Color){0x0e, 0x45, 0x0b, 0xFF},
		(Color){0x1b, 0x2a, 0x09, 0xFF},
};*/

static bool PPUInitFrameBuffer(PPU* ppu) {
	// Stitching an image manually since Raylib doesn't offer a neat way to create
	// an image without allocating memory
	Image img = (Image){
		.data = ppu->framebuffer,
		.width = PPU_SCREEN_WIDTH,
		.height = PPU_SCREEN_HEIGHT,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
	};

	ppu->frameTex = LoadTextureFromImage(img);
	return IsTextureValid(ppu->frameTex);
}

PPU* PPUCreate(MemoryBus* bus) {
	PPU* ppu = calloc(1, sizeof(PPU));
	if (ppu == NULL) return NULL;

	if (!PPUInitFrameBuffer(ppu)) {
		free(ppu);
		return NULL;
	}

	ppu->bus = bus;

	return ppu;
}

void PPUDestroy(PPU* ppu) {
	UnloadTexture(ppu->frameTex);
	free(ppu);
}

#define SCANLINE_VBLANK_START 144
#define SCANLINE_VBLANK_END 154

#define CYCLE_OAMSCAN_END 80

static void PPURenderFrame(PPU* ppu) {
	UpdateTexture(ppu->frameTex, ppu->framebuffer);

	const Rectangle source = (Rectangle){
		.x = 0,
		.y = 0,
		.width = PPU_SCREEN_WIDTH,
		.height = PPU_SCREEN_HEIGHT,
	};

	const Rectangle dest = (Rectangle){
		.x = 0,
		.y = 0,
		.width = 600,
		.height = 400,
	};

	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(ppu->frameTex, source, dest, (Vector2){0}, 0.0, WHITE);
	DrawFPS(0, 0);
	EndDrawing();
}

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
			//  - number of pixels to the right of the pixel to draw if tile not
			//  considered before - 2 (if negative, 0 penalty)
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
		PPURenderFrame(ppu);
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
	if (x >= PPU_SCREEN_WIDTH) return;
	if (y >= PPU_SCREEN_HEIGHT) return;

	size_t index = x + y * PPU_SCREEN_WIDTH;
	ppu->framebuffer[index] = c;
}
