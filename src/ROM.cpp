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

	byte ROM::getbankcount()
	{
		return bin[0x148];
	}

	void ROM::copy(int32_t start, size_t size, byte* dst)
	{
		if (size + start > bin.size()) throw std::runtime_error("out of rom bounds");
		memcpy(dst, bin.data() + start, size);
	}
}