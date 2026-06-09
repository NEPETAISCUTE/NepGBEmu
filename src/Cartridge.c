#include "Cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* getCartridgeTypeName(Cartridge* cartridge) {
	u8 type = cartridge->header.cartType;

	switch (type) {
		case CARTRIDGE_TYPE_ROM_ONLY: return "ROM ONLY";
		case CARTRIDGE_TYPE_MBC1: return "MBC1";
		case CARTRIDGE_TYPE_MBC1_RAM: return "MBC1+RAM";
		case CARTRIDGE_TYPE_MBC1_RAM_BATTERY: return "MBC1+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC2: return "MBC2";
		case CARTRIDGE_TYPE_MBC2_BATTERY: return "MBC2+BATTERY";
		case CARTRIDGE_TYPE_ROM_RAM: return "ROM+RAM";
		case CARTRIDGE_TYPE_ROM_RAM_BATTERY: return "ROM+RAM+BATTERY";
		case CARTRIDGE_TYPE_MMM01: return "MMM01";
		case CARTRIDGE_TYPE_MMM01_RAM: return "MMM01+RAM";
		case CARTRIDGE_TYPE_MMM01_RAM_BATTERY: return "MMM01+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC3_TIMER_BATTERY: return "MBC3+TIMER+BATTERY";
		case CARTRIDGE_TYPE_MBC3_TIMER_RAM_BATTERY: return "MBC3+TIMER+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC3: return "MBC3";
		case CARTRIDGE_TYPE_MBC3_RAM: return "MBC3+RAM";
		case CARTRIDGE_TYPE_MBC3_RAM_BATTERY: return "MBC3+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC5: return "MBC5";
		case CARTRIDGE_TYPE_MBC5_RAM: return "MBC5+RAM";
		case CARTRIDGE_TYPE_MBC5_RAM_BATTERY: return "MBC5+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC5_RUMBLE: return "MBC5+RUMBLE";
		case CARTRIDGE_TYPE_MBC5_RUMBLE_RAM: return "MBC5+RUMBLE+RAM";
		case CARTRIDGE_TYPE_MBC5_RUMBLE_RAM_BATTERY: return "MBC5+RUMBLE+RAM+BATTERY";
		case CARTRIDGE_TYPE_MBC6: return "MBC6";
		case CARTRIDGE_TYPE_MBC7_SENSOR_RUMBLE_RAM_BATTERY: return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
		case CARTRIDGE_TYPE_POCKET_CAMERA: return "POCKET CAMERA";
		case CARTRIDGE_TYPE_BANDAI_TAMA5: return "BANDAI TAMA5";
		case CARTRIDGE_TYPE_HUC3: return "HuC3";
		case CARTRIDGE_TYPE_HUC1_RAM_BATTERY: return "HuC1+RAM+BATTERY";

		default: return "Unknown...";
	}
}

Cartridge* CartridgeCreate(u8* bytes) {
	// nintendo logo data
	const u8 NintendoLogo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
							   0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
							   0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

	// the 0x100 first bytes of the ROM aren't the header, they're interrupt vectors, and also, are replaced by the gameboy BIOS at startup
	const u16 HEADER_OFFSET = 0x104;
	const u16 HEADER_TITLE = 0x134;
	const u16 HEADER_MANUFACTURER_CODE = 0x13F;
	const u16 HEADER_CGB_FLAG = 0x143;
	const u16 HEADER_NEW_LICENSEE_CODE = 0x144;
	const u16 HEADER_SGB_FLAG = 0x146;
	const u16 HEADER_CART_TYPE = 0x147;
	const u16 HEADER_ROM_SIZE = 0x148;
	const u16 HEADER_RAM_SIZE = 0x149;
	const u16 HEADER_DESTINATION_CODE = 0x14A;
	const u16 HEADER_OLD_LICENSEE_CODE = 0x14B;
	const u16 HEADER_VERSION_NUMBER = 0x14C;
	const u16 HEADER_HEADER_CHECKSUM = 0x14D;
	const u16 HEADER_GLOBAL_CHECKSUM = 0x14E;

	Cartridge* cartridge = malloc(sizeof(Cartridge));
	if (cartridge == NULL) return NULL;

	// just avoiding issues with uninited values
	cartridge->rom = NULL;
	cartridge->ram = NULL;

	cartridge->header.isNintendoLogoValid = memcmp(&bytes[HEADER_OFFSET], NintendoLogo, 0x30) == 0;

	bool isCGBFlagByteTitle = true;
	switch (bytes[HEADER_CGB_FLAG]) {
		case 0x80: break;
		case 0xC0: break;
		default: isCGBFlagByteTitle = false;
	}

	u8 titleSize = 16;
	if (isCGBFlagByteTitle) {
		titleSize--;

		// gonna assume it's 15 character title
		titleSize = 15;
	}

	cartridge->header.romSize = 32768 * (1 << bytes[HEADER_ROM_SIZE]);

	switch (bytes[HEADER_RAM_SIZE]) {
		case 2: cartridge->header.ramSize = 8192; break;
		case 3: cartridge->header.ramSize = 32768; break;
		case 4: cartridge->header.ramSize = 131072; break;
		case 5: cartridge->header.ramSize = 65536; break;
		default: cartridge->header.ramSize = 0;
	}

	cartridge->rom = malloc(cartridge->header.romSize);
	if (cartridge->rom == NULL) {
		CartridgeDestroy(cartridge);
		return NULL;
	}

	cartridge->ram = malloc(cartridge->header.ramSize);
	if (cartridge->ram == NULL) {
		CartridgeDestroy(cartridge);
		return NULL;
	}

	strncpy(cartridge->header.title, (const char*)&bytes[HEADER_TITLE], titleSize);
	cartridge->header.title[titleSize] = '\0';

	cartridge->header.manufacturerCode = *(u32*)(&bytes[HEADER_MANUFACTURER_CODE]);

	cartridge->header.isSGBCompat = bytes[HEADER_SGB_FLAG] == 0x03;
	cartridge->header.cartType = bytes[HEADER_CART_TYPE];

	cartridge->header.isOverseasOnly = bytes[HEADER_DESTINATION_CODE] == 1;
	cartridge->header.licenseeCode = (bytes[HEADER_OLD_LICENSEE_CODE] == 0x33)
										 ? ((*(u16*)(&bytes[HEADER_NEW_LICENSEE_CODE])) << 8) | bytes[HEADER_OLD_LICENSEE_CODE]
										 : bytes[HEADER_OLD_LICENSEE_CODE];

	cartridge->header.versionNumber = bytes[HEADER_VERSION_NUMBER];
	cartridge->header.headerChecksum = bytes[HEADER_HEADER_CHECKSUM];

	u8 checksum = 0;
	for (size_t address = 0x134; address <= 0x14C; address++) {
		checksum = checksum - bytes[address] - 1;
	}
	cartridge->header.isHeaderChecksumValid = cartridge->header.headerChecksum == checksum;

	cartridge->header.globalChecksum = bytes[HEADER_GLOBAL_CHECKSUM];

	u16 globalChecksum = 0;
	for (size_t address = 0x0000; address != 0x0000; address++) {
		if (address >= 0x14E && address <= 0x14F) continue;
		globalChecksum += bytes[address];
	}

	cartridge->header.isGlobalChecksumValid = cartridge->header.globalChecksum == globalChecksum;

	for (size_t i = 0; i < cartridge->header.romSize; i++) {
		cartridge->rom[i] = bytes[i];
	}

	return cartridge;
}

void CartridgeDestroy(Cartridge* cartridge) {
	// not really necessary, but still doing the check for debugging
	if (cartridge == NULL) {
		fprintf(stderr, "Warning: CartridgeDestroy - attempted to destroy NULL");
		return;
	}

	// just free up the memory
	if (cartridge->rom) free(cartridge->rom);
	if (cartridge->ram) free(cartridge->ram);
	free(cartridge);
}

// 0x0000 - 0x3FFF: ROM bank 00
// 0x4000 - 0x7FFF: ROM bank XX
// 0xA000 - 0xBFFF: Cart RAM
const u16 ROM_BANK0_START = 0x0000;
const u16 ROM_BANK0_END = 0x3FFF;
const u16 ROM_BANKX_START = 0x4000;
const u16 ROM_BANKX_END = 0x7FFF;
const u16 CARTRIDGE_RAM_START = 0xA000;
const u16 CARTRIDGE_RAM_END = 0xBFFF;

u8 CartridgeRead(Cartridge* cart, u16 address) {
	if (address <= ROM_BANK0_END) {
		return cart->rom[address];
	}

	if (address >= ROM_BANKX_START && address <= ROM_BANKX_END) {
		// TODO: add support for mappers
		return cart->rom[address];
	}

	if (cart->header.ramSize >= 0 && address >= CARTRIDGE_RAM_START && address <= CARTRIDGE_RAM_END) {
		// TODO: add support for mappers
		return cart->ram[address - CARTRIDGE_RAM_START];
	}

	// open bus normally
	return 0;
}

void CartridgeWrite(Cartridge* cart, u16 address, u8 value) {
	if (cart->header.ramSize > 0 && address >= CARTRIDGE_RAM_START && address <= CARTRIDGE_RAM_END) {
		// TODO: add support for mappers
		cart->ram[(address - CARTRIDGE_RAM_START) % cart->header.ramSize] = value;
	}
}