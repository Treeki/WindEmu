#pragma once
#include "arm710.h"
#include <unordered_set>

enum EpocKey {
	EStdKeyDial = 161,
	EStdKeyOff = 160,
	EStdKeyHelp = 159,
	EStdKeyDictaphoneRecord = 158,
	EStdKeyDictaphoneStop = 157,
	EStdKeyDictaphonePlay = 156,
	EStdKeySliderUp = 155,
	EStdKeySliderDown = 154,
	EStdKeyDecContrast = 153,
	EStdKeyIncContrast = 152,
	EStdKeyBacklightToggle = 151,
	EStdKeyBacklightOff = 150,
	EStdKeyBacklightOn = 149,
	EStdKeyMenu = 148,
	EStdKeyNkpFullStop = 147,
	EStdKeyNkp0 = 146,
	EStdKeyNkp9 = 145,
	EStdKeyNkp8 = 144,
	EStdKeyNkp7 = 143,
	EStdKeyNkp6 = 142,
	EStdKeyNkp5 = 141,
	EStdKeyNkp4 = 140,
	EStdKeyNkp3 = 139,
	EStdKeyNkp2 = 138,
	EStdKeyNkp1 = 137,
	EStdKeyNkpEnter = 136,
	EStdKeyNkpPlus = 135,
	EStdKeyNkpMinus = 134,
	EStdKeyNkpAsterisk = 133,
	EStdKeyNkpForwardSlash = 132,
	EStdKeyEquals = 131,
	EStdKeyMinus = 130,
	EStdKeySquareBracketRight = 129,
	EStdKeySquareBracketLeft = 128,
	EStdKeyHash = 127,
	EStdKeySingleQuote = 126,
	EStdKeySemiColon = 125,
	EStdKeyBackSlash = 124,
	EStdKeyForwardSlash = 123,
	EStdKeyFullStop = 122,
	EStdKeyComma = 121,
	EStdKeyXXX = 120,
	EStdKeyF24 = 119,
	EStdKeyF23 = 118,
	EStdKeyF22 = 117,
	EStdKeyF21 = 116,
	EStdKeyF20 = 115,
	EStdKeyF19 = 114,
	EStdKeyF18 = 113,
	EStdKeyF17 = 112,
	EStdKeyF16 = 111,
	EStdKeyF15 = 110,
	EStdKeyF14 = 109,
	EStdKeyF13 = 108,
	EStdKeyF12 = 107,
	EStdKeyF11 = 106,
	EStdKeyF10 = 105,
	EStdKeyF9 = 104,
	EStdKeyF8 = 103,
	EStdKeyF7 = 102,
	EStdKeyF6 = 101,
	EStdKeyF5 = 100,
	EStdKeyF4 = 99,
	EStdKeyF3 = 98,
	EStdKeyF2 = 97,
	EStdKeyF1 = 96,
	EStdKeyScrollLock = 28,
	EStdKeyNumLock = 27,
	EStdKeyCapsLock = 26,
	EStdKeyRightFunc = 25,
	EStdKeyLeftFunc = 24,
	EStdKeyRightCtrl = 23,
	EStdKeyLeftCtrl = 22,
	EStdKeyRightAlt = 21,
	EStdKeyLeftAlt = 20,
	EStdKeyRightShift = 19,
	EStdKeyLeftShift = 18,
	EStdKeyDownArrow = 17,
	EStdKeyUpArrow = 16,
	EStdKeyRightArrow = 15,
	EStdKeyLeftArrow = 14,
	EStdKeyDelete = 13,
	EStdKeyInsert = 12,
	EStdKeyPageDown = 11,
	EStdKeyPageUp = 10,
	EStdKeyEnd = 9,
	EStdKeyHome = 8,
	EStdKeyPause = 7,
	EStdKeyPrintScreen = 6,
	EStdKeySpace = 5,
	EStdKeyEscape = 4,
	EStdKeyEnter = 3,
	EStdKeyTab = 2,
	EStdKeyBackspace = 1,
	EStdKeyNull = 0
};

class EmuBase : public ARM710
{
protected:
#ifndef __EMSCRIPTEN__
	std::unordered_set<uint32_t> _breakpoints;
#endif
	int64_t passedCycles = 0;
	int64_t nextTickAt = 0;
	uint8_t readKeyboard(int kScan);

public:
	EmuBase(bool isTVersion) : ARM710(isTVersion) { }

	virtual uint8_t *getROMBuffer() = 0;
	virtual size_t getROMSize() = 0;
	virtual void loadROM(uint8_t *buffer, size_t size) = 0;
	virtual void executeUntil(int64_t cycles) = 0;
	virtual int32_t getClockSpeed() const = 0;
	virtual const char *getDeviceName() const = 0;
	virtual int getDigitiserWidth() const = 0;
	virtual int getDigitiserHeight() const = 0;
	virtual int getLCDOffsetX() const = 0;
	virtual int getLCDOffsetY() const = 0;
	virtual int getLCDWidth() const = 0;
	virtual int getLCDHeight() const = 0;
	virtual void readLCDIntoBuffer(uint8_t **lines, bool is32BitOutput) const = 0;
	virtual void setKeyboardKey(EpocKey key, bool value) = 0;
	virtual void updateTouchInput(int32_t x, int32_t y, bool down) = 0;

#ifndef __EMSCRIPTEN__
	std::unordered_set<uint32_t> &breakpoints() { return _breakpoints; }
#endif
	uint64_t currentCycles() const { return passedCycles; }
};

