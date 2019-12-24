#pragma once
#include <stdint.h>
#include "arm710.h"

class CLPS7600
{
private:
	ARM710 *cpu;

	uint32_t interruptStatus = 0;
	uint32_t interruptMask = 0;
	uint32_t systemInterfaceConfig = 0x1F8;
	uint32_t cardInterfaceConfig = 0;
	uint32_t powerManagement = 0;
	uint32_t cardPowerControl = 0;
	uint32_t cardInterfaceTiming0A = 0x1F00;
	uint32_t cardInterfaceTiming0B = 0;
	uint32_t cardInterfaceTiming1A = 0x1F00;
	uint32_t cardInterfaceTiming1B = 0;
	uint32_t dmaControl = 0;
	uint32_t deviceInformation = 0x40;

	bool isIOMode() const { return (cardInterfaceConfig & 0x100); }
	bool isMemoryMode() const { return !(cardInterfaceConfig & 0x100); }
	uint32_t getInputLevel() const;

public:
	CLPS7600(ARM710 *_cpu);

	uint32_t read(uint32_t addr, ARM710::ValueSize valueSize);
	void write(uint32_t value, uint32_t addr, ARM710::ValueSize valueSize);
};

