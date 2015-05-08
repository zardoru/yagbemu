#pragma once

#include "types.h"

namespace GBEmu {
	class ROM{
		vbyte bin;

		bool mbc1, mbc2;
	public:
		ROM();
		void copy(int32_t start, size_t size, byte* dst);

		std::string gettitle();

		byte getromsize();
		byte getramsize();
		// returns size of rom banks. 
		size_t getbanksize();
		byte getbankcount();

		bool ismbc2();

		// dst needs to be large enough.
		void copybank(int bank, byte* dst);

		byte getaddrvalue(word addr);

		// loads rom binary from file
		void loadfromfile(const char* filename);
	};
}