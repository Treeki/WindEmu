#include "clps7111.h"
#include "clps7111_defs.h"
#include "hardware.h"
#include <time.h>
#include "common.h"


CLPS7111::CLPS7111() : ARM710(false), pcCardController(this) {
}


uint32_t CLPS7111::getRTC() {
	return time(nullptr) - 946684800;
}


uint32_t CLPS7111::readReg8(uint32_t reg) {
	if (reg == PADR) {
		return readKeyboard();
	} else if (reg == PBDR) {
		return (portValues >> 16) & 0xFF;
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
uint32_t CLPS7111::readReg32(uint32_t reg) {
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

void CLPS7111::writeReg8(uint32_t reg, uint8_t value) {
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
void CLPS7111::writeReg32(uint32_t reg, uint32_t value) {
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

bool CLPS7111::isPhysAddressValid(uint32_t physAddress) const {
	uint8_t region = (physAddress >> 24) & 0xF1;
	switch (region) {
	case 0: return true;
	case 0x80: return (physAddress <= 0x80000FFF);
	case 0xC0: return true;
	default: return false;
	}
}


MaybeU32 CLPS7111::readPhysical(uint32_t physAddr, ValueSize valueSize) {
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

bool CLPS7111::writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) {
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



void CLPS7111::configure() {
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

void CLPS7111::loadROM(const char *path) {
	FILE *f = fopen(path, "rb");
	fread(ROM, 1, sizeof(ROM), f);
	fclose(f);
}

void CLPS7111::executeUntil(int64_t cycles) {
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

void CLPS7111::dumpRAM(const char *path) {
	FILE *f = fopen(path, "wb");
	fwrite(MemoryBlockC0, 1, sizeof(MemoryBlockC0), f);
	fclose(f);
}



void CLPS7111::printRegs() {
	printf("R00:%08x R01:%08x R02:%08x R03:%08x\n", getGPR(0), getGPR(1), getGPR(2), getGPR(3));
	printf("R04:%08x R05:%08x R06:%08x R07:%08x\n", getGPR(4), getGPR(5), getGPR(6), getGPR(7));
	printf("R08:%08x R09:%08x R10:%08x R11:%08x\n", getGPR(8), getGPR(9), getGPR(10), getGPR(11));
	printf("R12:%08x R13:%08x R14:%08x R15:%08x\n", getGPR(12), getGPR(13), getGPR(14), getGPR(15));
//    printf("cpsr=%08x spsr=%08x\n", cpu.cpsr.packed, cpu.spsr.packed);
}

const char *CLPS7111::identifyObjectCon(uint32_t ptr) {
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

void CLPS7111::fetchStr(uint32_t str, char *buf) {
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

void CLPS7111::fetchName(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x10, V32).value(), buf);
}

void CLPS7111::fetchProcessFilename(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x3C, V32).value(), buf);
}

void CLPS7111::debugPC(uint32_t pc) {
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
//	if (pc == 0x3B250) {
//		log("DBG 5003B250: pc=%08x lr=%08x sp=%08x", getRealPC(), getGPR(14), getGPR(13));
//	}
}


const uint8_t *CLPS7111::getLCDBuffer() const {
	if ((lcdAddress >> 24) == 0xC0)
		return &MemoryBlockC0[lcdAddress & MemoryBlockMask];
	else
		return nullptr;
}


uint8_t CLPS7111::readKeyboard() {
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



void CLPS7111::diffPorts(uint32_t oldval, uint32_t newval) {
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
	if (changes & 0x1000000) log("PRT A0: %d", newval&0x1000000);
	if (changes & 0x2000000) log("PRT A1: %d", newval&0x2000000);
	if (changes & 0x4000000) log("PRT A2: %d", newval&0x4000000);
	if (changes & 0x8000000) log("PRT A3: %d", newval&0x8000000);
	if (changes & 0x10000000) log("PRT A4: %d", newval&0x10000000);
	if (changes & 0x20000000) log("PRT A5: %d", newval&0x20000000);
	if (changes & 0x40000000) log("PRT A6: %d", newval&0x40000000);
	if (changes & 0x80000000) log("PRT A7: %d", newval&0x80000000);
}
