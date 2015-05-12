#include "ROM.h"
#include <iostream>
#include <fstream>

namespace GBEmu {
	ROM::ROM()
	{
		mbc1 = false;
		mbc2 = false;
	}

	byte ROM::getromsize()
	{
		return bin[0x148];
	}

	byte ROM::getramsize()
	{
		return bin[0x149];
	}

	void ROM::loadfromfile(const char* filename)
	{
		std::fstream in(filename, std::ios::in | std::ios::binary);
		if (!in.is_open()) 
			throw std::runtime_error("rom file could not be found");

		in.seekg(0, in.end);
		size_t romsize = in.tellg();

		bin.assign(romsize, 0);
		in.seekg(0, in.beg);

		in.read((char*)bin.data(), romsize);
		// read succesfully

		switch (bin[0x147]) {
		case 1:
		case 2:
		case 3:
			mbc1 = true; break;
		case 5:
		case 6:
			mbc2 = true; break;
		default:
			break;
		}
	}

	bool ROM::ismbc1()
	{
		return mbc1;
	}

	byte ROM::getaddrvalue(word addr)
	{
		if (addr < bin.size())
			return bin[addr];
		else
			return 0;
	}

	std::string ROM::gettitle()
	{
		char out[8];
		strncpy(out, (const char*)&bin[0x134], 8);
		return out;
	}

	bool ROM::ismbc2()
	{
		return mbc2;
	}

	// return the number of ROM banks
	byte ROM::getrombankcount()
	{
		switch (bin[0x148])
		{
		case 0:
			return 2;
		case 1:
			return 4;
		case 2:
			return 8;
		case 3:
			return 16;
		case 4:
			return 32;
		case 5:
			return 64;
		case 6:
			return 128;
		case 0x52:
			return 72;
		case 0x53:
			return 80;
		case 0x54:
			return 96;
		}

		return 0;
	}

	byte ROM::getrambankcount()
	{
		switch (bin[0x149])
		{
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return 1;
		case 3:
			return 4;
		case 4:
			return 16;
		}
	}

	byte ROM::readBank(byte Bank, word relativeAddr)
	{
		return 0; // stub
	}

	void ROM::copy(int32_t start, size_t size, byte* dst)
	{
		if (size + start > bin.size()) throw std::runtime_error("out of rom bounds");
		memcpy(dst, bin.data() + start, size);
	}
}