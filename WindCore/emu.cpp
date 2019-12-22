#include "emu.h"
#include "wind.h"
#include "wind_hw.h"
#include <time.h>
#include "common.h"


#define INCLUDE_BANK1

Emu::Emu() : etna(this) {
}


uint32_t Emu::getRTC() {
    return time(nullptr) - 946684800;
}


uint32_t Emu::readReg8(uint32_t reg) {
	if ((reg & 0xF00) == 0x600) {
		return uart1.readReg8(reg & 0xFF);
	} else if ((reg & 0xF00) == 0x700) {
		return uart2.readReg8(reg & 0xFF);
	} else if (reg == TC1CTRL) {
		return tc1.config;
	} else if (reg == TC2CTRL) {
		return tc2.config;
	} else if (reg == PADR) {
        return readKeyboard();
	} else if (reg == PBDR) {
		return (portValues >> 16) & 0xFF;
	} else if (reg == PCDR) {
		return (portValues >> 8) & 0xFF;
	} else if (reg == PDDR) {
		return portValues & 0xFF;
	} else if (reg == PADDR) {
		return (portDirections >> 24) & 0xFF;
	} else if (reg == PBDDR) {
		return (portDirections >> 16) & 0xFF;
	} else if (reg == PCDDR) {
		return (portDirections >> 8) & 0xFF;
	} else if (reg == PDDDR) {
		return portDirections & 0xFF;
	} else {
//		printf("RegRead8 unknown:: pc=%08x lr=%08x reg=%03x\n", getGPR(15)-4, getGPR(14), reg);
		return 0xFF;
	}
}
uint32_t Emu::readReg32(uint32_t reg) {
	if (reg == LCDCTL) {
		printf("LCD control read pc=%08x lr=%08x !!!\n", getGPR(15), getGPR(14));
		return lcdControl;
	} else if (reg == LCDST) {
		printf("LCD state read pc=%08x lr=%08x !!!\n", getGPR(15), getGPR(14));
		return 0xFFFFFFFF;
	} else if (reg == PWRSR) {
//		printf("!!! PWRSR read pc=%08x lr=%08x !!!\n", getGPR(15), getGPR(14));
		return pwrsr;
	} else if (reg == INTSR) {
		return pendingInterrupts & interruptMask;
	} else if (reg == INTRSR) {
		return pendingInterrupts;
	} else if (reg == INTENS) {
		return interruptMask;
	} else if ((reg & 0xF00) == 0x600) {
		return uart1.readReg32(reg & 0xFF);
	} else if ((reg & 0xF00) == 0x700) {
		return uart2.readReg32(reg & 0xFF);
	} else if (reg == TC1VAL) {
		return tc1.value;
	} else if (reg == TC2VAL) {
		return tc2.value;
    } else if (reg == SSSR) {
//		printf("!!! SSSR kludge! !!!\n");
		return 0;
    } else if (reg == RTCDRL) {
//        uint16_t v = getRTC() & 0xFFFF;
        uint16_t v = rtc & 0xFFFF;
//        printf("RTCDRL: %04x\n", v);
        return v;
    } else if (reg == RTCDRU) {
//        uint16_t v = getRTC() >> 16;
        uint16_t v = rtc >> 16;
//        printf("RTCDRU: %04x\n", v);
        return v;
    } else if (reg == KSCAN) {
        return kScan;
    } else {
//		printf("RegRead32 unknown:: pc=%08x lr=%08x reg=%03x\n", getGPR(15)-4, getGPR(14), reg);
		return 0xFFFFFFFF;
	}
}

void Emu::writeReg8(uint32_t reg, uint8_t value) {
	if ((reg & 0xF00) == 0x600) {
		uart1.writeReg8(reg & 0xFF, value);
	} else if ((reg & 0xF00) == 0x700) {
		uart2.writeReg8(reg & 0xFF, value);
	} else if (reg == TC1CTRL) {
		tc1.config = value;
	} else if (reg == TC2CTRL) {
		tc2.config = value;
	} else if (reg == PADR) {
		uint32_t oldPorts = portValues;
		portValues &= 0x00FFFFFF;
		portValues |= (uint32_t)value << 24;
		diffPorts(oldPorts, portValues);
	} else if (reg == PBDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFF00FFFF;
		portValues |= (uint32_t)value << 16;
		if ((portValues & 0x10000) && !(oldPorts & 0x10000))
			etna.setPromBit0High();
		else if (!(portValues & 0x10000) && (oldPorts & 0x10000))
			etna.setPromBit0Low();
		if ((portValues & 0x20000) && !(oldPorts & 0x20000))
			etna.setPromBit1High();
		diffPorts(oldPorts, portValues);
	} else if (reg == PCDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFFFF00FF;
		portValues |= (uint32_t)value << 8;
		diffPorts(oldPorts, portValues);
	} else if (reg == PDDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFFFFFF00;
		portValues |= (uint32_t)value;
		diffPorts(oldPorts, portValues);
	} else if (reg == PADDR) {
		portDirections &= 0x00FFFFFF;
		portDirections |= (uint32_t)value << 24;
	} else if (reg == PBDDR) {
		portDirections &= 0xFF00FFFF;
		portDirections |= (uint32_t)value << 16;
	} else if (reg == PCDDR) {
		portDirections &= 0xFFFF00FF;
		portDirections |= (uint32_t)value << 8;
	} else if (reg == PDDDR) {
		portDirections &= 0xFFFFFF00;
		portDirections |= (uint32_t)value;
    } else if (reg == KSCAN) {
        kScan = value;
    } else {
//		printf("RegWrite8 unknown:: pc=%08x reg=%03x value=%02x\n", getGPR(15)-4, reg, value);
	}
}
void Emu::writeReg32(uint32_t reg, uint32_t value) {
	if (reg == LCDCTL) {
		printf("LCD: ctl write %08x\n", value);
		lcdControl = value;
	} else if (reg == LCD_DBAR1) {
		printf("LCD: address write %08x\n", value);
        lcdAddress = value;
	} else if (reg == LCDT0) {
		printf("LCD: horz timing write %08x\n", value);
	} else if (reg == LCDT1) {
		printf("LCD: vert timing write %08x\n", value);
	} else if (reg == LCDT2) {
		printf("LCD: clocks write %08x\n", value);
	} else if (reg == INTENS) {
//		diffInterrupts(interruptMask, interruptMask | value);
		interruptMask |= value;
	} else if (reg == INTENC) {
//		diffInterrupts(interruptMask, interruptMask &~ value);
		interruptMask &= ~value;
	} else if (reg == HALT) {
		halted = true;
	// BLEOI = 0x410,
	// MCEOI = 0x414,
	} else if (reg == TEOI) {
		pendingInterrupts &= ~(1 << TINT);
	// TEOI = 0x418,
	// STFCLR = 0x41C,
	// E2EOI = 0x420,
	} else if ((reg & 0xF00) == 0x600) {
		uart1.writeReg32(reg & 0xFF, value);
	} else if ((reg & 0xF00) == 0x700) {
		uart2.writeReg32(reg & 0xFF, value);
	} else if (reg == TC1LOAD) {
		tc1.load(value);
	} else if (reg == TC1EOI) {
		pendingInterrupts &= ~(1 << TC1OI);
	} else if (reg == TC2LOAD) {
		tc2.load(value);
	} else if (reg == TC2EOI) {
		pendingInterrupts &= ~(1 << TC2OI);
	} else {
//		printf("RegWrite32 unknown:: pc=%08x reg=%03x value=%08x\n", getGPR(15)-4, reg, value);
	}
}

bool Emu::isPhysAddressValid(uint32_t physAddress) const {
	uint8_t region = (physAddress >> 24) & 0xF1;
	switch (region) {
	case 0: return true;
	case 0x80: return (physAddress <= 0x80000FFF);
	case 0xC0: return true;
	case 0xC1: return true;
	case 0xD0: return true;
	case 0xD1: return true;
	default: return false;
	}
}


MaybeU32 Emu::readPhysical(uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;
	if (valueSize == V8) {
		if (region == 0)
			return ROM[physAddr & 0xFFFFFF];
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			return etna.readReg8(physAddr & 0xFFF);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			return readReg8(physAddr & 0xFFF);
		else if (region == 0xC0)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
#ifdef INCLUDE_BANK1
		else if (region == 0xC1)
			return MemoryBlockC1[physAddr & MemoryBlockMask];
#endif
		else if (region == 0xD0)
			return MemoryBlockD0[physAddr & MemoryBlockMask];
#ifdef INCLUDE_BANK1
		else if (region == 0xD1)
			return MemoryBlockD1[physAddr & MemoryBlockMask];
#endif
		else if (region >= 0xC0)
			return 0xFF; // just throw accesses to unmapped RAM away
	} else {
		uint32_t result;
		if (region == 0)
			LOAD_32LE(result, physAddr & 0xFFFFFF, ROM);
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			result = etna.readReg32(physAddr & 0xFFF);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			result = readReg32(physAddr & 0xFFF);
		else if (region == 0xC0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
		else if (region == 0xC1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC1);
#endif
		else if (region == 0xD0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
		else if (region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD1);
#endif
		else if (region >= 0xC0)
			return 0xFFFFFFFF; // just throw accesses to unmapped RAM away
		else
			return {};
		return result;
	}

	return {};
}

bool Emu::writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;
	if (valueSize == V8) {
		if (region == 0xC0)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
#ifdef INCLUDE_BANK1
		else if (region == 0xC1)
			MemoryBlockC1[physAddr & MemoryBlockMask] = (uint8_t)value;
#endif
		else if (region == 0xD0)
			MemoryBlockD0[physAddr & MemoryBlockMask] = (uint8_t)value;
#ifdef INCLUDE_BANK1
		else if (region == 0xD1)
			MemoryBlockD1[physAddr & MemoryBlockMask] = (uint8_t)value;
#endif
		else if (region >= 0xC0)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			etna.writeReg8(physAddr & 0xFFF, value);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			writeReg8(physAddr & 0xFFF, value);
		else
			return false;
	} else {
		uint8_t region = (physAddr >> 24) & 0xF1;
		if (region == 0xC0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
		else if (region == 0xC1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC1);
#endif
		else if (region == 0xD0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
		else if (region == 0xD1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD1);
#endif
		else if (region >= 0xC0)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			etna.writeReg32(physAddr & 0xFFF, value);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			writeReg32(physAddr & 0xFFF, value);
		else
			return false;
	}
	return true;
}



void Emu::configure() {
	if (configured) return;
	configured = true;

	uart1.cpu = this;
	uart2.cpu = this;
	memset(&tc1, 0, sizeof(tc1));
	memset(&tc2, 0, sizeof(tc1));

	nextTickAt = TICK_INTERVAL;
	rtc = getRTC();

	setProcessorID(0x41807100);
	reset();
}

void Emu::loadROM(const char *path) {
	FILE *f = fopen(path, "rb");
	fread(ROM, 1, sizeof(ROM), f);
	fclose(f);
}

void Emu::executeUntil(int64_t cycles) {
	if (!configured)
		configure();

	while (!asleep && passedCycles < cycles) {
		if (passedCycles >= nextTickAt) {
			// increment RTCDIV
			if ((pwrsr & 0x3F) == 0x3F) {
				rtc++;
				pwrsr &= ~0x3F;
			} else {
				pwrsr++;
			}

			nextTickAt += TICK_INTERVAL;
			pendingInterrupts |= (1<<TINT);
		}
		if (tc1.tick(passedCycles))
			pendingInterrupts |= (1<<TC1OI);
		if (tc2.tick(passedCycles))
			pendingInterrupts |= (1<<TC2OI);

		if ((pendingInterrupts & interruptMask & FIQ_INTERRUPTS) != 0)
			requestFIQ();
		if ((pendingInterrupts & interruptMask & IRQ_INTERRUPTS) != 0)
			requestIRQ();

		// what's running?
		if (halted) {
			// keep the clock moving
			passedCycles++;
		} else {
			if (auto v = virtToPhys(getGPR(15) - 0xC); v.has_value() && instructionReady())
				debugPC(v.value());
			passedCycles += tick();

			uint32_t new_pc = getGPR(15) - 0xC;
			if (_breakpoints.find(new_pc) != _breakpoints.end()) {
				log("⚠️ Breakpoint triggered at %08x!\n", new_pc);
				return;
			}
		}
	}
}

void Emu::dumpRAM(const char *path) {
	FILE *f = fopen(path, "wb");
	fwrite(MemoryBlockC0, 1, sizeof(MemoryBlockC0), f);
	fwrite(MemoryBlockC1, 1, sizeof(MemoryBlockC1), f);
	fwrite(MemoryBlockD0, 1, sizeof(MemoryBlockD0), f);
	fwrite(MemoryBlockD1, 1, sizeof(MemoryBlockD1), f);
	fclose(f);
}



void Emu::printRegs() {
	printf("R00:%08x R01:%08x R02:%08x R03:%08x\n", getGPR(0), getGPR(1), getGPR(2), getGPR(3));
	printf("R04:%08x R05:%08x R06:%08x R07:%08x\n", getGPR(4), getGPR(5), getGPR(6), getGPR(7));
	printf("R08:%08x R09:%08x R10:%08x R11:%08x\n", getGPR(8), getGPR(9), getGPR(10), getGPR(11));
	printf("R12:%08x R13:%08x R14:%08x R15:%08x\n", getGPR(12), getGPR(13), getGPR(14), getGPR(15));
//    printf("cpsr=%08x spsr=%08x\n", cpu.cpsr.packed, cpu.spsr.packed);
}

const char *Emu::identifyObjectCon(uint32_t ptr) {
	if (ptr == readVirtualDebug(0x80000980, V32).value()) return "process";
	if (ptr == readVirtualDebug(0x80000984, V32).value()) return "thread";
	if (ptr == readVirtualDebug(0x80000988, V32).value()) return "chunk";
//	if (ptr == readVirtualDebug(0x8000098C, V32).value()) return "semaphore";
//	if (ptr == readVirtualDebug(0x80000990, V32).value()) return "mutex";
	if (ptr == readVirtualDebug(0x80000994, V32).value()) return "logicaldevice";
	if (ptr == readVirtualDebug(0x80000998, V32).value()) return "physicaldevice";
	if (ptr == readVirtualDebug(0x8000099C, V32).value()) return "channel";
	if (ptr == readVirtualDebug(0x800009A0, V32).value()) return "server";
//	if (ptr == readVirtualDebug(0x800009A4, V32).value()) return "unk9A4"; // name always null
	if (ptr == readVirtualDebug(0x800009AC, V32).value()) return "library";
//	if (ptr == readVirtualDebug(0x800009B0, V32).value()) return "unk9B0"; // name always null
//	if (ptr == readVirtualDebug(0x800009B4, V32).value()) return "unk9B4"; // name always null
	return NULL;
}

void Emu::fetchStr(uint32_t str, char *buf) {
	if (str == 0) {
		strcpy(buf, "<NULL>");
		return;
	}
	int size = readVirtualDebug(str, V32).value();
	for (int i = 0; i < size; i++) {
		buf[i] = readVirtualDebug(str + 4 + i, V8).value();
	}
	buf[size] = 0;
}

void Emu::fetchName(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x10, V32).value(), buf);
}

void Emu::fetchProcessFilename(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x3C, V32).value(), buf);
}

void Emu::debugPC(uint32_t pc) {
	char objName[1000];
	if (pc == 0x2CBC4) {
		// CObjectCon::AddL()
		uint32_t container = getGPR(0);
		uint32_t obj = getGPR(1);
		const char *wut = identifyObjectCon(container);
		if (wut) {
			fetchName(obj, objName);
			if (strcmp(wut, "process") == 0) {
				char procName[1000];
				fetchProcessFilename(obj, procName);
				log("OBJS: added %s at %08x <%s> <%s>", wut, obj, objName, procName);
			} else {
				log("OBJS: added %s at %08x <%s>", wut, obj, objName);
			}
		}
	}

	if (pc == 0x6D8) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
		log("KERNEL MMU SECTION: v:%08x p:%08x size:%08x idx:%02x",
			virtAddr, physAddr, regionSize, btIndex);
	}
	if (pc == 0x710) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
		uint32_t pageTableA = getGPR(4);
		uint32_t pageTableB = getGPR(5);
		log("KERNEL MMU PAGES: v:%08x p:%08x size:%08x idx:%02x tableA:%08x tableB:%08x",
			virtAddr, physAddr, regionSize, btIndex, pageTableA, pageTableB);
	}
}


const uint8_t *Emu::getLCDBuffer() const {
	if ((lcdAddress >> 24) == 0xC0)
		return &MemoryBlockC0[lcdAddress & MemoryBlockMask];
	else
		return nullptr;
}


uint8_t Emu::readKeyboard() {
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
