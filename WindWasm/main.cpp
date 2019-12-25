#include "../WindCore/emubase.h"
#include "../WindCore/windermere.h"
#include <stdio.h>
#include <emscripten.h>
#include <SDL.h>

EmuBase *emu;
// SDL_Window *window;
SDL_Surface *surface;

static EpocKey resolveKey(SDLKey sym) {
	switch (sym) {
		case SDLK_BACKSPACE: return EStdKeyBackspace;
		case SDLK_TAB:       return EStdKeyTab;
		case SDLK_RETURN:    return EStdKeyEnter;
		case SDLK_ESCAPE:    return EStdKeyEscape;
		case SDLK_SPACE:     return EStdKeySpace;
		case SDLK_QUOTE:     return EStdKeySingleQuote;
		case SDLK_COMMA:     return EStdKeyComma;
		case SDLK_PERIOD:    return EStdKeyFullStop;
		case SDLK_0:         return (EpocKey)'0';
		case SDLK_1:         return (EpocKey)'1';
		case SDLK_2:         return (EpocKey)'2';
		case SDLK_3:         return (EpocKey)'3';
		case SDLK_4:         return (EpocKey)'4';
		case SDLK_5:         return (EpocKey)'5';
		case SDLK_6:         return (EpocKey)'6';
		case SDLK_7:         return (EpocKey)'7';
		case SDLK_8:         return (EpocKey)'8';
		case SDLK_9:         return (EpocKey)'9';
		case SDLK_a:         return (EpocKey)'A';
		case SDLK_b:         return (EpocKey)'B';
		case SDLK_c:         return (EpocKey)'C';
		case SDLK_d:         return (EpocKey)'D';
		case SDLK_e:         return (EpocKey)'E';
		case SDLK_f:         return (EpocKey)'F';
		case SDLK_g:         return (EpocKey)'G';
		case SDLK_h:         return (EpocKey)'H';
		case SDLK_i:         return (EpocKey)'I';
		case SDLK_j:         return (EpocKey)'J';
		case SDLK_k:         return (EpocKey)'K';
		case SDLK_l:         return (EpocKey)'L';
		case SDLK_m:         return (EpocKey)'M';
		case SDLK_n:         return (EpocKey)'N';
		case SDLK_o:         return (EpocKey)'O';
		case SDLK_p:         return (EpocKey)'P';
		case SDLK_q:         return (EpocKey)'Q';
		case SDLK_r:         return (EpocKey)'R';
		case SDLK_s:         return (EpocKey)'S';
		case SDLK_t:         return (EpocKey)'T';
		case SDLK_u:         return (EpocKey)'U';
		case SDLK_v:         return (EpocKey)'V';
		case SDLK_w:         return (EpocKey)'W';
		case SDLK_x:         return (EpocKey)'X';
		case SDLK_y:         return (EpocKey)'Y';
		case SDLK_z:         return (EpocKey)'Z';
		case SDLK_UP:        return EStdKeyUpArrow;
		case SDLK_DOWN:      return EStdKeyDownArrow;
		case SDLK_RIGHT:     return EStdKeyRightArrow;
		case SDLK_LEFT:      return EStdKeyLeftArrow;
		case SDLK_RSHIFT:    return EStdKeyRightShift;
		case SDLK_LSHIFT:    return EStdKeyLeftShift;
		case SDLK_RCTRL:     return EStdKeyRightCtrl;
		case SDLK_LCTRL:     return EStdKeyLeftCtrl;
		case SDLK_RALT:      return EStdKeyMenu;
		case SDLK_LALT:      return EStdKeyMenu;
		case SDLK_LSUPER:    return EStdKeyLeftFunc;
		case SDLK_RSUPER:    return EStdKeyRightFunc;
	}
	return EStdKeyNull;
}

void emuEventLoop() {
	// printf("Doing it\n");
	emu->executeUntil(emu->currentCycles() + (emu->getClockSpeed() / 64));

	uint8_t *lines[480];

	SDL_LockSurface(surface);
	int baseX = emu->getLCDOffsetX();
	int baseY = emu->getLCDOffsetY();
	int height = emu->getLCDHeight();
	for (int y = 0; y < height; y++) {
		lines[y] = (uint8_t *)surface->pixels + (surface->pitch * (y + baseY)) + (baseX * 4);
	}
	emu->readLCDIntoBuffer(lines, true);
	SDL_UnlockSurface(surface);
	SDL_Flip(surface);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
			EpocKey k = resolveKey(event.key.keysym.sym);
			if (k != EStdKeyNull)
				emu->setKeyboardKey(k, (event.key.state == SDL_PRESSED));
		} else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
			if (event.button.button == SDL_BUTTON_LEFT)
				emu->updateTouchInput(event.button.x, event.button.y, (event.button.state == SDL_PRESSED));
		} else if (event.type == SDL_MOUSEMOTION) {
			emu->updateTouchInput(event.motion.x, event.motion.y, (event.motion.state & SDL_BUTTON(1)));
		}
	}
}

int main(int argc, char **argv) {
	emu = new Windermere::Emulator;
	emu->setLogger([](const char *str) {
		printf("%s\n", str);
	});
	FILE *f = fopen("rom/5mx.bin", "rb");
	fread(emu->getROMBuffer(), 1, 10485760, f);
	fclose(f);

	if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) != 0) {
		printf("SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}
	surface = SDL_SetVideoMode(emu->getDigitiserWidth(), emu->getDigitiserHeight(), 32, SDL_SWSURFACE);
	if (surface == NULL) {
		printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
		return 1;
	}
	// window = SDL_CreateWindow(
	// 	"WindEmu",
	// 	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	// 	emu->getDigitiserWidth(), emu->getDigitiserHeight(),
	// 	0);
	// if (window == NULL) {
	// 	printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
	// 	return 1;
	// }

	emscripten_set_main_loop(&emuEventLoop, 64, 1);

	return 0;
}
