
#pragma once

#include "types.h"
#include "MMU.h"

namespace GBEmu {

	// not-really-a-class
	class Z80
	{
	public:
		struct z80op {
			void(*op)(Z80*);
			char desc[16];
		};

		// registers
		union {
			struct { byte a, b, c, d, e, h, l; };
			struct { byte A, B, C, D, E, H, L; };
		};

		// flags
		byte f;

		// flags
		enum flag_e {
			// zero flag. set if last op was 0.
			zf = 0x80,

			// operation flag. set if last operation was substraction.
			opf = 0x40,

			// half carry flag. set if last operation overflowed past 15.
			hcf = 0x20,

			// carry flag. set if last operation overflowed above 255 or under 0.
			cf = 0x10
		};

		void setflag(flag_e flag);
		void resetflag(flag_e flag);
		bool isflagset(flag_e flag);

		// 16 bit ops over registers
		void setAF(word v);
		void setBC(word v);
		void setDE(word v);
		void setHL(word v);
		void setSP(word v);
		word getAF();
		word getBC();
		word getDE();
		word getHL();
		word getSP(); // for completeness

		void runinterrupts();

		// program counter
		word prevpc;
		word pc;

		// stack pointer
		word sp;

		// do dump
		bool dump;

		// do we return false next step?
		bool breaknextstep;

		// awaiting interrupt?
		bool halted;

		// stopped cpu/gpu until input?
		bool stopped;

		// interrupts enabled?
		bool interrupts;

		// bios is running
		bool biosRunning;

		struct clock_t {
			// clock, in machine cycles.
			uint32_t machine;

			// clock in clock cycles.
			uint32_t clock;
		} clock;

		// assume op & 0xFF00 is the high part, and op & 0xFF is the low part. 
		// in general, if (0xFF00 & op) >> 2 == 0xCB use the extended instruction set
		// otherwise use the regular 256 instructions from the gameboy.

		bool runopcode(uint16_t op);
		void runopfromname(std::string op);

		byte fetchb();
		word fetchw();
		byte getvaluepointedbyHL();
		void setvalueatHL(byte v);

		bool step();

		// system components
		MMU mmu;

		Z80();
		void dumpins();
	};
}