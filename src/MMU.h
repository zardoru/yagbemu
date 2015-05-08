#include "types.h"
#include "ROM.h"

#pragma once

namespace GBEmu {
	class MMU {
		union {
			struct {
				byte rombank0[kB(16)]; // ROM bank 0 0x0000 -> 0x3FFF
				byte srombank[kB(16)]; // switchable ROM bank 0x4000 -> 0x7FFF
				byte vram[kB(8)]; // video ram 0x8000 -> 0x9FFF
				byte sram[kB(8)]; // switchable ram bank 0xA000 -> 0xBFFF

				byte internalram8[kB(8)]; // 0xC000 -> DFFF. writing to this = writing to echo8 right next
				byte internalramecho8[kB(8)]; // E000 -> FDFF

				byte spriteattr[0xA0]; // FE00 -> FE9F (OAM)
				byte emptyunusuable[0x60]; // FEA0 -> FEFF
				byte ioports[0x4C]; // FF00 -> FF4B
				byte emptyunusable2[0x35];// FF4C -> FF7F

				byte internalram[0x7F]; // FF80 -> FFFF
			};
			byte memory[0x10000];
		} ram;

		byte rambanks;
		ROM* rom;
		word swappedrambank;
		size_t romsize;

		bool inbios;
	public:
		MMU();
		void writeb(word addr, byte val);
		void writew(word addr, word val);
		byte readb(word addr);
		word readw(word addr);

		void assignrom(ROM* rom);
		void cleanBIOS();
	};
}