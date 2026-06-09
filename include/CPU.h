#ifndef NEPGB_CPU_H
#define NEPGB_CPU_H

#include "MemoryBus.h"

#define FLAG_CARRY 4
#define FLAG_HALFCARRY 5
#define FLAG_SUBTRACT 6
#define FLAG_ZERO 7

typedef struct CPU {
	union {
		struct {
			// low
			u8 c;
			// high
			u8 b;
		};
		u16 bc;
	};

	union {
		struct {
			// low
			u8 e;
			// high
			u8 d;
		};
		u16 de;
	};

	union {
		struct {
			// low
			u8 f;
			// high
			u8 a;
		};
		u16 af;
	};

	union {
		struct {
			// low
			u8 l;
			// high
			u8 h;
		};
		u16 hl;
	};

	u16 sp;
	u16 pc;

	// reference necessary for reading and writing data
	MemoryBus* bus;

	bool isInterruptEnabled;

	bool lowPower;
	bool veryLowPower;
	bool isHardLocked;

	u8 instructionByteAdvance;

	u8 cycle;
	u8 extraCycle;	// may be unused, not sure if there are cycle penalties

} CPU;

CPU* CPUCreate(MemoryBus* bus);
void CPUDestroy(CPU* cpu);

void CPUSetFlags(CPU* cpu, bool z, bool n, bool h, bool c);

void CPUPush8(CPU* cpu, u8 value);
u8 CPUPop8(CPU* cpu);

void CPUPush16(CPU* cpu, u16 value);
u16 CPUPop16(CPU* cpu);

void CPURunInstruction(CPU* cpu);

#endif	// NEPGB_CPU_H
