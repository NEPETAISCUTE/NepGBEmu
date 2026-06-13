#include "PPURenderer.h"
/*const Color PALETTE[4] = {
		(Color){0x9a, 0x9e, 0x3f, 0xFF},
		(Color){0x49, 0x6b, 0x22, 0xFF},
		(Color){0x0e, 0x45, 0x0b, 0xFF},
		(Color){0x1b, 0x2a, 0x09, 0xFF},
};*/

static void PPURequestVBlankInterrupt(PPU* ppu) {
	// if (!ppu->bus->isBootRomLoaded) printf("requesting vblank at frame %d, scanline %d, cycle %d\n", ppu->frame, ppu->scanline, ppu->cycle);
	MemoryBusWrite(ppu->bus, 0xFF0F, SetBit(MemoryBusRead(ppu->bus, 0xFF0F, false), 0), false);
}

static void PPURequestSTATInterrupt(PPU* ppu) {
	// if (!ppu->bus->isBootRomLoaded) printf("requesting stat at frame %d, scanline %d, cycle %d\n", ppu->frame, ppu->scanline, ppu->cycle);
	MemoryBusWrite(ppu->bus, 0xFF0F, SetBit(MemoryBusRead(ppu->bus, 0xFF0F, false), 1), false);
}

static void PPUUpdateSTATMode(PPU* ppu, u8 mode, bool requestInterrupt) {
	mode = mode & 0b11;
	u8 stat = ppu->bus->ppuRegs.rSTAT;
	u8 prevMode = stat & 0b11;
	if (prevMode == mode) return;
	// printf("prevMode %d != mode %d at scanline %d\n", prevMode, mode, ppu->scanline);
	ppu->bus->ppuRegs.rSTAT = AssignBits(stat, 0, 2, mode);
	// changing locking
	switch (mode) {
		case 0:
		case 1:
			ppu->bus->oamLock = false;
			ppu->bus->videoMemLock = false;
			break;
		case 2:
			ppu->bus->oamLock = true;
			ppu->bus->videoMemLock = false;
			break;
		case 3:
			ppu->bus->oamLock = true;
			ppu->bus->videoMemLock = true;
			break;
		default:
	}
	if (mode != 3 && GetFlag(ppu->bus->ppuRegs.rSTAT, 3 + mode)) PPURequestSTATInterrupt(ppu);
	if (GetFlag(ppu->bus->ppuRegs.rSTAT, 6) && ppu->bus->ppuRegs.rLYC == ppu->bus->ppuRegs.rLY) PPURequestSTATInterrupt(ppu);
}

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

	ppu->isOff = true;
	PPUUpdateSTATMode(ppu, 0, true);

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
		.width = GetRenderWidth(),
		.height = GetRenderHeight(),
	};

	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(ppu->frameTex, source, dest, (Vector2){0}, 0.0, WHITE);
	DrawFPS(0, 0);
	EndDrawing();
}

void PPUUpdate(PPU* ppu) {
	if (!ppu->isOff && !GetFlag(ppu->bus->ppuRegs.rLCDC, 7)) {
		ppu->bus->oamLock = false;
		ppu->bus->videoMemLock = false;

		for (size_t i = 0; i < PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT; i++) {
			ppu->framebuffer[i] = DMG_MASTER_PALETTE[4];  // whiter than white, color impossible to get without screen off
		}

		PPUUpdateSTATMode(ppu, 0, false);  // mode 0

		printf("ppu is off\n");

		ppu->isOff = true;
		return;
	}

	if (ppu->isOff) {
		if (GetFlag(ppu->bus->ppuRegs.rLCDC, 7)) {
			printf("ppu is on\n");
			ppu->isOff = false;
			ppu->cycle = 0;	 // unsure about this
			ppu->scanline = 0;
		}
	}

	if (!ppu->isOff) {
		if (ppu->scanline < SCANLINE_VBLANK_START) {
			if (ppu->cycle == 0) {
				PPUUpdateSTATMode(ppu, 2, true);  // mode 2
			} else if (ppu->cycle < CYCLE_OAMSCAN_END) {
			} else if (ppu->cycle == CYCLE_OAMSCAN_END) {
				// TODO:
				//  take in account window fetcher setup (6 dots)
				//  take in account OBJ penalty (6 to 11 dots)
				//  - number of pixels to the right of the pixel to draw if tile not
				//  considered before - 2 (if negative, 0 penalty)
				//  - add a base penalty of 6 dots
				//  - if OBJ is completely off-screen, 11 dot penalty regardless of SCX
				ppu->waitCycles = PPURendererDrawScanline(ppu) - 1;	 // 172 to 289 dots
				// printf("waitCycles at scanline %d: %d\n", ppu->scanline, ppu->waitCycles + 1);
				PPUUpdateSTATMode(ppu, 3, true);  // mode 3
												  // PPURendererDrawTilesetScanline(ppu);
			} else if (ppu->waitCycles > 0) {
				ppu->waitCycles--;
				// if (ppu->waitCycles == 0) printf("in HBlank at cycle %d\n", ppu->cycle + 1);
			} else {
				PPUUpdateSTATMode(ppu, 0, true);  // mode 0
			}
		} else if (ppu->scanline < SCANLINE_VBLANK_END) {
			if (GetBits(ppu->bus->ppuRegs.rSTAT, 0, 2) != 1) {
				PPUUpdateSTATMode(ppu, 1, true);  // mode 1
				PPURequestVBlankInterrupt(ppu);

				// what happens after that doesn't matter since it's vblank and the ppu doesn't draw, + if we turn off and turn on ppu, it
				// shouldn't "not render"
			}
		}

		ppu->cycle++;
		if (ppu->cycle >= 456) {
			ppu->cycle = 0;
			ppu->scanline++;
			ppu->bus->ppuRegs.rLY = ppu->scanline;
			if (ppu->bus->ppuRegs.rLYC == ppu->bus->ppuRegs.rLY) PPURequestSTATInterrupt(ppu);
		}
		if (ppu->scanline > 153) {
			ppu->scanline = 0;
			ppu->frame++;
			PPUUpdateSTATMode(ppu, 2, true);
			// render present basically
			PPURenderFrame(ppu);
		}
	}
}

void PPUPlot(PPU* ppu, u8 x, u8 y, Color c) {
	if (x >= PPU_SCREEN_WIDTH) return;
	if (y >= PPU_SCREEN_HEIGHT) return;

	size_t index = x + y * PPU_SCREEN_WIDTH;
	ppu->framebuffer[index] = c;
}
