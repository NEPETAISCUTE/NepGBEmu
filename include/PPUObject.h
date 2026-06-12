#ifndef NEPGB_PPU_OBJECT_H
#define NEPGB_PPU_OBJECT_H

#include "PPU.h"
#include "TileStrip.h"
#include "common.h"

typedef struct PPUObject {
	u8 objY;
	u8 x;

	u8 tid;

	bool BGPriority;
	bool yFlip;
	bool xFlip;
	bool useOBP1;	// unused in CGB
	bool bank;		// unused in DMG
	u8 CGBPalette;	// unused in DMG

	TileStrip* strip;
} PPUObject;

PPUObject* PPUObjectCreate(PPU* ppu, u8 objSlot);
void PPUObjectDestroy(PPUObject* obj);

u8 PPUObjectGetColorIndex(PPUObject* obj, u8 relX);

#endif	// NEPGB_PPU_OBJECT_H