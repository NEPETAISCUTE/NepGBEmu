#include "Joypad.h"

#include "MemoryBus.h"

void JoypadTick(MemoryBus* bus) {}

u8 JoypadRegRead(JoypadReg* reg) {
	u8 keys = ~(MakeField(IsKeyDown(KEY_ENTER), IsKeyDown(KEY_SPACE), IsKeyDown(KEY_J), IsKeyDown(KEY_K), IsKeyDown(KEY_S), IsKeyDown(KEY_W),
						  IsKeyDown(KEY_A), IsKeyDown(KEY_D)));
	switch (reg->selector) {
		case 0b00: return GetHighNybble(keys) & GetLowNybble(keys);	 // TODO: implement bitwise
		case 0b10: return GetLowNybble(keys);						 // read D-Pad
		case 0b01: return GetHighNybble(keys);						 // read buttons
		case 0b11: return 0xF;										 // just 0xF, no buttons being read
	}
	// should never happen
	return 0xF;
}

void JoypadRegWrite(JoypadReg* reg, u8 value) { reg->selector = GetBits(value, 4, 2); }