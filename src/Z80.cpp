#include "MMU.h"
#include "Z80.h"

#include <iostream>
#include <sstream>

// current issues: half carry may not be calculated correctly for the most part. it seems a lot of people skip it, anyway.
// unimplemented opcodes.
#include "z80op.inl.h"

namespace GBEmu {

	Z80::Z80()
	{
		pc = 0x0;
		prevpc = -1;
		dump = false;
		halted = false;
		stopped = false;
		interrupts = true;
		memset(&clock, 0, sizeof(clock_t));
		breaknextstep = false;
	}

	void Z80::setflag(flag_e flag)
	{
		f |= flag;
	}

	void Z80::resetflag(flag_e flag)
	{
		f &= ~flag;
	}

	bool Z80::isflagset(flag_e flag)
	{
		return f & flag;
	}

	word Z80::fetchw()
	{
		byte low = fetchb();
		byte high = fetchb();
		return packWord(high, low);
	}

	byte Z80::fetchb()
	{
		return mmu.readb(pc++);
	}

	byte Z80::getvaluepointedbyHL()
	{
		word HL = getHL();
		return mmu.readb(HL);
	}

	void Z80::setvalueatHL(byte v)
	{
		word HL = getHL();
		return mmu.writeb(HL, v);
	}

	void Z80::setAF(word v)
	{
		a = (v & 0xFF00) >> 8;
		f = v & 0xFF;
	}

	void Z80::setBC(word v)
	{
		b = (v & 0xFF00) >> 8;
		c = v & 0xFF;
	}

	void Z80::setDE(word v)
	{
		d = (v & 0xFF00) >> 8;
		e = v & 0xFF;
	}

	void Z80::setHL(word v)
	{
		h = (v & 0xFF00) >> 8;
		l = v & 0xFF;
	}

	void Z80::setSP(word v)
	{
		sp = v;
	}

	word Z80::getAF()
	{
		return (a << 8) | f;
	}

	word Z80::getBC()
	{
		return (b << 8) | c;
	}

	word Z80::getDE()
	{
		return (d << 8) | e;
	}

	word Z80::getHL()
	{
		return (h << 8) | l;
	}

	word Z80::getSP()
	{
		return sp;
	}

	byte Z80::step()
	{
		prevpc = pc;
		byte opc = fetchb();
		auto rt = runopcode(opc);

		// done with the bios!
		if (biosRunning && pc == 0x100) {
			biosRunning = false;
			mmu.cleanBIOS();
		}

		// rt = rt && !breaknextstep;

		if (breaknextstep) breaknextstep = false;
		return rt;
	}

	void Z80::runInterrupt(interrupt_t intr)
	{
		bool interruptRan = false;
		// addresses 0x40, 0x48, 0x50, 0x58 and 0x60 are interrupt addresses.

		byte interrupts = mmu.readb(0xFF0F);
		interrupts |= 1 << intr;
		mmu.writeb(0xFF0F, interrupts);
	}

	void Z80::callint(word addr)
	{
		mmu.writew(sp, pc);
		sp -= 2;
		pc = addr;

		interrupts = false;
		clock.machine += 5;
	}

	void Z80::executeinterrupts()
	{
		if (!interrupts) // they're disabled. don't process them.
			return;

		byte enabledinterrupts = mmu.readb(0xFFFF);
		byte requests = mmu.readb(0xFF0F);
		if (requests & (1 << int_vblank) && enabledinterrupts & (1 << int_vblank))
		{
			callint(0x40);
			return;
		}

		if (requests & (1 << int_lcdstat) && enabledinterrupts & (1 << int_lcdstat))
		{
			callint(0x48);
			return;
		}

		if (requests & (1 << int_timer) && enabledinterrupts & (1 << int_timer))
		{
			callint(0x50);
			return;
		}

		if (requests & (1 << int_serial) && enabledinterrupts & (1 << int_serial))
		{
			callint(0x58);
			return;
		}

		if (requests & (1 << int_joypad) && enabledinterrupts & (1 << int_joypad))
		{
			callint(0x60);
			return;
		}
	}

	void Z80::runopfromname(std::string op)
	{
		for (int i = 0; i < 256; i++)
		{
			if (ops[i].desc == op)
			{
				ops[i].op(this);
				return;
			}
		}
	}

	byte Z80::runopcode(word opc)
	{
		if (halted) return 0; // we're awaiting an interrupt

		if ((opc & 0xFF00) == 0)
		{
			if (ops[opc].op == ILLOP)
			{
				std::cout << "illegal instruction " << std::hex << opc << std::endl;
				return 0;
			}

			if (dump && opc != 0xCB && opc != 0) // skip nops
			{
				std::string desc = ops[opc].desc;
				int i;
				if ((i = desc.find_first_of("n")) != std::string::npos) {
					char buf[8];
					sprintf(buf, "$%02x", mmu.readb(pc));
					desc.replace(i, 1, buf);
				}

				if ((i = desc.find_first_of("w")) != std::string::npos) {
					char buf[8];
					sprintf(buf, "$%04x", mmu.readw(pc));
					desc.replace(i, 1, buf);
				}

				disassembly[pc] = desc;

				printf("%04x: %s\n", pc, desc.c_str());
			}

			byte c = ops[opc].op(this);
			clock.machine += c;
			return c;
		}
		else {
			if (optable2[opc & 0xFF].op == ILLOP || optable2[opc & 0xFF].op == NULL)
			{
				std::cout << "illegal instruction (0xcb) " << std::hex << (opc & 0xFF) << std::endl;
				return false;
			}

			byte c = optable2[opc & 0xFF].op(this);
			if (dump) 
			{
				std::clog << "0x" << std::hex << prevpc << ": " << optable2[opc & 0xFF].desc << std::endl;
			}
			return c;
		}

		// call outside the fetch/execute cycle. that way we've got finer-grained control over the timing.
		// executeinterrupts();
		return 0;
	}

	double Z80::msPerCycle()
	{
		constexpr auto frequency = 4.194304 * 1000000; // regular GB frequency
		return 1. / frequency * 1000.0;
	}

	void Z80::dumpins()
	{
		for (auto v : disassembly)
		{
			std::clog << v.second;
		}
	}
}