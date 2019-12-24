#include "clps7600.h"
#include "arm710.h"

CLPS7600::CLPS7600(ARM710 *_cpu)
{
	cpu = _cpu;
}

enum {
	PCM_BVD1 = 1,
	PCM_BVD2 = 2,
	PCM_CD1 = 4,
	PCM_CD2 = 8,
	PCM_VS1 = 0x10,
	PCM_VS2 = 0x20,
	PDREQ_L = 0x40,
	PCTL = 0x100,
	PCM_WP = 0x200,
	PCM_RDY = 0x400,
	FIFOTHLD = 0x800,
	IDLE = 0x1000,
	WR_FAIL = 0x2000,
	RD_FAIL = 0x4000,
	RESERVED = 0x8000
};

uint32_t CLPS7600::getInputLevel() const {
	uint32_t v = 0;

	if (isMemoryMode())
		v |= PCM_RDY; // we are ALWAYS ready

	return v;
}

uint32_t CLPS7600::read(uint32_t addr, ARM710::ValueSize valueSize)
{
	cpu->log("CLPS7600 read: addr=%07x size=%d pc=%08x lr=%08x", addr, (valueSize == ARM710::V32) ? 32 : 8, cpu->getRealPC(), cpu->getGPR(14));
	if (valueSize == ARM710::V32) {
		switch (addr) {
		case 0xC000000: // Interrupt Status
			return interruptStatus;
		case 0xC000400: // Interrupt Mask
			return interruptMask;
		case 0xC001C00: // Interrupt Input Level
			return getInputLevel();
		case 0xC002000: // System Interface Configuration
			return systemInterfaceConfig;
		case 0xC002400: // Card Interface Configuration
			return cardInterfaceConfig;
		case 0xC002800: // Power Management
			return powerManagement;
		case 0xC002C00: // Card Power Control
			return cardPowerControl;
		case 0xC003000: // Card Interface Timing 0A
			return cardInterfaceTiming0A;
		case 0xC003400: // Card Interface Timing 0B
			return cardInterfaceTiming0B;
		case 0xC003800: // Card Interface Timing 1A
			return cardInterfaceTiming1A;
		case 0xC003C00: // Card Interface Timing 1B
			return cardInterfaceTiming1B;
		case 0xC004000: // DMA Control
			return dmaControl;
		case 0xC004400: // Device Information
			return deviceInformation;
		default:
			cpu->log("CLPS7600 unknown register read: addr=%07x pc=%08x lr=%08x", addr, cpu->getRealPC(), cpu->getGPR(14));
			return 0xFFFFFFFF;
		}
	}
	cpu->log("unknown!!");
	return 0xFF;
}

void CLPS7600::write(uint32_t value, uint32_t addr, ARM710::ValueSize valueSize)
{
	cpu->log("CLPS7600 write: addr=%07x size=%d value=%08x pc=%08x lr=%08x", addr, (valueSize == ARM710::V32) ? 32 : 8, value, cpu->getRealPC(), cpu->getGPR(14));
	if (valueSize == ARM710::V32) {
		switch (addr) {
		case 0xC000400: // Interrupt Mask
			interruptMask = value;
			break;
		case 0xC000800: // Interrupt Clear
			break;
		case 0xC000C00: // Interrupt Output Select
			break;
		case 0xC001000: // Interrupt Reserved Register 1
			break;
		case 0xC001400: // Interrupt Reserved Register 2
			break;
		case 0xC001800: // Interrupt Reserved Register 3
			break;
		case 0xC002000: // System Interface Configuration
			systemInterfaceConfig = value;
			break;
		case 0xC002400: // Card Interface Configuration
			cardInterfaceConfig = value;
			cpu->log("PC card enabled: %s", (value & 0x400) ? "yes" : "no");
			cpu->log("PC card write protect: %s", (value & 0x200) ? "yes" : "no");
			cpu->log("PC card mode: %s", (value & 0x100) ? "i/o" : "memory");
			break;
		case 0xC002800: // Power Management
			powerManagement = value;
			break;
		case 0xC002C00: // Card Power Control
			cardPowerControl = value;
			break;
		case 0xC003000: // Card Interface Timing 0A
			cardInterfaceTiming0A = value;
			break;
		case 0xC003400: // Card Interface Timing 0B
			cardInterfaceTiming0B = value;
			break;
		case 0xC003800: // Card Interface Timing 1A
			cardInterfaceTiming1A = value;
			break;
		case 0xC003C00: // Card Interface Timing 1B
			cardInterfaceTiming1B = value;
			break;
		case 0xC004000: // DMA Control
			dmaControl = value;
			break;
		case 0xC004400: // Device Information
			deviceInformation = value;
			break;
		default:
			cpu->log("CLPS7600 unknown register write: addr=%07x value=%08x pc=%08x lr=%08x", addr, value, cpu->getRealPC(), cpu->getGPR(14));
		}
	} else {
		cpu->log("unknown write!!");
	}
}
