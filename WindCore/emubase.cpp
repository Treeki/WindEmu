#include "emubase.h"

uint8_t EmuBase::readKeyboard(int kScan) {
	uint8_t val = 0;
	if (kScan & 8) {
		// Select one keyboard
		int whichColumn = kScan & 7;
		for (int i = 0; i < 7; i++)
			if (keyboardKeys[whichColumn * 7 + i])
				val |= (1 << i);
	} else if (kScan == 0) {
		// Report all columns combined
		// EPOC's keyboard driver relies on this...
		for (int i = 0; i < 8*7; i++)
			if (keyboardKeys[i])
				val |= (1 << (i % 7));
	}
	return val;
}
