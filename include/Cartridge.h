#ifndef NEPGB_CARTRIDGE_H
#define NEPGB_CARTRIDGE_H

#include "common.h"

extern const u16 ROM_BANK0_START;
extern const u16 ROM_BANK0_END;
extern const u16 ROM_BANKX_START;
extern const u16 ROM_BANKX_END;
extern const u16 CARTRIDGE_RAM_START;
extern const u16 CARTRIDGE_RAM_END;

typedef enum CartridgeType {
	CARTRIDGE_TYPE_ROM_ONLY = 0x0,
	CARTRIDGE_TYPE_MBC1 = 0x1,
	CARTRIDGE_TYPE_MBC1_RAM = 0x2,
	CARTRIDGE_TYPE_MBC1_RAM_BATTERY = 0x3,
	CARTRIDGE_TYPE_MBC2 = 0x5,
	CARTRIDGE_TYPE_MBC2_BATTERY = 0x6,
	CARTRIDGE_TYPE_ROM_RAM = 0x8,
	CARTRIDGE_TYPE_ROM_RAM_BATTERY = 0x9,
	CARTRIDGE_TYPE_MMM01 = 0xB,
	CARTRIDGE_TYPE_MMM01_RAM = 0xC,
	CARTRIDGE_TYPE_MMM01_RAM_BATTERY = 0xD,
	CARTRIDGE_TYPE_MBC3_TIMER_BATTERY = 0xF,
	CARTRIDGE_TYPE_MBC3_TIMER_RAM_BATTERY = 0x10,
	CARTRIDGE_TYPE_MBC3 = 0x11,
	CARTRIDGE_TYPE_MBC3_RAM = 0x12,
	CARTRIDGE_TYPE_MBC3_RAM_BATTERY = 0x13,
	CARTRIDGE_TYPE_MBC5 = 0x19,
	CARTRIDGE_TYPE_MBC5_RAM = 0x1A,
	CARTRIDGE_TYPE_MBC5_RAM_BATTERY = 0x1B,
	CARTRIDGE_TYPE_MBC5_RUMBLE = 0x1C,
	CARTRIDGE_TYPE_MBC5_RUMBLE_RAM = 0x1D,
	CARTRIDGE_TYPE_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	CARTRIDGE_TYPE_MBC6 = 0x20,
	CARTRIDGE_TYPE_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
	CARTRIDGE_TYPE_POCKET_CAMERA = 0xFC,
	CARTRIDGE_TYPE_BANDAI_TAMA5 = 0xFD,
	CARTRIDGE_TYPE_HUC3 = 0xFE,
	CARTRIDGE_TYPE_HUC1_RAM_BATTERY = 0xFF,
} CartridgeType;

typedef struct HeaderData {
	bool isNintendoLogoValid;
	char title[17];
	u32 manufacturerCode;
	bool isCGBGame;
	u32 licenseeCode;
	bool isSGBCompat;
	CartridgeType cartType;
	u32 romSize;
	u32 ramSize;
	bool isOverseasOnly;
	u8 versionNumber;
	u8 headerChecksum;
	bool isHeaderChecksumValid;
	u16 globalChecksum;
	bool isGlobalChecksumValid;
} HeaderData;

typedef struct Cartridge {
	HeaderData header;

	u8* rom;
	u8* ram;
} Cartridge;

const char* getCartridgeTypeName(Cartridge* cartridge);

Cartridge* CartridgeCreate(u8* bytes);
void CartridgeDestroy(Cartridge* cartridge);

u8 CartridgeRead(Cartridge* cart, u16 address);
void CartridgeWrite(Cartridge* cart, u16 address, u8 value);

#endif	// NEPGB_CARTRIDGE_H
