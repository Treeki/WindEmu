#pragma once
#include "arm710.h"
#include <unordered_set>

class EmuBase : public ARM710
{
protected:
	std::unordered_set<uint32_t> _breakpoints;
	int64_t passedCycles = 0;
	int64_t nextTickAt = 0;
	uint8_t readKeyboard(int kScan);

public:
	EmuBase(bool isTVersion) : ARM710(isTVersion) { }

	virtual void loadROM(uint8_t *buffer, size_t size) = 0;
	virtual void executeUntil(int64_t cycles) = 0;
	virtual int32_t getClockSpeed() const = 0;
	virtual int getLCDWidth() const = 0;
	virtual int getLCDHeight() const = 0;
	virtual void readLCDIntoBuffer(uint8_t **lines) const = 0;

	std::unordered_set<uint32_t> &breakpoints() { return _breakpoints; }
	uint64_t currentCycles() const { return passedCycles; }

	bool keyboardKeys[8*7] = {0};
};

