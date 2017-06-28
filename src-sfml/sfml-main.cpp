#include <SFML/Graphics.hpp>

#include <sstream>
#include "Z80.h"
#include "Video.h"

int main() {
	sf::RenderWindow wnd(sf::VideoMode(160 * 4, 144 * 4), "YAGBEMU");

	sf::Texture canvas;
	sf::Sprite screen;

	canvas.create(GBEmu::CANVAS_WIDTH, GBEmu::CANVAS_HEIGHT);
	screen.setTexture(canvas);
	screen.setScale(4, 4);
	GBEmu::Z80 cpu;
	GBEmu::Video vid(&cpu.mmu);
	GBEmu::ROM rom;
	rom.loadfromfile("t.gb");
	cpu.mmu.assignrom(&rom);

	sf::Font fnt;
	fnt.loadFromFile("fnt.ttf");
	sf::Text notif;
	notif.setFont(fnt);

	// set the character size
	notif.setCharacterSize(24);
	notif.setFillColor(sf::Color::Red);

	bool update = false;
	vid.addRefreshHook([&](const GBEmu::Pixel *p) {
		update = true;
		canvas.update((sf::Uint8*)p, GBEmu::CANVAS_WIDTH, GBEmu::CANVAS_HEIGHT, 0, 0);
	});

	int vid_debug = 0;
	while (wnd.isOpen()) {

		while (!update) {
			sf::Event evt;
			while (wnd.pollEvent(evt)) {
				if (evt.type == sf::Event::Closed) return 0;
				if (evt.type == sf::Event::KeyPressed && evt.key.code == sf::Keyboard::A) {
					vid_debug++;
					vid_debug %= 5;
					std::stringstream ss;
					ss << "YAGBEMU debug video mode: " << vid_debug;
					wnd.setTitle(ss.str());
				}
			}

			byte c = cpu.step();
			vid.updateTimer(c, &cpu, (GBEmu::VIDEO_DEBUGMODE)vid_debug);
			cpu.executeinterrupts();
		}

		wnd.clear();
		wnd.draw(screen);
		
		std::stringstream ss;
		ss << (int)cpu.mmu.readb(GBEmu::SCY_ADDR);
		notif.setString(ss.str());
		wnd.draw(notif);


		wnd.display();

		
		update = false;
	}
}