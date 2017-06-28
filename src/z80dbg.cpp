#include "Z80.h"
#include "ROM.h"
#include "Video.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <csignal>

GBEmu::Z80 cpu;
GBEmu::ROM rom;
GBEmu::Video vid(&cpu.mmu);
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

/*int main()
{
	int op;
	std::string cmd;
	std::cout << std::right << std::setfill('0');

	std::cout << "yagbemu's gbz80 debugger interface start.\n";
	printregs(cpu); print16regs(cpu);

	std::cin.unsetf(std::ios::dec);
	std::cin.unsetf(std::ios::hex);
	std::cin.unsetf(std::ios::oct);

	while (true)
	{
		using std::hex;
		signal(SIGINT, sigbreak);
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
			vid.updateTimer(cpu.step(), &cpu); 
		}
		else if (cmd == "stepandinfo" || cmd == "si")
		{
			vid.updateTimer(cpu.step(), &cpu); 
			
			printregs(cpu); print16regs(cpu);
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
			std::cout << cpu.mmu.readw(addr) << std::endl;
		}
		else if (cmd == "printb") {
			short addr; std::cin >> addr;
			std::cin.clear();
			std::cin.ignore();
			std::cout << (int)cpu.mmu.readb(addr) << std::endl;
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
			byte c = 0;
			while (c = cpu.step()) { vid.updateTimer(c, &cpu); }
			locked = false;
		} else if (cmd == "q")
			return 0;
		else if (cmd == "sd" || cmd == "setdump")
			cpu.dump = true;
		else if (cmd == "cd" || cmd == "cleardump") 
			cpu.dump = false;
		else if (cmd == "trace")
			cpu.dumpins();
		else if (cmd == "breakpoint" || cmd == "b")
		{
			int bp; std::cin >> bp;
			
			locked = true;
			while (cpu.pc != bp)
			{
				byte c;
				if (! (c = cpu.step()) ) break;
				vid.updateTimer(c, &cpu);
			}
			locked = false;

			if (bp == cpu.pc) std::cout << "breakpoint hit" << std::endl;
		}
		else if (cmd == "disassemble")
		{
			std::cout << "writing to dis.txt\n";
			std::fstream out("dis.txt", std::ios::out);
			for (auto v : cpu.disassembly) {
				out << v.second << "\n";
			}
		}
		else std::cout << "unknown command\n";
		
	}
}*/