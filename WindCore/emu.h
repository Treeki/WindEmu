#pragma once
#include "arm.h"
#include "wind_hw.h"

class Emu {
    uint8_t ROM[0x1000000];
    uint8_t MemoryBlockC[0x800000];
    enum { MemoryBlockCMask = 0x7FFFFF };
    uint8_t MemoryBlockD[0x800000];
    enum { MemoryBlockDMask = 0x7FFFFF };

    uint32_t controlReg;
    uint32_t translationTableBase;
    uint32_t domainAccessControl;
    uint16_t pendingInterrupts = 0;
    uint16_t interruptMask = 0;
    uint32_t portValues = 0;
    uint32_t portDirections = 0;
    uint32_t pwrsr = 0x00002000; // cold start flag
    uint32_t lcdControl = 0;
    uint32_t lcdAddress = 0;
    uint32_t kScan = 0;
    uint32_t rtc = 0;

    int64_t nextTickAt = 0;
    Timer tc1, tc2;
    UART uart1, uart2;
    bool asleep = false;

    struct ARMCore cpu;

    inline bool isMMU() {
        return (controlReg & 1);
    }

    uint32_t getRTC();

    uint32_t readReg8(uint32_t reg);
    uint32_t readReg32(uint32_t reg);
    void writeReg8(uint32_t reg, uint8_t value);
    void writeReg32(uint32_t reg, uint32_t value);

public:
    uint32_t readPhys8(uint32_t physAddress);
    uint32_t readPhys16(uint32_t physAddress);
    uint32_t readPhys32(uint32_t physAddress);
    void writePhys8(uint32_t physAddress, uint8_t value);
    void writePhys16(uint32_t physAddress, uint16_t value);
    void writePhys32(uint32_t physAddress, uint32_t value);

    uint32_t virtToPhys(uint32_t virtAddress);

    uint32_t readVirt8(uint32_t virtAddress) { return readPhys8(virtToPhys(virtAddress)); }
    uint32_t readVirt16(uint32_t virtAddress) { return readPhys16(virtToPhys(virtAddress)); }
    uint32_t readVirt32(uint32_t virtAddress) { return readPhys32(virtToPhys(virtAddress)); }
    void writeVirt8(uint32_t virtAddress, uint8_t value) { writePhys8(virtToPhys(virtAddress), value); }
    void writeVirt16(uint32_t virtAddress, uint16_t value) { writePhys16(virtToPhys(virtAddress), value); }
    void writeVirt32(uint32_t virtAddress, uint32_t value) { writePhys32(virtToPhys(virtAddress), value); }

    const uint8_t *getLCDBuffer() const;

private:
    bool configured = false;
    void configure();
    void configureMemoryBindings();
    void configureCpuHandlers();

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
    int64_t currentCycles() const { return cpu.cycles; }
};
