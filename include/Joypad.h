#ifndef NEPGB_JOYPAD_H
#define NEPGB_JOYPAD_H

#include "common.h"

typedef struct MemoryBus MemoryBus;	 // forward declaring it, because include

typedef struct JoypadReg {
	u8 selector;
} JoypadReg;

void JoypadTick(MemoryBus* bus);
u8 JoypadRegRead(JoypadReg* reg);
void JoypadRegWrite(JoypadReg* reg, u8 value);

#endif	// NEPGB_JOYPAD_H