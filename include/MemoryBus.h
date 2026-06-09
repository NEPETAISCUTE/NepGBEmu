#ifndef NEPGB_MEMORY_BUS_H
#define NEPGB_MEMORY_BUS_H

#include "Cartridge.h"
#include "PPURegisters.h"

extern const u16 VRAM_START;
extern const u16 WRAM_BANK0_START;
extern const u16 WRAM_BANKX_START;
extern const u16 MIRROR_WRAM_START;
extern const u16 OAM_START;
extern const u16 IO_REG_START;
extern const u16 HRAM_START;
extern const u16 INTERRUPT_ENABLE;

typedef struct MemoryBus {
	// 0x0000 - 0x3FFF: ROM bank 00
	// 0x4000 - 0x7FFF: ROM bank XX
	// 0x8000 - 0x9FFF: VRAM
	// 0xA000 - 0xBFFF: Cart RAM
	// 0xC000 - 0xCFFF: WRAM bank 00
	// 0xD000 - 0xDFFF: WRAM bank XX
	// 0xE000 - 0xFDFF: mirror of 0xC000-0xDDFF (possibly unstable, nintendo says it is ill advised to read/write on it)
	// 0xFE00 - 0xFE9F: OAM
	// 0xFEA0 - 0xFEFF: unusable (ill advised as well)
	// 0xFF00 - 0xFF7F: I/O registers
	// 0xFF80 - 0xFFFE: HRAM
	// 0xFFFF: Interrupt Enable register (IE)

	Cartridge* cart;
	PPURegisters ppuRegs;

	// TODO: move videoRam to PPU, as it's the PPU's role to handle whether VRAM is locked or not, depending on the rendering process
	u8 videoRAM[0x4000];
	u8 workRAM[0x8000];

	u8 highRAM[0x7E];

	u8 workRAMBank;

	bool isBootRomLoaded;

	bool oamLock;
	bool videoMemLock;
} MemoryBus;

MemoryBus* MemoryBusCreate(Cartridge* cart);
void MemoryBusDestroy(MemoryBus* bus);

u8 MemoryBusRead(MemoryBus* bus, u16 address, bool isCPU);
void MemoryBusWrite(MemoryBus* bus, u16 address, u8 value, bool isCPU);

u8 MemoryBusReadCPU(MemoryBus* bus, u16 address);
void MemoryBusWriteCPU(MemoryBus* bus, u16 address, u8 value);

#endif	// NEPGB_MEMORY_BUS_H
