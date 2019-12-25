#pragma once
#include "emubase.h"
#include "wind_defs.h"
#include "hardware.h"
#include "etna.h"

namespace Windermere {
class Emulator : public EmuBase {
public:
    uint8_t ROM[0x1000000];
	uint8_t ROM2[0x40000];
    uint8_t MemoryBlockC0[0x800000];
    uint8_t MemoryBlockC1[0x800000];
    uint8_t MemoryBlockD0[0x800000];
    uint8_t MemoryBlockD1[0x800000];
    enum { MemoryBlockMask = 0x7FFFFF };

private:
    uint16_t pendingInterrupts = 0;
    uint16_t interruptMask = 0;
    uint32_t portValues = 0;
    uint32_t portDirections = 0;
    uint32_t pwrsr = 0x00002000; // cold start flag
    uint32_t lcdControl = 0;
    uint32_t lcdAddress = 0;
    uint32_t rtc = 0;
	uint16_t lastSSIRequest = 0;
	int ssiReadCounter = 0;

	uint32_t kScan = 0;
	uint8_t keyboardColumns[8] = {0,0,0,0,0,0,0};
	int32_t touchX = 0, touchY = 0;

    Timer tc1, tc2;
    UART uart1, uart2;
	Etna etna;
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
	void diffInterrupts(uint16_t oldval, uint16_t newval);
	uint32_t readKeyboard();

public:
	Emulator();
	uint8_t *getROMBuffer() override;
	size_t getROMSize() override;
	void loadROM(uint8_t *buffer, size_t size) override;
	void executeUntil(int64_t cycles) override;
	int32_t getClockSpeed() const override { return CLOCK_SPEED; }
	const char *getDeviceName() const override;
	int getDigitiserWidth() const override;
	int getDigitiserHeight() const override;
	int getLCDOffsetX() const override;
	int getLCDOffsetY() const override;
	int getLCDWidth() const override;
	int getLCDHeight() const override;
	void readLCDIntoBuffer(uint8_t **lines, bool is32BitOutput) const override;
	void setKeyboardKey(EpocKey key, bool value) override;
	void updateTouchInput(int32_t x, int32_t y, bool down) override;
};
}
