
#pragma once

#include "types.h"
#include "MMU.h"
#include <map>

namespace GBEmu {

	// not-really-a-class
	class Z80
	{
	public:
		struct z80op {
			byte (*op)(Z80*);
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

		enum interrupt_t
		{
			int_vblank,
			int_lcdstat,
			int_timer,
			int_serial,
			int_joypad
		};

		// run an interrupt request
		void runInterrupt(interrupt_t intr);

		void callint(word addr);

		// actually execute interrupts
		void executeinterrupts();

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

		// the point of this structure is to use the accomulation of cycles to know
		// how long to wait.
		struct clock_t {
			// clock, in machine cycles.
			uint32_t machine;
		} clock;

		// assume op & 0xFF00 is the high part, and op & 0xFF is the low part. 
		// in general, if (0xFF00 & op) >> 2 == 0xCB use the extended instruction set
		// otherwise use the regular 256 instructions from the gameboy.

		byte runopcode(uint16_t op);
		void runopfromname(std::string op);

		template <class T>
		void zfset(T v) {
			if (v == 0) 
				setflag(Z80::zf); 
			else 
				resetflag(Z80::zf);
		}

		byte fetchb();
		word fetchw();
		byte getvaluepointedbyHL();
		void setvalueatHL(byte v);

		byte step();

		// system components
		MMU mmu;

		std::map<word, std::string> disassembly;
		Z80();

		double msPerCycle();
		void dumpins();
	};
}