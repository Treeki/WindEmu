#include "clps7111.h"
#include "clps7111_defs.h"
#include "hardware.h"
#include <time.h>
#include "common.h"


namespace CLPS7111 {
Emulator::Emulator() : EmuBase(false), pcCardController(this) {
}


uint32_t Emulator::getRTC() {
	return time(nullptr) - 946684800;
}



uint32_t Emulator::readReg8(uint32_t reg) {
	if (reg == PADR) {
		return ((portValues >> 24) & 0x80) | (readKeyboard() & 0x7F);
	} else if (reg == PBDR) {
		return ((portValues >> 16) & 0x0F) | (keyboardExtra << 4);
	} else if (reg == PDDR) {
		return (portValues >> 8) & 0xFF;
	} else if (reg == PEDR) {
		return portValues & 0xFF;
	} else if (reg == PADDR) {
		return (portDirections >> 24) & 0xFF;
	} else if (reg == PBDDR) {
		return (portDirections >> 16) & 0xFF;
	} else if (reg == PDDDR) {
		return (portDirections >> 8) & 0xFF;
	} else if (reg == PEDDR) {
		return portDirections & 0xFF;
	} else {
		log("RegRead8 unknown:: pc=%08x lr=%08x reg=%03x", getRealPC(), getGPR(14), reg);
		return 0xFF;
	}
}
uint32_t Emulator::readReg32(uint32_t reg) {
	if (reg == SYSCON1) {
		uint32_t flg = 0;
		if (tc1.config & Timer::PERIODIC) flg |= 0x10;
		if (tc1.config & Timer::MODE_512KHZ) flg |= 0x20;
		if (tc2.config & Timer::PERIODIC) flg |= 0x40;
		if (tc2.config & Timer::MODE_512KHZ) flg |= 0x80;
		flg |= (kScan & 0xF);
		return flg;
	} else if (reg == SYSFLG1) {
		uint32_t flg = sysFlg1;
		flg |= (rtcDiv << 16);
		// maybe set more stuff?
		return flg;
	} else if (reg == INTSR1) {
		return pendingInterrupts & 0xFFFF;
	} else if (reg == INTMR1) {
		return interruptMask & 0xFFFF;
	} else if (reg == LCDCON) {
		return lcdControl;
	} else if (reg == TC1D) {
		return tc1.value;
	} else if (reg == TC2D) {
		return tc2.value;
	} else if (reg == RTCDR) {
		return rtc;
	} else if (reg == PALLSW) {
		return lcdPalette & 0xFFFFFFFF;
	} else if (reg == PALMSW) {
		return lcdPalette >> 32;
	} else if (reg == SYSCON2) {
		return 0;
	} else if (reg == SYSFLG2) {
		return 0;
	} else if (reg == INTSR2) {
		return pendingInterrupts >> 16;
	} else if (reg == INTMR2) {
		return interruptMask >> 16;
	} else {
		log("RegRead32 unknown:: pc=%08x lr=%08x reg=%03x", getRealPC(), getGPR(14), reg);
		return 0xFFFFFFFF;
	}
}

void Emulator::writeReg8(uint32_t reg, uint8_t value) {
	if (reg == PADR) {
		uint32_t oldPorts = portValues;
		portValues &= 0x00FFFFFF;
		portValues |= (uint32_t)value << 24;
		diffPorts(oldPorts, portValues);
	} else if (reg == PBDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFF00FFFF;
		portValues |= (uint32_t)value << 16;
//		if ((portValues & 0x10000) && !(oldPorts & 0x10000))
//			etna.setPromBit0High();
//		else if (!(portValues & 0x10000) && (oldPorts & 0x10000))
//			etna.setPromBit0Low();
//		if ((portValues & 0x20000) && !(oldPorts & 0x20000))
//			etna.setPromBit1High();
		diffPorts(oldPorts, portValues);
	} else if (reg == PDDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFFFF00FF;
		portValues |= (uint32_t)value << 8;
		diffPorts(oldPorts, portValues);
	} else if (reg == PEDR) {
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
	} else if (reg == PDDDR) {
		portDirections &= 0xFFFF00FF;
		portDirections |= (uint32_t)value << 8;
	} else if (reg == PEDDR) {
		portDirections &= 0xFFFFFF00;
		portDirections |= (uint32_t)value;
	} else if (reg == FRBADDR) {
		log("LCD: address write %08x", value << 28);
		lcdAddress = value << 28;
	} else {
		log("RegWrite8 unknown:: pc=%08x reg=%03x value=%02x", getRealPC(), reg, value);
	}
}
void Emulator::writeReg32(uint32_t reg, uint32_t value) {
	if (reg == SYSCON1) {
		kScan = value & 0xF;
		tc1.config = Timer::ENABLED; // always on with PS-7111!
		if (value & 0x10) tc1.config |= Timer::PERIODIC;
		if (value & 0x20) tc1.config |= Timer::MODE_512KHZ;
		tc2.config = Timer::ENABLED;
		if (value & 0x40) tc2.config |= Timer::PERIODIC;
		if (value & 0x80) tc2.config |= Timer::MODE_512KHZ;
	} else if (reg == INTMR1) {
		interruptMask &= 0xFFFF0000;;
		interruptMask |= (value & 0xFFFF);
	} else if (reg == LCDCON) {
		log("LCD: ctl write %08x", value);
		lcdControl = value;
	} else if (reg == TC1D) {
		tc1.load(value);
	} else if (reg == TC2D) {
		tc2.load(value);
	} else if (reg == RTCDR) {
		rtc = value;
	} else if (reg == PALLSW) {
		lcdPalette &= 0xFFFFFFFF00000000;
		lcdPalette |= value;
	} else if (reg == PALMSW) {
		lcdPalette &= 0x00000000FFFFFFFF;
		lcdPalette |= (uint64_t)value << 32;
	} else if (reg == HALT) {
		halted = true;
	// BLEOI = 0x410,
	// MCEOI = 0x414,
	} else if (reg == TEOI) {
		pendingInterrupts &= ~(1 << TINT);
	// TEOI = 0x418,
	// STFCLR = 0x41C,
	// E2EOI = 0x420,
	} else if (reg == TC1EOI) {
		pendingInterrupts &= ~(1 << TC1OI);
	} else if (reg == TC2EOI) {
		pendingInterrupts &= ~(1 << TC2OI);
	} else if (reg == SYSCON2) {
		log("SysCon2 write: %08x", value);
	} else if (reg == INTMR2) {
		interruptMask &= 0xFFFF;
		interruptMask |= (value << 16);
	} else if (reg == KBDEOI) {
		pendingInterrupts &= ~(1 << KBDINT);
	} else {
		log("RegWrite32 unknown:: pc=%08x reg=%03x value=%08x", getRealPC(), reg, value);
	}
}

MaybeU32 Emulator::readPhysical(uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 28);
	if (valueSize == V8) {
		if (region == 0)
			return ROM[physAddr & 0xFFFFFF];
		else if (region == 1)
			return ROM2[physAddr & 0x3FFFF];
		else if (region == 4)
			return pcCardController.read(physAddr & 0xFFFFFFF, V8);
		else if (region == 8 && physAddr <= 0x80001FFF)
			return readReg8(physAddr & 0x1FFF);
		else if (region == 0xC)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
		else if (region > 0xC)
			return 0xFF; // just throw accesses to unmapped RAM away
	} else {
		uint32_t result;
		if (region == 0)
			LOAD_32LE(result, physAddr & 0xFFFFFF, ROM);
		else if (region == 1)
			LOAD_32LE(result, physAddr & 0x3FFFF, ROM2);
		else if (region == 4)
			result = pcCardController.read(physAddr & 0xFFFFFFF, V32);
		else if (region == 8 && physAddr <= 0x80001FFF)
			result = readReg32(physAddr & 0x1FFF);
		else if (region == 0xC)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region > 0xC)
			return 0xFFFFFFFF; // just throw accesses to unmapped RAM away
		else
			return {};
		return result;
	}

	return {};
}

bool Emulator::writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 28);
	if (valueSize == V8) {
		if (region == 0xC)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region > 0xC)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 4)
			pcCardController.write(value, physAddr & 0xFFFFFFF, V8);
		else if (region == 8 && physAddr <= 0x80001FFF)
			writeReg8(physAddr & 0x1FFF, value);
		else
			return false;
	} else {
		if (region == 0xC)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region > 0xC)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 4)
			pcCardController.write(value, physAddr & 0xFFFFFFF, V32);
		else if (region == 8 && physAddr <= 0x80001FFF)
			writeReg32(physAddr & 0x1FFF, value);
		else
			return false;
	}
	return true;
}



void Emulator::configure() {
	if (configured) return;
	configured = true;

	srand(1000);

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
			if (rtcDiv == 0x3F) {
				rtc++;
				rtcDiv = 0;
			} else {
				rtcDiv++;
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
	if (ptr == readVirtualDebug(0x80000880, V32).value()) return "process";
	if (ptr == readVirtualDebug(0x80000884, V32).value()) return "thread";
	if (ptr == readVirtualDebug(0x80000888, V32).value()) return "chunk";
//	if (ptr == readVirtualDebug(0x8000088C, V32).value()) return "semaphore";
//	if (ptr == readVirtualDebug(0x80000890, V32).value()) return "mutex";
	if (ptr == readVirtualDebug(0x80000894, V32).value()) return "logicaldevice";
	if (ptr == readVirtualDebug(0x80000898, V32).value()) return "physicaldevice";
	if (ptr == readVirtualDebug(0x8000089C, V32).value()) return "channel";
	if (ptr == readVirtualDebug(0x800008A0, V32).value()) return "server";
//	if (ptr == readVirtualDebug(0x800008A4, V32).value()) return "unk8A4"; // name always null
	if (ptr == readVirtualDebug(0x800008AC, V32).value()) return "library";
//	if (ptr == readVirtualDebug(0x800008B0, V32).value()) return "unk8B0"; // name always null
//	if (ptr == readVirtualDebug(0x800008B4, V32).value()) return "unk8B4"; // name always null
	return "???";
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
	if (pc == 0x32304) {
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

	if (pc == 0x634) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
		log("KERNEL MMU SECTION: v:%08x p:%08x size:%08x idx:%02x",
			virtAddr, physAddr, regionSize, btIndex);
	}
	if (pc == 0x66C) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
		uint32_t pageTableA = getGPR(4);
		uint32_t pageTableB = getGPR(5);
		log("KERNEL MMU PAGES: v:%08x p:%08x size:%08x idx:%02x tableA:%08x tableB:%08x",
			virtAddr, physAddr, regionSize, btIndex, pageTableA, pageTableB);
	}
	if (pc == 0x15070) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t regionSize = getGPR(2);
		uint32_t a = getGPR(3);
		log("DPlatChunkHw MAPPING: v:%08x p:%08x size:%08x arg:%08x",
			virtAddr, physAddr, regionSize, a);
	}
}


int Emulator::getLCDWidth() const {
	return 320;
}
int Emulator::getLCDHeight() const {
	return 200;
}
void Emulator::readLCDIntoBuffer(uint8_t **lines) const {
	if (lcdAddress == 0xC0000000) {
		int width = 320, height = 200;
		int bpp = 1;
		if (lcdControl & 0x40000000) bpp = 2;
		if (lcdControl & 0x80000000) bpp = 4;
		int ppb = 8 / bpp;

		// build our image out
		int lineWidth = (width * bpp) / 8;
		for (int y = 0; y < height; y++) {
			int lineOffs = lineWidth * y;
			for (int x = 0; x < width; x++) {
				uint8_t byte = MemoryBlockC0[lineOffs + (x / ppb)];
				int shift = (x & (ppb - 1)) * bpp;
				int mask = (1 << bpp) - 1;
				int palIdx = (byte >> shift) & mask;
				int palValue;
				if (bpp == 1)
					palValue = palIdx * 255;
				else
					palValue = (lcdPalette >> (palIdx * 4)) & 0xF;

				palValue |= (palValue << 4);
				lines[y][x] = palValue ^ 0xFF;
			}
		}
	}
}



void Emulator::diffPorts(uint32_t oldval, uint32_t newval) {
	uint32_t changes = oldval ^ newval;
	if (changes & 1) log("PRT E0: %d", newval&1);
	if (changes & 2) log("PRT E1: %d", newval&2);
	if (changes & 4) log("PRT E2: %d", newval&4);
	if (changes & 0x100) log("PRT D0: %d", newval&0x100);
	if (changes & 0x200) log("PRT D1: %d", newval&0x200);
	if (changes & 0x400) log("PRT D2: %d", newval&0x400);
	if (changes & 0x800) log("PRT D3: %d", newval&0x800);
	if (changes & 0x1000) log("PRT D4: %d", newval&0x1000);
	if (changes & 0x2000) log("PRT D5: %d", newval&0x2000);
	if (changes & 0x4000) log("PRT D6: %d", newval&0x4000);
	if (changes & 0x8000) log("PRT D7: %d", newval&0x8000);
	if (changes & 0x10000) log("PRT B0: %d", newval&0x10000);
	if (changes & 0x20000) log("PRT B1: %d", newval&0x20000);
	if (changes & 0x40000) log("PRT B2: %d", newval&0x40000);
	if (changes & 0x80000) log("PRT B3: %d", newval&0x80000);
	if (changes & 0x100000) log("PRT B4: %d", newval&0x100000);
	if (changes & 0x200000) log("PRT B5: %d", newval&0x200000);
	if (changes & 0x400000) log("PRT B6: %d", newval&0x400000);
	if (changes & 0x800000) log("PRT B7: %d", newval&0x800000);
}


uint32_t Emulator::readKeyboard() const {
	if (kScan & 8) {
		// Select one keyboard
		if ((kScan & 7) < 7)
			return keyboardColumns[kScan & 7];
		else
			return 0;
	} else if (kScan == 0) {
		// Report all columns combined
		uint8_t val = 0;
		for (int i = 0; i < 7; i++)
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
	case '1':                KEY(0, 0);
	case '2':                KEY(1, 0);
	case '3':                KEY(2, 0);
	case '4':                KEY(3, 0);
	case '5':                KEY(4, 0);
	case '6':                KEY(5, 0);
	case '7':                KEY(6, 0);

	case '8':                KEY(0, 1);
	case '9':                KEY(1, 1);
	case '0':                KEY(2, 1);
	case 'P':                KEY(3, 1);
	case EStdKeySingleQuote: KEY(4, 1);
	case EStdKeyEnter:       KEY(5, 1);
	case EStdKeyBackspace:   KEY(6, 1);

	case EStdKeyEscape:      KEY(0, 2);
	case 'Q':                KEY(1, 2);
	case 'W':                KEY(2, 2);
	case 'E':                KEY(3, 2);
	case 'R':                KEY(4, 2);
	case 'T':                KEY(5, 2);
	case 'Y':                KEY(6, 2);

	case 'U':                KEY(0, 3);
	case 'J':                KEY(1, 3);
	case 'I':                KEY(2, 3);
	case 'K':                KEY(3, 3);
	case 'O':                KEY(4, 3);
	case 'L':                KEY(5, 3);
	case EStdKeyUpArrow:     KEY(6, 3);

	case EStdKeyTab:         KEY(0, 4);
	case 'A':                KEY(1, 4);
	case 'S':                KEY(2, 4);
	case 'D':                KEY(3, 4);
	case 'F':                KEY(4, 4);
	case 'G':                KEY(5, 4);
	case 'H':                KEY(6, 4);

	case EStdKeySpace:       KEY(0, 5);
	case EStdKeyComma:       KEY(1, 5);
	case 'M':                KEY(2, 5);
	case EStdKeyFullStop:    KEY(3, 5);
	case EStdKeyLeftArrow:   KEY(4, 5);
	case EStdKeyDownArrow:   KEY(5, 5);
	case EStdKeyRightArrow:  KEY(6, 5);

	case 'Z':                KEY(0, 6);
	case 'X':                KEY(1, 6);
	case EStdKeyMenu:        KEY(2, 6);
	case 'C':                KEY(3, 6);
	case 'V':                KEY(4, 6);
	case 'B':                KEY(5, 6);
	case 'N':                KEY(6, 6);

	case EStdKeyLeftShift:   KEY(8, 0);
	case EStdKeyRightShift:  KEY(8, 1);
	case EStdKeyLeftCtrl:    KEY(8, 2);
	case EStdKeyLeftFunc:    KEY(8, 3);
	}

	if (idx >= 0x800) {
		if (value)
			keyboardExtra |= (idx & 0xFF);
		else
			keyboardExtra &= ~(idx & 0xFF);
	} else if (idx >= 0) {
		if (value)
			keyboardColumns[idx >> 8] |= (idx & 0xFF);
		else
			keyboardColumns[idx >> 8] &= ~(idx & 0xFF);
	}
}

}
