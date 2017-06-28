#pragma once

#include "Z80.h"

namespace GBEmu {
	const word LY_ADDR = 0xFF44;
	const word SCX_ADDR = 0xFF43;
	const word SCY_ADDR = 0xFF42;
	const word LCDC_ADDR = 0xFF40;
	const word BGPAL_ADDR = 0xFF47;

	const word VRAM_BASE = 0x8000;

	const word CANVAS_WIDTH = 160;
	const word CANVAS_HEIGHT = 144;
	const uint32_t CANVAS_SIZE = 160 * 144;

	enum VIDEO_DEBUGMODE {
		NORMAL,
		TILE,
		TILE_B,
		TILE_M0,
		TILE_M1
	};

	typedef union {
		struct {
			byte r;
			byte g;
			byte b;
			byte a;
		};
		unsigned int val;
	} Pixel;

	typedef function<void(const Pixel* px)> RefreshHook;

	class Video {
		MMU *mmu;

		void OnWriteLY(word _a, byte _b);

		Pixel canvas[CANVAS_SIZE];

		// double internalLY;
		WriteHook LY; // FF44

		int modeCounter;
		int mode;
		byte line;

		byte getSCX();
		byte getSCY();
		
		byte getLCDC();
		bool getBGMap();
		bool getIsLCDOn();

		bool getBGTile();

		byte getTilePx(bool bank1, word tile, byte y, byte x);
		Pixel getBGPalColor(byte bg);

		void renderScan();
		void renderScanDebug(VIDEO_DEBUGMODE debug);
		void renderScanDebugBG(VIDEO_DEBUGMODE debug);
		vector<RefreshHook> OnRefresh;
	public:
		Video(MMU *mem);
		void addRefreshHook(RefreshHook hook);
		void updateTimer(int cpuCycles, Z80 *pr, VIDEO_DEBUGMODE debug = NORMAL);
	};
}