#include "emu.h"
#include "wind.h"
#include "wind_hw.h"
#include <time.h>


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
//		printf("RegRead8 unknown:: pc=%08x lr=%08x reg=%03x\n", cpu.gprs[ARM_PC]-4, cpu.gprs[ARM_LR], reg);
		return 0xFF;
	}
}
uint32_t Emu::readReg32(uint32_t reg) {
	if (reg == LCDCTL) {
        printf("LCD control read pc=%08x lr=%08x !!!\n", cpu.gprs[ARM_PC], cpu.gprs[ARM_LR]);
		return lcdControl;
	} else if (reg == LCDST) {
        printf("LCD state read pc=%08x lr=%08x !!!\n", cpu.gprs[ARM_PC], cpu.gprs[ARM_LR]);
		return 0xFFFFFFFF;
	} else if (reg == PWRSR) {
//		printf("!!! PWRSR read pc=%08x lr=%08x !!!\n", cpu.gprs[ARM_PC], cpu.gprs[ARM_LR]);
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
//		printf("RegRead32 unknown:: pc=%08x lr=%08x reg=%03x\n", cpu.gprs[ARM_PC]-4, cpu.gprs[ARM_LR], reg);
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
//		printf("RegWrite8 unknown:: pc=%08x reg=%03x value=%02x\n", cpu.gprs[ARM_PC]-4, reg, value);
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
		cpu.halted = true;
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
//		printf("RegWrite32 unknown:: pc=%08x reg=%03x value=%08x\n", cpu.gprs[ARM_PC]-4, reg, value);
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

uint32_t Emu::readPhys8(uint32_t physAddress) {
    uint32_t result = 0xFF;
    uint8_t region = (physAddress >> 24) & 0xF1;
	if (region == 0)
		result = ROM[physAddress & 0xFFFFFF];
    else if (region == 0x20 && physAddress <= 0x20000FFF)
        result = etna.readReg8(physAddress & 0xFFF);
    else if (region == 0x80 && physAddress <= 0x80000FFF)
		result = readReg8(physAddress & 0xFFF);
    else if (region == 0xC0)
        result = MemoryBlockC0[physAddress & MemoryBlockMask];
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        result = MemoryBlockC1[physAddress & MemoryBlockMask];
#endif
    else if (region == 0xD0)
        result = MemoryBlockD0[physAddress & MemoryBlockMask];
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        result = MemoryBlockD1[physAddress & MemoryBlockMask];
#endif
//	else
//		printf("<%08x> unmapped read8 addr p:%08x\n", cpu.gprs[ARM_PC] - 4, physAddress);
	return result;
}
uint32_t Emu::readPhys16(uint32_t physAddress) {
	uint32_t result = 0xFFFFFFFF;
    uint8_t region = (physAddress >> 24) & 0xF1;
	if (region == 0)
		LOAD_16LE(result, physAddress & 0xFFFFFF, ROM);
    else if (region == 0xC0)
        LOAD_16LE(result, physAddress & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        LOAD_16LE(result, physAddress & MemoryBlockMask, MemoryBlockC1);
#endif
    else if (region == 0xD0)
        LOAD_16LE(result, physAddress & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        LOAD_16LE(result, physAddress & MemoryBlockMask, MemoryBlockD1);
#endif
//	else
//		printf("<%08x> unmapped read16 addr p:%08x\n", cpu.gprs[ARM_PC] - 4, physAddress);
	return result;
}
uint32_t Emu::readPhys32(uint32_t physAddress) {
	uint32_t result = 0xFFFFFFFF;
    uint8_t region = (physAddress >> 24) & 0xF1;
	if (region == 0)
		LOAD_32LE(result, physAddress & 0xFFFFFF, ROM);
    else if (region == 0x20 && physAddress <= 0x20000FFF)
        result = etna.readReg32(physAddress & 0xFFF);
    else if (region == 0x80 && physAddress <= 0x80000FFF)
		result = readReg32(physAddress & 0xFFF);
    else if (region == 0xC0)
        LOAD_32LE(result, physAddress & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        LOAD_32LE(result, physAddress & MemoryBlockMask, MemoryBlockC1);
#endif
    else if (region == 0xD0)
        LOAD_32LE(result, physAddress & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        LOAD_32LE(result, physAddress & MemoryBlockMask, MemoryBlockD1);
#endif
//	else
//		printf("<%08x> unmapped read32 addr p:%08x\n", cpu.gprs[ARM_PC] - 4, physAddress);
	return result;
}

void Emu::writePhys8(uint32_t physAddress, uint8_t value) {
    uint8_t region = (physAddress >> 24) & 0xF1;
    if (region == 0xC0)
        MemoryBlockC0[physAddress & MemoryBlockMask] = (uint8_t)value;
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        MemoryBlockC1[physAddress & MemoryBlockMask] = (uint8_t)value;
#endif
    else if (region == 0xD0)
        MemoryBlockD0[physAddress & MemoryBlockMask] = (uint8_t)value;
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        MemoryBlockD1[physAddress & MemoryBlockMask] = (uint8_t)value;
#endif
    else if (region == 0x20 && physAddress <= 0x20000FFF)
        etna.writeReg8(physAddress & 0xFFF, value);
    else if (region == 0x80 && physAddress <= 0x80000FFF)
        writeReg8(physAddress & 0xFFF, value);
//	else
//		printf("<%08x> unmapped write8 addr p:%08x :: %02x\n", cpu.gprs[ARM_PC] - 4, physAddress, value);
}
void Emu::writePhys16(uint32_t physAddress, uint16_t value) {
    uint8_t region = (physAddress >> 24) & 0xF1;
    if (region == 0xC0)
        STORE_16LE(value, physAddress & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        STORE_16LE(value, physAddress & MemoryBlockMask, MemoryBlockC1);
#endif
    else if (region == 0xD0)
        STORE_16LE(value, physAddress & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        STORE_16LE(value, physAddress & MemoryBlockMask, MemoryBlockD1);
#endif
//	else
//		printf("<%08x> unmapped write16 addr p:%08x :: %04x\n", cpu.gprs[ARM_PC] - 4, physAddress, value);
}
void Emu::writePhys32(uint32_t physAddress, uint32_t value) {
    uint8_t region = (physAddress >> 24) & 0xF1;
    if (region == 0xC0)
        STORE_32LE(value, physAddress & MemoryBlockMask, MemoryBlockC0);
#ifdef INCLUDE_BANK1
    else if (region == 0xC1)
        STORE_32LE(value, physAddress & MemoryBlockMask, MemoryBlockC1);
#endif
    else if (region == 0xD0)
        STORE_32LE(value, physAddress & MemoryBlockMask, MemoryBlockD0);
#ifdef INCLUDE_BANK1
    else if (region == 0xD1)
        STORE_32LE(value, physAddress & MemoryBlockMask, MemoryBlockD1);
#endif
    else if (region == 0x20 && physAddress <= 0x20000FFF)
        etna.writeReg32(physAddress & 0xFFF, value);
    else if (region == 0x80 && physAddress <= 0x80000FFF)
		writeReg32(physAddress & 0xFFF, value);
//	else
//		printf("<%08x> unmapped write32 addr p:%08x :: %08x\n", cpu.gprs[ARM_PC] - 4, physAddress, value);
}

uint32_t Emu::virtToPhys(uint32_t virtAddress) {
	if (!isMMU())
		return virtAddress;

	// find the TTB
	uint32_t ttbEntryAddr = translationTableBase & 0xFFFFC000;
	ttbEntryAddr |= ((virtAddress & 0xFFF00000) >> 18);
	uint32_t ttbEntry = readPhys32(ttbEntryAddr);

	if ((ttbEntry & 3) == 1) {
		// Page
		uint32_t pageTableAddr = ttbEntry & 0xFFFFFC00;
		pageTableAddr |= ((virtAddress & 0x000FF000) >> 10);
		uint32_t pageTableEntry = readPhys32(pageTableAddr);
		if ((pageTableEntry & 3) == 1) {
			// Large Page
			uint32_t lpBaseAddr = pageTableEntry & 0xFFFF0000;
			return lpBaseAddr | (virtAddress & 0x0000FFFF);
		} else if ((pageTableEntry & 3) == 2) {
			// Small Page
			uint32_t lpBaseAddr = pageTableEntry & 0xFFFFF000;
			return lpBaseAddr | (virtAddress & 0x00000FFF);
		} else {
			// Fault/Reserved
            // TODO: should raise Abort here?
			printf("!!! lv2 bad entry=%d vaddr=%08x !!!\n", pageTableEntry & 3, virtAddress);
			return 0xFFFFFFFF;
		}
	} else if ((ttbEntry & 3) == 2) {
		// Section
		uint32_t sectBaseAddr = ttbEntry & 0xFFF00000;
        return sectBaseAddr | (virtAddress & 0x000FFFFF);
    } else {
        // Fault/Reserved
        // TODO: should raise Abort here?
        printf("!!! lv1 bad entry=%d vaddr=%08x !!!\n", ttbEntry & 3, virtAddress);
        return 0xFFFFFFFF;
    }
}


void Emu::configure() {
    if (configured) return;
    configured = true;

    uart1.cpu = &cpu;
    uart2.cpu = &cpu;
    memset(&tc1, 0, sizeof(tc1));
    memset(&tc2, 0, sizeof(tc1));
    cpu.owner = this;

    nextTickAt = TICK_INTERVAL;
    rtc = getRTC();

    configureMemoryBindings();
    configureCpuHandlers();

    ARMReset(&cpu);
}

void Emu::configureMemoryBindings() {
    cpu.memory.load8 = [](struct ARMCore *cpu, uint32_t address, int *) {
        return ((Emu *)cpu->owner)->readVirt8(address);
    };
    cpu.memory.load16 = [](struct ARMCore *cpu, uint32_t address, int *) {
        return ((Emu *)cpu->owner)->readVirt16(address);
    };
    cpu.memory.load32 = [](struct ARMCore *cpu, uint32_t address, int *) {
        return ((Emu *)cpu->owner)->readVirt32(address);
    };
    cpu.memory.loadMultiple = [](struct ARMCore *cpu, uint32_t address, int mask, enum LSMDirection direction, int *cycleCounter) {
        uint32_t value;
        int i, offset = 4, popcount = 0;

        if (direction & LSM_D) {
            offset = -4;
            popcount = popcount32(mask);
            address -= (popcount << 2) - 4;
        }
        if (direction & LSM_B)
            address += offset;

        if (!mask) {
            value = cpu->memory.load32(cpu, address, cycleCounter);
            cpu->gprs[ARM_PC] = value;
            address += 64;
        }
        for (i = 0; i < 16; i++) {
            if (mask & (1 << i)) {
                value = cpu->memory.load32(cpu, address, cycleCounter);
                cpu->gprs[i] = value;
                address += 4;
            }
        }

        if (direction & LSM_B)
            address -= offset;
        if (direction & LSM_D)
            address -= (popcount << 2) + 4;

        return address;
    };

    cpu.memory.store8 = [](struct ARMCore *cpu, uint32_t address, int8_t value, int *) {
        ((Emu *)cpu->owner)->writeVirt8(address, value);
    };
    cpu.memory.store16 = [](struct ARMCore *cpu, uint32_t address, int16_t value, int *) {
        ((Emu *)cpu->owner)->writeVirt16(address, value);
    };
    cpu.memory.store32 = [](struct ARMCore *cpu, uint32_t address, int32_t value, int *) {
        ((Emu *)cpu->owner)->writeVirt32(address, value);
    };
    cpu.memory.storeMultiple = [](struct ARMCore *cpu, uint32_t address, int mask, enum LSMDirection direction, int *cycleCounter) {
        uint32_t value;
        int i, offset = 4, popcount = 0;

        if (direction & LSM_D) {
            offset = -4;
            popcount = popcount32(mask);
            address -= (popcount << 2) - 4;
        }
        if (direction & LSM_B)
            address += offset;

        if (!mask) {
            value = cpu->gprs[ARM_PC] + 4;
            cpu->memory.store32(cpu, address, value, cycleCounter);
            address += 64;
        }
        for (i = 0; i < 16; i++) {
            if (mask & (1 << i)) {
                value = cpu->gprs[i];
                if (i == ARM_PC) value += 4;
                cpu->memory.store32(cpu, address, value, cycleCounter);
                address += 4;
            }
        }

        if (direction & LSM_B)
            address -= offset;
        if (direction & LSM_D)
            address -= (popcount << 2) + 4;

        return address;
    };

    cpu.memory.activeSeqCycles32 = 0;
    cpu.memory.activeNonseqCycles32 = 0;
    cpu.memory.stall = [](struct ARMCore *cpu, int32_t wait) {
        return 0;
    };
}

void Emu::configureCpuHandlers() {
    cpu.irqh.reset = [](struct ARMCore *cpu) {
        printf("reset...\n");
    };
    cpu.irqh.processEvents = [](struct ARMCore *cpu) {
        // printf("processEvents...\n");
    };
    cpu.irqh.swi32 = [](struct ARMCore *cpu, int immediate) {
        ARMRaiseSWI(cpu);
    };
    cpu.irqh.hitIllegal = [](struct ARMCore *cpu, uint32_t opcode) {
        printf("hitIllegal... %08x\n", opcode);
    };
    cpu.irqh.bkpt32 = [](struct ARMCore *cpu, int immediate) {
        printf("bkpt32... %08x\n", immediate);
    };
    cpu.irqh.readCPSR = [](struct ARMCore *cpu) {
        // printf("readCPSR...\n");
        // printf("at %08x our priv became %s\n", cpu->gprs[ARM_PC]-4, privname(cpu));
    };
    cpu.irqh.hitStub = [](struct ARMCore *cpu, uint32_t opcode) {
        Emu *emu = (Emu *)cpu->owner;
        if ((opcode & 0x0F100F10) == 0x0E100F10) {
            // coprocessor read
            int cpReg = (opcode & 0x000F0000) >> 16;
            int armReg = (opcode & 0x0000F000) >> 12;
            if (cpReg == 0)
                cpu->gprs[armReg] = 0x41807100; //5mx device id
        } else if ((opcode & 0x0F100F10) == 0x0E000F10) {
            // coprocessor write
            int cpReg = (opcode & 0x000F0000) >> 16;
            int armReg = (opcode & 0x0000F000) >> 12;
            if (cpReg == 1) {
                emu->controlReg = cpu->gprs[armReg];
                printf("mmu is now %s\n", emu->isMMU() ? "on" : "off");
            } else if (cpReg == 2) {
                emu->translationTableBase = cpu->gprs[armReg];
            } else if (cpReg == 3) {
                emu->domainAccessControl = cpu->gprs[armReg];
            }
        } else {
            printf("hitStub... %08x\n", opcode);
        }
    };
}

void Emu::loadROM(const char *path) {
    FILE *f = fopen(path, "rb");
    fread(ROM, 1, sizeof(ROM), f);
    fclose(f);
}

void Emu::executeUntil(int64_t cycles) {
    if (!configured)
        configure();

    while (!asleep && cpu.cycles < cycles) {
        if (cpu.cycles >= nextTickAt) {
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
        if (tc1.tick(cpu.cycles))
            pendingInterrupts |= (1<<TC1OI);
        if (tc2.tick(cpu.cycles))
            pendingInterrupts |= (1<<TC2OI);

        if ((pendingInterrupts & interruptMask & FIQ_INTERRUPTS) != 0)
            ARMRaiseFIQ(&cpu);
        if ((pendingInterrupts & interruptMask & IRQ_INTERRUPTS) != 0)
            ARMRaiseIRQ(&cpu);

//        if (cpu.cycles >= 30000000) {
//            static bool lcdtest = false;
//            if (!lcdtest) {
//                printf("lcdtest\n");
//                pendingInterrupts |= (1<<LCDINT);
//                lcdtest = true;
//            }
//        }

        // what's running?
        if (cpu.halted) {
            // keep the clock moving
            cpu.cycles++;
        } else {
            uint32_t pc = cpu.gprs[ARM_PC] - 4;
            uint32_t phys_pc = virtToPhys(pc);
            debugPC(phys_pc);
            ARMRun(&cpu);

            uint32_t new_pc = cpu.gprs[ARM_PC] - 4;
            if (_breakpoints.find(new_pc) != _breakpoints.end())
                return;
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
    printf("R00:%08x R01:%08x R02:%08x R03:%08x\n", cpu.gprs[0], cpu.gprs[1], cpu.gprs[2], cpu.gprs[3]);
    printf("R04:%08x R05:%08x R06:%08x R07:%08x\n", cpu.gprs[4], cpu.gprs[5], cpu.gprs[6], cpu.gprs[7]);
    printf("R08:%08x R09:%08x R10:%08x R11:%08x\n", cpu.gprs[8], cpu.gprs[9], cpu.gprs[10], cpu.gprs[11]);
    printf("R12:%08x R13:%08x R14:%08x R15:%08x\n", cpu.gprs[12], cpu.gprs[13], cpu.gprs[14], cpu.gprs[15]);
    printf("cpsr=%08x spsr=%08x\n", cpu.cpsr.packed, cpu.spsr.packed);
}

const char *Emu::identifyObjectCon(uint32_t ptr) {
    if (ptr == readVirt32(0x80000980)) return "process";
    if (ptr == readVirt32(0x80000984)) return "thread";
    if (ptr == readVirt32(0x80000988)) return "chunk";
//	if (ptr == readVirt32(0x8000098C)) return "semaphore";
//	if (ptr == readVirt32(0x80000990)) return "mutex";
    if (ptr == readVirt32(0x80000994)) return "logicaldevice";
    if (ptr == readVirt32(0x80000998)) return "physicaldevice";
    if (ptr == readVirt32(0x8000099C)) return "channel";
    if (ptr == readVirt32(0x800009A0)) return "server";
//	if (ptr == readVirt32(0x800009A4)) return "unk9A4"; // name always null
    if (ptr == readVirt32(0x800009AC)) return "library";
//	if (ptr == readVirt32(0x800009B0)) return "unk9B0"; // name always null
//	if (ptr == readVirt32(0x800009B4)) return "unk9B4"; // name always null
    return NULL;
}

void Emu::fetchStr(uint32_t str, char *buf) {
    if (str == 0) {
        strcpy(buf, "<NULL>");
        return;
    }
    int size = readVirt32(str);
    for (int i = 0; i < size; i++) {
        buf[i] = readVirt8(str + 4 + i);
    }
    buf[size] = 0;
}

void Emu::fetchName(uint32_t obj, char *buf) {
    fetchStr(readVirt32(obj + 0x10), buf);
}

void Emu::fetchProcessFilename(uint32_t obj, char *buf) {
    fetchStr(readVirt32(obj + 0x3C), buf);
}

void Emu::debugPC(uint32_t pc) {
    char objName[1000];
    if (pc == 0x2CBC4) {
        // CObjectCon::AddL()
        uint32_t container = cpu.gprs[0];
        uint32_t obj = cpu.gprs[1];
        const char *wut = identifyObjectCon(container);
        if (wut) {
            fetchName(obj, objName);
            printf("OBJS: added %s at %08x <%s>", wut, obj, objName);
            if (strcmp(wut, "process") == 0) {
                fetchProcessFilename(obj, objName);
                printf(" <%s>", objName);
            }
            printf("\n");
        }
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
