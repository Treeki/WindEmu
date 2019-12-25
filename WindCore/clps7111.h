#pragma once
#include "emubase.h"
#include "clps7111_defs.h"
#include "clps7600.h"
#include "hardware.h"
#include "etna.h"

namespace CLPS7111 {
class Emulator : public EmuBase {
public:
	uint8_t ROM[0x800000];
	uint8_t ROM2[0x40000];
	uint8_t MemoryBlockC0[0x400000];
	enum { MemoryBlockMask = 0x3FFFFF };

private:
	uint16_t pendingInterrupts = 0;
	uint16_t interruptMask = 0;
	uint32_t portValues = 0;
	uint32_t portDirections = 0;
	uint32_t sysFlg1 = 0x20008000; // constant CL-PS7111 flag and cold start flag
	uint32_t lcdControl = 0;
	uint32_t lcdAddress = 0xC0000000;
	uint32_t rtc = 0;
	uint32_t rtcDiv = 0;
	uint64_t lcdPalette = 0;

	uint32_t kScan = 0;
	uint8_t keyboardColumns[7] = {0,0,0,0,0,0,0};
	uint8_t keyboardExtra = 0;

	Timer tc1, tc2;
	CLPS7600 pcCardController;
	bool halted = false, asleep = false;


	uint32_t getRTC();

	uint32_t readReg8(uint32_t reg);
	uint32_t readReg32(uint32_t reg);
	void writeReg8(uint32_t reg, uint8_t value);
	void writeReg32(uint32_t reg, uint32_t value);

public:
	MaybeU32 readPhysical(uint32_t physAddr, ValueSize valueSize) override;
	bool writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) override;

private:
	bool configured = false;
	void configure();

	const char *identifyObjectCon(uint32_t ptr);
	void fetchStr(uint32_t str, char *buf);
	void fetchName(uint32_t obj, char *buf);
	void fetchProcessFilename(uint32_t obj, char *buf);
	void debugPC(uint32_t pc);
	void diffPorts(uint32_t oldval, uint32_t newval);
	uint32_t readKeyboard() const;

public:
	Emulator();
	void loadROM(uint8_t *buffer, size_t size) override;
	void executeUntil(int64_t cycles) override;
	int32_t getClockSpeed() const override { return CLOCK_SPEED; }
	int getLCDWidth() const override;
	int getLCDHeight() const override;
	void readLCDIntoBuffer(uint8_t **lines) const override;
	void setKeyboardKey(EpocKey key, bool value) override;
};
}
