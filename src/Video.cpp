#include "Video.h"

void GBEmu::Video::OnWriteLY(word _a, byte _b)
{
	line = 0;
	mmu->rawwriteb(LY_ADDR, 0);
}

byte GBEmu::Video::getSCX()
{
	return mmu->rawreadb(SCX_ADDR);
}

byte GBEmu::Video::getSCY()
{
	return mmu->rawreadb(SCY_ADDR);
}

byte GBEmu::Video::getLCDC()
{
	return mmu->rawreadb(LCDC_ADDR);
}

bool GBEmu::Video::getBGMap()
{
	return getLCDC() & 0x08; // bit 4
}

bool GBEmu::Video::getIsLCDOn()
{
	return getLCDC() & 0x80;
}

bool GBEmu::Video::getBGTile()
{
	return getLCDC() & 0x10; // fifth bit
}

byte GBEmu::Video::getTilePx(bool bank1, word tile, byte y, byte x)
{
	x = 7 - x;
	word tile_reladdr = tile * 16; // tile base address (16 bytes/tile)
	word row = y * 2; // 2 bytes per row

	word displacement = 0;
	if (bank1)
		displacement = 0x800;

	byte bit = (1 << x);
	byte bitL = (mmu->readb(tile_reladdr + row + VRAM_BASE + displacement) & bit) >> x;
	byte bitH = (mmu->readb(tile_reladdr + row + VRAM_BASE + 1 + displacement) & bit) >> x;

	byte px = (bitH << 1) | bitL;
	return px;
}

GBEmu::Pixel GBEmu::Video::getBGPalColor(byte bg)
{
	byte bgpal = mmu->rawreadb(BGPAL_ADDR);
	// first 2 bits
	byte brightness = (bgpal >> (bg * 2)) & 0x03;
	// 0 to 255, 3 to 0
	byte pxbrightness = byte(255.0 - 255.0 / 3.0 * brightness);

	return Pixel{
		pxbrightness, pxbrightness, pxbrightness, 255
	};
}

void GBEmu::Video::renderScanDebug(VIDEO_DEBUGMODE debug)
{
	word canvasoffs = line * 160;
	word tile = (line >> 3) * 20 + ((debug == TILE_B) ? 128 : 0);

	word x = 0;
	for (int i = 0; i < 160; i++) {
		auto px = getBGPalColor(getTilePx(debug == TILE_B, tile + (x / 8), line % 8, x % 8));

		canvas[canvasoffs + x] = px;
		x++;
	}
}

void GBEmu::Video::renderScanDebugBG(VIDEO_DEBUGMODE debug)
{
	word mapoffs = debug == TILE_M0 ? 0x1C00 : 0x1800;
	mapoffs += ((line + getSCY()) & 0xFF) >> 3;

	byte lineoffs = getSCX() >> 3;

	byte tile = mmu->rawreadb(VRAM_BASE + mapoffs + lineoffs);
	
	word canvasoffs = line * 160;

	word x = 0;
	for (int i = 0; i < 160; i++) {
		auto px = Pixel { tile, tile, tile, 255 };

		canvas[canvasoffs] = px;
		canvasoffs++;
		x++;
		if (x == 8) {
			x = 0;
			lineoffs = (lineoffs + 1) & 31;
			tile = mmu->rawreadb(VRAM_BASE + mapoffs + lineoffs);
		}
	}

}

// a painfully direct translation
void GBEmu::Video::renderScan()
{
	word mapoffs = getBGMap() ? 0x1C00 : 0x1800;
	mapoffs += (((line + getSCY()) & 255) >> 3) << 5;

	byte y = (line + getSCY()) & 7; 
	byte x = getSCX() & 7;

	word canvasoffs = line * 160;

	word lineoffs = getSCX() >> 3;
	char tile = mmu->rawreadb(VRAM_BASE + mapoffs + lineoffs);

	// 0 to 0x8800 if tilset #1 is enabled
	// and 128 to 0x9000.
	word wadd = 128;
	if (getBGTile() && tile < 128) {
		tile += wadd;
	}

	// draw the scanline
	for (int i = 0; i < 160; i++) {
		auto px = getBGPalColor(getTilePx(getBGTile(), tile, y, x));

		canvas[canvasoffs] = px;
		canvasoffs += 1;

		x++;
		if (x == 8) {
			x = 0;
			lineoffs = (lineoffs + 1) & 31;
			tile = mmu->rawreadb(VRAM_BASE + mapoffs + lineoffs);
			if (getBGTile() && tile < 128) { // signed tile
				tile += wadd;
			}
		}
	}
}

GBEmu::Video::Video(MMU * mem)
{
	LY = bind(&Video::OnWriteLY, this, std::placeholders::_1, std::placeholders::_2);
	
	mmu = mem;
	
	mmu->addWriteHook(LY_ADDR, &LY);
	// internalLY = 0;

	modeCounter = 0;
	mode = 0;
	line = 0;

	memset(canvas, 255, sizeof(canvas));
}

void GBEmu::Video::addRefreshHook(RefreshHook hook)
{
	OnRefresh.push_back(hook);
}

void GBEmu::Video::updateTimer(int cpuCycles, Z80 *pr, VIDEO_DEBUGMODE debug)
{
	// 4 cpu cycles = a bunch of time in Hz depending on what's up
	// we've got to set up that 144 and onward is the v-blank period
	// and "one" of the LY counter is an amount of cpu cycles
	// v-blank lasts  ~1.1 millisecs so...
	/*constexpr auto ly_per_ms = (153 - 144) / 1.1;
	auto ly_total = pr->msPerCycle() * cpuCycles / ly_per_ms;
	internalLY += ly_total;
	internalLY = fmod(internalLY, 154.0);
	
	well actually we don't need it as per below. foolish me*/

	// "_modeclock" in 
	// http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-GPU-Timings

	if (!getIsLCDOn()) return;

	modeCounter += cpuCycles;

	switch (mode) {
	case 2: // OAM access
		if (modeCounter >= 80) {
			mode = 3;
			modeCounter = 0;
		}
	case 3: // VRAM access

		// admittedly, I'd have understood that 
		// in practice it should dictate the current pixel of this scanline, or something.
		// may be important for some abusive demos.
		if (modeCounter >= 172) {
			mode = 0;
			modeCounter = 0;
			
			if (debug == TILE || debug == TILE_B)
				renderScanDebug(debug);
			else if (debug == TILE_M0 || debug == TILE_M1)
				renderScanDebugBG(debug);
			else
				renderScan();
		}
	case 0: // H-blank
		if (modeCounter >= 204) {
			modeCounter = 0;
			line++;

			// vblank
			if (line == 144) // matches LY >= 144
			{
				mode = 1;
				
				for (auto f: OnRefresh) {
					f(canvas);
				}

				pr->runInterrupt(Z80::int_vblank);
			}
			else
				mode = 2;
		}
	case 1: // V-blank
		if (modeCounter >= 456) {
			modeCounter = 0;
			line++;
			if (line > 153) {
				mode = 2;
				line = 0;
			}
		}
	}

	mmu->rawwriteb(LY_ADDR, line);
}
