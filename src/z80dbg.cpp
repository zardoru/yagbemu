#include "Z80.h"
#include "ROM.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <bitset>
#include <csignal>

GBEmu::Z80 cpu;
GBEmu::ROM rom;
bool locked = false;

void sigbreak(int sig)
{
	if (locked)
	{
		cpu.breaknextstep = true;
		std::cout << "execution interrupted\n";
	}
}

void printregs(GBEmu::Z80 &cpu)
{
	std::cout << std::hex <<
		"A = $" << std::setw(2) << (int)cpu.a << "\t" <<
		"B = $" << std::setw(2) << (int)cpu.b << std::endl <<
		"C = $" << std::setw(2) << (int)cpu.c << "\t" <<
		"D = $" << std::setw(2) << (int)cpu.d << std::endl <<
		"E = $" << std::setw(2) << (int)cpu.e << "\t" <<
		"F = $" << std::bitset<8>((int)cpu.f) << std::endl <<
		"H = $" << std::setw(2) << (int)cpu.h << "\t" <<
		"L = $" << std::setw(2) << (int)cpu.l << std::endl <<
		"PC = $" << std::setw(4) << (int)cpu.pc << std::endl;
}

void print16regs(GBEmu::Z80 &cpu)
{
	std::cout << std::hex <<
		"AF = $" << std::setw(4) << (int)cpu.getAF() << "\t" <<
		"BC = $" << std::setw(4) << (int)cpu.getBC() << "\t" <<
		"DE = $" << std::setw(4) << (int)cpu.getDE() << std::endl <<
		"HL = $" << std::setw(4) << (int)cpu.getHL() << "\t" <<
		"SP = $" << std::setw(4) << (int)cpu.getSP() << std::endl;
}

int main()
{
	int op;
	std::string cmd;
	std::cin >> std::hex;
	std::cout << std::right << std::setfill('0');

	std::cout << "yagbemu's gbzb80 debugger interface go.\n";
	printregs(cpu); print16regs(cpu);

	signal(SIGINT, sigbreak);

	while (true)
	{
		std::cin >> cmd;
		if (cmd == "op")
		{
			std::cin >> op;
			cpu.runopcode(op);
		}
		else if (cmd == "do")
		{
			std::string op;
			std::getline(std::cin, op);
			op = op.substr(1);
			cpu.runopfromname(op);
		}
		else if (cmd == "step" || cmd == "s")
		{
			cpu.step(); 
		}
		else if (cmd == "stepandinfo" || cmd == "si")
		{
			cpu.step(); printregs(cpu); print16regs(cpu);
		}
		else if (cmd == "r" || cmd == "reg" || cmd == "regs")
		{
			printregs(cpu); print16regs(cpu);
		}
		else if (cmd == "setreg") // assign a register.
		{
			std::string rg; std::cin >> rg;
			short w; std::cin >> w;
			if (rg == "a")
				cpu.a = w;
			if (rg == "b")
				cpu.b = w;
			if (rg == "c")
				cpu.c = w;
			if (rg == "d")
				cpu.d = w;
			if (rg == "e")
				cpu.e = w;
			if (rg == "h")
				cpu.h = w;
			if (rg == "l")
				cpu.l = w;
			if (rg == "f")
				cpu.f = w;
			if (rg == "sp")
				cpu.sp = w;
			if (rg == "pc")
				cpu.pc = w;
		}
		else if (cmd == "printw") {
			short addr; std::cin >> addr;
			std::cout << cpu.mmu.readw(addr);
		}
		else if (cmd == "printb") {
			short addr; std::cin >> addr;
			std::cout << (int)cpu.mmu.readb(addr);
		}
		else if (cmd == "setb") {
			short addr; std::cin >> addr;
			byte b; std::cin >> b;
			cpu.mmu.writeb(addr, b);
		}
		else if (cmd == "setw") {
			short addr; std::cin >> addr;
			word b; std::cin >> b;
			cpu.mmu.writew(addr, b);
		}
		else if (cmd == "ldrom" || cmd == "ld")
		{
			std::string fn; std::getline(std::cin, fn);
			fn = fn.substr(1);

			rom.loadfromfile(fn.c_str());
			cpu.mmu.assignrom(&rom);

			std::cout << "loaded rom " << rom.gettitle() << std::endl;
		} else if (cmd == "c" || cmd == "continue")
		{
			locked = true;
			while (cpu.step()) {
				std::cout << "\r" << std::hex << cpu.prevpc;
			}
			locked = false;
		} else if (cmd == "q")
			return 0;
		else if (cmd == "sd" || cmd == "setdump")
			cpu.dump = true;
		else if (cmd == "cd" || cmd == "cleardump") 
			cpu.dump = false;
		else if (cmd == "trace")
			cpu.dumpins();
		else if (cmd == "breakpoint")
		{
			int bp; std::cin >> bp;
			
			locked = true;
			while (cpu.pc != bp)
			{
				if (!cpu.step()) break;
			}
			locked = false;

			if (bp == cpu.pc) std::cout << "breakpoint hit" << std::endl;
		}
		else std::cout << "??\n";
		
	}
}