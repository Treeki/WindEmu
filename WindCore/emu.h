#pragma once
#include "arm710a.h"
#include "wind_hw.h"
#include "etna.h"
#include <unordered_set>

class Emu : public ARM710a {
public:
    uint8_t ROM[0x1000000];
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
    uint32_t kScan = 0;
    uint32_t rtc = 0;

	int64_t passedCycles = 0;
    int64_t nextTickAt = 0;
    Timer tc1, tc2;
    UART uart1, uart2;
	Etna etna;
	bool halted = false, asleep = false;

	std::unordered_set<uint32_t> _breakpoints;

    uint32_t getRTC();

    uint32_t readReg8(uint32_t reg);
    uint32_t readReg32(uint32_t reg);
    void writeReg8(uint32_t reg, uint8_t value);
    void writeReg32(uint32_t reg, uint32_t value);

public:
	bool isPhysAddressValid(uint32_t addr) const;
	MaybeU32 readPhysical(uint32_t physAddr, ValueSize valueSize) override;
	bool writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) override;

    const uint8_t *getLCDBuffer() const;

private:
    bool configured = false;
    void configure();

    void printRegs();
    const char *identifyObjectCon(uint32_t ptr);
    void fetchStr(uint32_t str, char *buf);
    void fetchName(uint32_t obj, char *buf);
    void fetchProcessFilename(uint32_t obj, char *buf);
    void debugPC(uint32_t pc);

    uint8_t readKeyboard();
public:
    bool keyboardKeys[8*7] = {0};

public:
    Emu();
    void loadROM(const char *path);
    void dumpRAM(const char *path);
	void executeUntil(int64_t cycles);
	std::unordered_set<uint32_t> &breakpoints() { return _breakpoints; }
	uint64_t currentCycles() const { return passedCycles; }
};
