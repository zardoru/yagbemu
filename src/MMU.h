#include "types.h"
#include "ROM.h"

#pragma once

namespace GBEmu {
	
	typedef function<byte(word addr)> ReadHook;
	typedef function<void(word addr, byte val)> WriteHook;

	class MMU {
	private:
		map<word, vector<ReadHook*>> ReadHooks;
		map<word, vector<WriteHook*>> WriteHooks;

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
		word swappedrambank, swappedrombank;
		size_t romsize;

		void doRomBanking(byte v);
		void doMBCstuff(word addr, byte val);

		bool inbios;
		bool MBC1, MBC2;
		bool rambankEnabled;

		enum {
			rombanking, // 8kB ram, 2mB ROM
			rambanking // 32kB ram, 512kB Rom
		} memoryModel;
	public:

		MMU();

		void addReadHook(word addr, ReadHook* func);
		void addWriteHook(word addr, WriteHook* func);

		void setMBC1(bool nv);
		void writeb(word addr, byte val);
		void writew(word addr, word val);
		byte readb(word addr) const;
		word readw(word addr) const;

		// straight up from our structure
		byte rawreadb(word addr) const;
		word rawreadw(word addr) const;
		void rawwriteb(word addr, byte b);
		void rawwritew(word addr, word w);

		void assignrom(ROM* rom);
		void cleanBIOS();
	};
}