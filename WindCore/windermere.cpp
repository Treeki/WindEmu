#include "windermere.h"
#include "wind_defs.h"
#include "hardware.h"
#include <time.h>
#include "common.h"


//#define INCLUDE_D
//#define INCLUDE_BANK1

namespace Windermere {
Emulator::Emulator() : EmuBase(true), etna(this) {
}


uint32_t Emulator::getRTC() {
    return time(nullptr) - 946684800;
}


uint32_t Emulator::readReg8(uint32_t reg) {
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
uint32_t Emulator::readReg32(uint32_t reg) {
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
	} else if (reg == SSDR) {
		// as per 5000A7B0 in 5mx rom
		uint16_t ssiValue = 0;
		switch (lastSSIRequest) {
//		case 0x9093: ssiValue = (uint16_t)(1156 - (touchY * 3.96)); break;
//		case 0xD0D3: ssiValue = (uint16_t)(2819 - (touchX * 3.91)); break;
		case 0x9093: ssiValue = (uint16_t)(1156 - (touchY * 3.96)); break;
		case 0xD0D3: ssiValue = (uint16_t)(1276 + (touchX * 3.91)); break;
		case 0xA4A4: ssiValue = 3100; break; // MainBattery
		case 0xE4E4: ssiValue = 3100; break; // BackupBattery
		}

		uint32_t ret = 0;
		if (ssiReadCounter == 4) ret = (ssiValue >> 5) & 0x7F;
		if (ssiReadCounter == 5) ret = (ssiValue << 3) & 0xF8;
		ssiReadCounter++;
		if (ssiReadCounter == 6) ssiReadCounter = 0;

		// by hardware we should be clearing SSEOTI here, i think
		// but we just leave it on to simplify things
		return ret;
	} else if (reg == SSSR) {
		return 0;
    } else if (reg == RTCDRL) {
        uint16_t v = rtc & 0xFFFF;
//        printf("RTCDRL: %04x\n", v);
        return v;
    } else if (reg == RTCDRU) {
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

void Emulator::writeReg8(uint32_t reg, uint8_t value) {
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
void Emulator::writeReg32(uint32_t reg, uint32_t value) {
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
	} else if (reg == SSDR) {
		if (value != 0)
			lastSSIRequest = (lastSSIRequest >> 8) | (value & 0xFF00);
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

MaybeU32 Emulator::readPhysical(uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;
	if (valueSize == V8) {
		if (region == 0)
			return ROM[physAddr & 0xFFFFFF];
		else if (region == 0x10)
			return ROM2[physAddr & 0x3FFFF];
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			return etna.readReg8(physAddr & 0xFFF);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			return readReg8(physAddr & 0xFFF);
#if defined(INCLUDE_BANK1)
		else if (region == 0xC0)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
		else if (region == 0xC1)
			return MemoryBlockC1[physAddr & MemoryBlockMask];
		else if (region == 0xD0)
			return MemoryBlockD0[physAddr & MemoryBlockMask];
		else if (region == 0xD1)
			return MemoryBlockD1[physAddr & MemoryBlockMask];
#elif defined(INCLUDE_D)
		else if (region == 0xC0 || region == 0xC1)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
		else if (region == 0xD0 || region == 0xD1)
			return MemoryBlockD0[physAddr & MemoryBlockMask];
#else
		else if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
#endif
		else if (region >= 0xC0)
			return 0xFF; // just throw accesses to unmapped RAM away
	} else {
		uint32_t result;
		if (region == 0)
			LOAD_32LE(result, physAddr & 0xFFFFFF, ROM);
		else if (region == 0x10)
			LOAD_32LE(result, physAddr & 0x3FFFF, ROM2);
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			result = etna.readReg32(physAddr & 0xFFF);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			result = readReg32(physAddr & 0xFFF);
#if defined(INCLUDE_BANK1)
		else if (region == 0xC0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xC1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC1);
		else if (region == 0xD0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD0);
		else if (region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD1);
#elif defined(INCLUDE_D)
		else if (region == 0xC0 || region == 0xC1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xD0 || region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD0);
#else
		else if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
#endif
		else if (region >= 0xC0)
			return 0xFFFFFFFF; // just throw accesses to unmapped RAM away
		else
			return {};
		return result;
	}

	return {};
}

bool Emulator::writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;
	if (valueSize == V8) {
#if defined(INCLUDE_BANK1)
		if (region == 0xC0)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xC1)
			MemoryBlockC1[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD0)
			MemoryBlockD0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD1)
			MemoryBlockD1[physAddr & MemoryBlockMask] = (uint8_t)value;
#elif defined(INCLUDE_D)
		if (region == 0xC0 || region == 0xC1)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD0 || region == 0xD1)
			MemoryBlockD0[physAddr & MemoryBlockMask] = (uint8_t)value;
#else
		if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
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
#if defined(INCLUDE_BANK1)
		if (region == 0xC0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xC1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC1);
		else if (region == 0xD0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD0);
		else if (region == 0xD1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD1);
#elif defined(INCLUDE_D)
		if (region == 0xC0 || region == 0xC1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xD0 || region == 0xD1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD0);
#else
		if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0x01)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
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



void Emulator::configure() {
	if (configured) return;
	configured = true;

	srand(1000);

	uart1.cpu = this;
	uart2.cpu = this;
	memset(&tc1, 0, sizeof(tc1));
	memset(&tc2, 0, sizeof(tc1));
	tc1.clockSpeed = CLOCK_SPEED;
	tc2.clockSpeed = CLOCK_SPEED;

	nextTickAt = TICK_INTERVAL;
	rtc = getRTC();

	reset();
}

void Emulator::loadROM(uint8_t *buffer, size_t size) {
	memcpy(ROM, buffer, min(size, sizeof(ROM)));
}

void Emulator::executeUntil(int64_t cycles) {
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

		if ((pendingInterrupts & interruptMask & FIQ_INTERRUPTS) != 0 && canAcceptFIQ()) {
			requestFIQ();
			halted = false;
		}
		if ((pendingInterrupts & interruptMask & IRQ_INTERRUPTS) != 0 && canAcceptIRQ()) {
			requestIRQ();
			halted = false;
		}

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
				log("⚠️ Breakpoint triggered at %08x!", new_pc);
				return;
			}
			if (new_pc >= 0x80000000 && new_pc <= 0x90000000) {
				log("BAD PC %08x!!", new_pc);
				logPcHistory();
				return;
			}
		}
	}
}


const char *Emulator::identifyObjectCon(uint32_t ptr) {
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

void Emulator::fetchStr(uint32_t str, char *buf) {
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

void Emulator::fetchName(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x10, V32).value(), buf);
}

void Emulator::fetchProcessFilename(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x3C, V32).value(), buf);
}

void Emulator::debugPC(uint32_t pc) {
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

	if (pc == 0x1576C) {
		uint32_t rawEvent = getGPR(0);
		uint32_t evtType = readVirtualDebug(rawEvent, V32).value_or(0);
		uint32_t evtTick = readVirtualDebug(rawEvent + 4, V32).value_or(0);
		uint32_t evtParamA = readVirtualDebug(rawEvent + 8, V32).value_or(0);
		uint32_t evtParamB = readVirtualDebug(rawEvent + 0xC, V32).value_or(0);
		const char *n = "???";
		switch (evtType) {
		case 0: n = "ENone"; break;
		case 1: n = "EPointerMove"; break;
		case 2: n = "EPointerSwitchOn"; break;
		case 3: n = "EKeyDown"; break;
		case 4: n = "EKeyUp"; break;
		case 5: n = "ERedraw"; break;
		case 6: n = "ESwitchOn"; break;
		case 7: n = "EActive"; break;
		case 8: n = "EInactive"; break;
		case 9: n = "EUpdateModifiers"; break;
		case 10: n = "EButton1Down"; break;
		case 11: n = "EButton1Up"; break;
		case 12: n = "EButton2Down"; break;
		case 13: n = "EButton2Up"; break;
		case 14: n = "EButton3Down"; break;
		case 15: n = "EButton3Up"; break;
		case 16: n = "ESwitchOff"; break;
		}
		log("EVENT %s: tick=%d params=%08x,%08x", n, evtTick, evtParamA, evtParamB);
	}
}


const char *Emulator::getDeviceName() const { return "Series 5mx"; }
int Emulator::getDigitiserWidth()  const { return 695; }
int Emulator::getDigitiserHeight() const { return 280; }
int Emulator::getLCDOffsetX()      const { return 45; }
int Emulator::getLCDOffsetY()      const { return 5; }
int Emulator::getLCDWidth()        const { return 640; }
int Emulator::getLCDHeight()       const { return 240; }

void Emulator::readLCDIntoBuffer(uint8_t **lines) const {
	if ((lcdAddress >> 24) == 0xC0) {
		const uint8_t *lcdBuf = &MemoryBlockC0[lcdAddress & MemoryBlockMask];
		int width = 640, height = 240;

		// fetch palette
		int bpp = 1 << (lcdBuf[1] >> 4);
		int ppb = 8 / bpp;
		uint16_t palette[16];
		for (int i = 0; i < 16; i++)
			palette[i] = lcdBuf[i*2] | ((lcdBuf[i*2+1] << 8) & 0xF00);

		// build our image out
		int lineWidth = (width * bpp) / 8;
		for (int y = 0; y < height; y++) {
			int lineOffs = 0x20 + (lineWidth * y);
			for (int x = 0; x < width; x++) {
				uint8_t byte = lcdBuf[lineOffs + (x / ppb)];
				int shift = (x & (ppb - 1)) * bpp;
				int mask = (1 << bpp) - 1;
				int palIdx = (byte >> shift) & mask;
				int palValue = palette[palIdx];

				palValue |= (palValue << 4);
				lines[y][x] = palValue ^ 0xFF;
			}
		}
	}
}


void Emulator::diffPorts(uint32_t oldval, uint32_t newval) {
	uint32_t changes = oldval ^ newval;
	if (changes & 1) log("PRT codec enable: %d", newval&1);
	if (changes & 2) log("PRT audio amp enable: %d", newval&2);
	if (changes & 4) log("PRT lcd power: %d", newval&4);
	if (changes & 8) log("PRT etna door: %d", newval&8);
	if (changes & 0x10) log("PRT sled: %d", newval&0x10);
	if (changes & 0x20) log("PRT pump pwr2: %d", newval&0x20);
	if (changes & 0x40) log("PRT pump pwr1: %d", newval&0x40);
	if (changes & 0x80) log("PRT etna err: %d", newval&0x80);
	if (changes & 0x100) log("PRT rs-232 rts: %d", newval&0x100);
	if (changes & 0x200) log("PRT rs-232 dtr toggle: %d", newval&0x200);
	if (changes & 0x400) log("PRT disable power led: %d", newval&0x400);
	if (changes & 0x800) log("PRT enable uart1: %d", newval&0x800);
	if (changes & 0x1000) log("PRT lcd backlight: %d", newval&0x1000);
	if (changes & 0x2000) log("PRT enable uart0: %d", newval&0x2000);
	if (changes & 0x4000) log("PRT dictaphone: %d", newval&0x4000);
// PROM read process makes this super spammy in stdout
//	if (changes & 0x10000) log("PRT EECS: %d", newval&0x10000);
//	if (changes & 0x20000) log("PRT EECLK: %d", newval&0x20000);
	if (changes & 0x40000) log("PRT contrast0: %d", newval&0x40000);
	if (changes & 0x80000) log("PRT contrast1: %d", newval&0x80000);
	if (changes & 0x100000) log("PRT contrast2: %d", newval&0x100000);
	if (changes & 0x200000) log("PRT contrast3: %d", newval&0x200000);
	if (changes & 0x400000) log("PRT case open: %d", newval&0x400000);
	if (changes & 0x800000) log("PRT etna cf power: %d", newval&0x800000);
}

void Emulator::diffInterrupts(uint16_t oldval, uint16_t newval) {
	uint16_t changes = oldval ^ newval;
	if (changes & 1) log("INTCHG external=%d", newval & 1);
	if (changes & 2) log("INTCHG lowbat=%d", newval & 2);
	if (changes & 4) log("INTCHG watchdog=%d", newval & 4);
	if (changes & 8) log("INTCHG mediachg=%d", newval & 8);
	if (changes & 0x10) log("INTCHG codec=%d", newval & 0x10);
	if (changes & 0x20) log("INTCHG ext1=%d", newval & 0x20);
	if (changes & 0x40) log("INTCHG ext2=%d", newval & 0x40);
	if (changes & 0x80) log("INTCHG ext3=%d", newval & 0x80);
	if (changes & 0x100) log("INTCHG timer1=%d", newval & 0x100);
	if (changes & 0x200) log("INTCHG timer2=%d", newval & 0x200);
	if (changes & 0x400) log("INTCHG rtcmatch=%d", newval & 0x400);
	if (changes & 0x800) log("INTCHG tick=%d", newval & 0x800);
	if (changes & 0x1000) log("INTCHG uart1=%d", newval & 0x1000);
	if (changes & 0x2000) log("INTCHG uart2=%d", newval & 0x2000);
	if (changes & 0x4000) log("INTCHG lcd=%d", newval & 0x4000);
	if (changes & 0x8000) log("INTCHG spi=%d", newval & 0x8000);
}


uint32_t Emulator::readKeyboard() {
	if (kScan & 8) {
		// Select one keyboard
		return keyboardColumns[kScan & 7];
	} else if (kScan == 0) {
		// Report all columns combined
		uint8_t val = 0;
		for (int i = 0; i < 8; i++)
			val |= keyboardColumns[i];
		return val;
	} else {
		return 0;
	}
}

void Emulator::setKeyboardKey(EpocKey key, bool value) {
	int idx = -1;
#define KEY(column, bit) idx = (column << 8) | (1 << bit); break

	switch ((int)key) {
	case EStdKeyDictaphoneRecord: KEY(0, 6);
	case '1':                     KEY(0, 5);
	case '2':                     KEY(0, 4);
	case '3':                     KEY(0, 3);
	case '4':                     KEY(0, 2);
	case '5':                     KEY(0, 1);
	case '6':                     KEY(0, 0);

	case EStdKeyDictaphonePlay:   KEY(1, 6);
	case '7':                     KEY(1, 5);
	case '8':                     KEY(1, 4);
	case '9':                     KEY(1, 3);
	case '0':                     KEY(1, 2);
	case EStdKeyBackspace:        KEY(1, 1);
	case EStdKeySingleQuote:      KEY(1, 0);

	case EStdKeyEscape:           KEY(2, 6);
	case 'Q':                     KEY(2, 5);
	case 'W':                     KEY(2, 4);
	case 'E':                     KEY(2, 3);
	case 'R':                     KEY(2, 2);
	case 'T':                     KEY(2, 1);
	case 'Y':                     KEY(2, 0);

	case EStdKeyMenu:             KEY(3, 6);
	case 'U':                     KEY(3, 5);
	case 'I':                     KEY(3, 4);
	case 'O':                     KEY(3, 3);
	case 'P':                     KEY(3, 2);
	case 'L':                     KEY(3, 1);
	case EStdKeyEnter:            KEY(3, 0);

	case EStdKeyLeftCtrl:         KEY(4, 6);
	case EStdKeyTab:              KEY(4, 5);
	case 'A':                     KEY(4, 4);
	case 'S':                     KEY(4, 3);
	case 'D':                     KEY(4, 2);
	case 'F':                     KEY(4, 1);
	case 'G':                     KEY(4, 0);

	case EStdKeyLeftFunc:         KEY(5, 6);
	case 'H':                     KEY(5, 5);
	case 'J':                     KEY(5, 4);
	case 'K':                     KEY(5, 3);
	case 'M':                     KEY(5, 2);
	case EStdKeyFullStop:         KEY(5, 1);
	case EStdKeyDownArrow:        KEY(5, 0);

	case EStdKeyRightShift:       KEY(6, 6);
	case 'Z':                     KEY(6, 5);
	case 'X':                     KEY(6, 4);
	case 'C':                     KEY(6, 3);
	case 'V':                     KEY(6, 2);
	case 'B':                     KEY(6, 1);
	case 'N':                     KEY(6, 0);

	case EStdKeyLeftShift:        KEY(7, 6);
	case EStdKeyDictaphoneStop:   KEY(7, 5);
	case EStdKeySpace:            KEY(7, 4);
	case EStdKeyUpArrow:          KEY(7, 3);
	case EStdKeyComma:            KEY(7, 2);
	case EStdKeyLeftArrow:        KEY(7, 1);
	case EStdKeyRightArrow:       KEY(7, 0);
	}

	if (idx >= 0) {
		if (value)
			keyboardColumns[idx >> 8] |= (idx & 0xFF);
		else
			keyboardColumns[idx >> 8] &= ~(idx & 0xFF);
	}
}

void Emulator::updateTouchInput(int32_t x, int32_t y, bool down) {
	pendingInterrupts &= ~(1 << EINT3);
	if (down)
		pendingInterrupts |= (1 << EINT3);
	touchX = x;
	touchY = y;
}

}
