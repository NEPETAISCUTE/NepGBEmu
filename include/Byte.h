#ifndef NEPGB_BYTE_H
#define NEPGB_BYTE_H

#define GetBit(value, index) ((((value) >> (index)) & 1))
#define SetBit(value, index) ((value) | (1 << (index)))
#define ClearBit(value, index) ((value) & ~(1 << (index)))
#define FlipBit(value, index) ((value) ^ (1 << (index)))

#define GetBits(value, index, len) (((value) >> (index)) & ((1 << (len)) - 1))

#define GetFlag(value, index) (GetBit(value, index) == 1)

#define GetHighNybble(value) (((value) & 0xF0) >> 4)
#define GetLowNybble(value) ((value) & 0x0F)

#define MakeByte(high, low) ((((high) & 0x0F) << 4) | ((low) & 0x0F))
#define MakeField(b7, b6, b5, b4, b3, b2, b1, b0) \
	(((b7) << 7) | ((b6) << 6) | ((b5) << 5) | ((b4) << 4) | ((b3) << 3) | ((b2) << 2) | ((b1) << 1) | (b0))

#define AssignBit(value, index, flag) ((flag) ? SetBit(value, index) : ClearBit(value, index))
// #define AssignBit(value, index, bit) SetFlagValue(value, index, bit == 1)

#define AssignBits(value, index, len, flags) (((value) & ~((1 << (len)) - 1)) | ((flags) & ((1 << (len)) - 1)))

#define GetLowByte(value) ((value) & 0xFF)
#define GetHighByte(value) (((value) >> 8) & 0xFF)

#define BuildU16(highByte, lowByte) ((((highByte) & 0xFF) << 8) | ((lowByte) & 0xFF))

#define BuildU2(highBit, lowBit) ((((highBit) & 1) << 1) | ((lowBit) & 1))

#endif	// NEPGB_BYTE_H
