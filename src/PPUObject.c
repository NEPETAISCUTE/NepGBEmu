#include "PPUObject.h"

PPUObject* PPUObjectCreate(PPU* ppu, u8 objSlot) {
	PPUObject* obj = malloc(sizeof(PPUObject));
	if (obj == NULL) return NULL;

	obj->objY = MemoryBusRead(ppu->bus, OAM_START + objSlot * 4, false);
	obj->x = MemoryBusRead(ppu->bus, OAM_START + 1 + objSlot * 4, false);
	obj->tid = MemoryBusRead(ppu->bus, OAM_START + 2 + objSlot * 4, false);

	u8 attr = MemoryBusRead(ppu->bus, OAM_START + 3 + objSlot * 4, false);

	obj->BGPriority = GetFlag(attr, 7);
	obj->yFlip = GetFlag(attr, 6);
	obj->xFlip = GetFlag(attr, 5);
	obj->useOBP1 = GetFlag(attr, 4);
	obj->bank = GetFlag(attr, 3);
	obj->CGBPalette = GetBits(attr, 0, 3);

	bool is8x16 = GetFlag(MemoryBusRead(ppu->bus, 0xFF40, false), 2);
	u8 ySlice = ppu->scanline - (obj->objY - 16);
	u8 tileId;
	if (is8x16) {
		tileId = ((ySlice) >= 8) ? (obj->tid & 0xFE) : ((obj->tid & 0xFE) | 1);
	} else {
		tileId = obj->tid;
	}
	obj->strip = TileStripCreate(ppu, tileId, ySlice, (tileId >= 128) ? 0x8800 : 0x8000);
	if (obj->strip == NULL) return NULL;

	return obj;
}

void PPUObjectDestroy(PPUObject* obj) {
	free(obj->strip);
	free(obj);
}

u8 PPUObjectGetColorIndex(PPUObject* obj, u8 relX) { return TileStripGetColorIndex(obj->strip, relX); }