#include "MMU.h"
#include "Z80.h"

#include <iostream>
#include <sstream>
#include <map>

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
		return tow(high, low);
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

	bool Z80::step()
	{
		prevpc = pc;
		byte opc = fetchb();
		bool rt = runopcode(opc);

		// done with the bios!
		if (biosRunning && pc == 0x100) {
			biosRunning = false;
			mmu.cleanBIOS();
		}

		rt = rt && !breaknextstep;

		if (breaknextstep) breaknextstep = false;
		return rt;
	}

	void Z80::runinterrupts()
	{

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

	std::map<word, std::string> opd;

	bool Z80::runopcode(word opc)
	{
		if ((opc & 0xFF00) == 0)
		{
			if (ops[opc].op == ILLOP)
			{
				std::cout << "illegal instruction " << std::hex << opc << std::endl;
				return false;
			}

			ops[opc].op(this);

			std::stringstream ss;
			ss << "0x" << std::hex << prevpc << ": " << ops[opc].desc << std::endl;
			opd[pc] = ss.str();

			if (dump && opc != 0xCB && opc != 0) // skip nops
			{
				std::clog << ss.str();
			}
		}
		else {
			if (optable2[opc & 0xFF].op == ILLOP || optable2[opc & 0xFF].op == NULL)
			{
				std::cout << "illegal instruction (0xcb) " << std::hex << (opc & 0xFF) << std::endl;
				return false;
			}

			optable2[opc & 0xFF].op(this);
			if (dump) 
			{
				std::clog << "0x" << std::hex << prevpc << ": " << optable2[opc & 0xFF].desc << std::endl;
			}
		}

		return true;
	}

	void Z80::dumpins()
	{
		for (auto v : opd)
		{
			std::clog << v.second;
		}
	}
}