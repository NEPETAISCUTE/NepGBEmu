#include "MemoryBus.h"

#include <stdlib.h>

const u16 VRAM_START = 0x8000;
const u16 WRAM_BANK0_START = 0xC000;
const u16 WRAM_BANKX_START = 0xD000;
const u16 MIRROR_WRAM_START = 0xE000;
const u16 OAM_START = 0xFE00;
const u16 IO_REG_START = 0xFF00;
const u16 HRAM_START = 0xFF80;
const u16 INTERRUPT_ENABLE = 0xFFFF;

// DMG boot rom
const u8 BOOT_ROM_DMG[] = {
	0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e, 0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c,
	0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0, 0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd,
	0x96, 0x00, 0x13, 0x7b, 0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9, 0x3e, 0x19,
	0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20, 0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64,
	0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04, 0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d,
	0x20, 0xf2, 0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06, 0x7b, 0xe2, 0x0c, 0x3e,
	0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20, 0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb,
	0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17, 0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd,
	0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5,
	0xb9, 0xa5, 0x42, 0x3c, 0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20, 0xf5, 0x06,
	0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50,
};

MemoryBus* MemoryBusCreate(Cartridge* cart) {
	MemoryBus* bus = malloc(sizeof(MemoryBus));
	if (bus == NULL) return NULL;

	bus->cart = cart;

	bus->isBootRomLoaded = true;

	// default bank
	bus->workRAMBank = 0;

	bus->oamLock = false;
	bus->videoMemLock = false;

	return bus;
}

void MemoryBusDestroy(MemoryBus* bus) { free(bus); }

u8 MemoryBusRead(MemoryBus* bus, u16 address, bool isCPU) {
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

	if (address < 0x100 && bus->isBootRomLoaded) {
		return BOOT_ROM_DMG[address];
	}

	if (address >= ROM_BANK0_START && address < ROM_BANKX_START) {
		return CartridgeRead(bus->cart, address);
	}

	if (address >= ROM_BANKX_START && address < VRAM_START) {
		return CartridgeRead(bus->cart, address);
	}

	if (address >= VRAM_START && address < CARTRIDGE_RAM_START) {
		if (isCPU && bus->videoMemLock) return 0xFF;
		return bus->videoRAM[address - VRAM_START];
	}

	if (address >= CARTRIDGE_RAM_START && address < WRAM_BANK0_START) {
		return CartridgeRead(bus->cart, address);
	}

	if (address >= WRAM_BANK0_START && address < WRAM_BANKX_START) {
		return bus->workRAM[address - WRAM_BANK0_START];
	}

	if (address >= WRAM_BANKX_START && address < MIRROR_WRAM_START) {
		return bus->workRAM[address - WRAM_BANK0_START + 0x1000 * bus->workRAMBank];
	}

	if (address >= MIRROR_WRAM_START && address < OAM_START) {
		return MemoryBusRead(bus, address - 0x2000, isCPU);
	}

	if (address >= OAM_START && address < IO_REG_START) {
		if (isCPU && bus->oamLock) return 0xFF;
		// WIP
		return 0;
	}

	if (address >= IO_REG_START && address < HRAM_START) {
		if (address <= 0xFF3F) {
		} else if (address <= 0xFF4B) {
			return PPURegistersRead(&bus->ppuRegs, address);
		} else if (false) {
		} else {
		}
		return 0;
	}

	if (address >= HRAM_START && address < INTERRUPT_ENABLE) {
		return bus->highRAM[address - HRAM_START];
	}

	if (address == INTERRUPT_ENABLE) {
		// WIP
		return 0;
	}

	return 0;
}

void MemoryBusWrite(MemoryBus* bus, u16 address, u8 value, bool isCPU) {
	if (address >= ROM_BANK0_START && address < ROM_BANKX_START) {
		CartridgeWrite(bus->cart, address, value);
		return;
	}

	if (address >= ROM_BANKX_START && address < VRAM_START) {
		CartridgeWrite(bus->cart, address, value);
		return;
	}

	if (address >= VRAM_START && address < CARTRIDGE_RAM_START) {
		if (isCPU && bus->videoMemLock) return;

		bus->videoRAM[address - VRAM_START] = value;
		return;
	}

	if (address >= CARTRIDGE_RAM_START && address < WRAM_BANK0_START) {
		CartridgeWrite(bus->cart, address, value);
		return;
	}

	if (address >= WRAM_BANK0_START && address < WRAM_BANKX_START) {
		bus->workRAM[address - WRAM_BANK0_START] = value;
		return;
	}

	if (address >= WRAM_BANKX_START && address < MIRROR_WRAM_START) {
		bus->workRAM[address - WRAM_BANK0_START + 0x1000 * bus->workRAMBank] = value;
		return;
	}

	if (address >= MIRROR_WRAM_START && address < OAM_START) {
		MemoryBusWrite(bus, address - 0x2000, value, isCPU);
		return;
	}

	if (address >= OAM_START && address < IO_REG_START) {
		if (isCPU && bus->oamLock) return;
		// WIP
		return;
	}

	if (address >= IO_REG_START && address < HRAM_START) {
		if (address <= 0xFF3F) {
		} else if (address <= 0xFF4B) {
			PPURegistersWrite(&bus->ppuRegs, address, value);
		} else if (false) {
		} else {
		}
		return;
	}

	if (address >= HRAM_START && address < INTERRUPT_ENABLE) {
		bus->highRAM[address - HRAM_START] = value;
	}

	if (address == INTERRUPT_ENABLE) {
		// WIP
		return;
	}
}

u8 MemoryBusReadCPU(MemoryBus* bus, u16 address) { return MemoryBusRead(bus, address, true); }

void MemoryBusWriteCPU(MemoryBus* bus, u16 address, u8 value) { return MemoryBusWrite(bus, address, value, true); }
