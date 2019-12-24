#include "arm710.h"
#include "common.h"

// this will need changing if this code ever compiles on big-endian procs
inline uint32_t read32LE(uint8_t *p) {
	return *((uint32_t *)p);
}
inline void write32LE(uint8_t *p, uint32_t v) {
	*((uint32_t *)p) = v;
}


void ARM710::switchBank(BankIndex newBank) {
	if (newBank != bank) {
		// R13 and R14 need saving/loading for all banks
		allModesBankedRegisters[bank][0] = GPRs[13];
		allModesBankedRegisters[bank][1] = GPRs[14];
		GPRs[13] = allModesBankedRegisters[newBank][0];
		GPRs[14] = allModesBankedRegisters[newBank][1];

		// R8 to R12 are only banked in FIQ mode
		auto oldBankR8to12 = (bank == FiqBank) ? 1 : 0;
		auto newBankR8to12 = (newBank == FiqBank) ? 1 : 0;
		if (oldBankR8to12 != newBankR8to12) {
			// swap these sets around
			for (int i = 0; i < 5; i++)
				fiqBankedRegisters[oldBankR8to12][i] = GPRs[8 + i];
			for (int i = 0; i < 5; i++)
				GPRs[8 + i] = fiqBankedRegisters[newBankR8to12][i];
		}

		bank = newBank;
	}
}


void ARM710::switchMode(Mode newMode) {
	auto oldMode = currentMode();
	if (newMode != oldMode) {
//		log("Switching mode! %x", newMode);
		switchBank(modeToBank[newMode & 0xF]);

		CPSR &= ~CPSR_ModeMask;
		CPSR |= newMode;
	}
}

void ARM710::raiseException(Mode mode, uint32_t savedPC, uint32_t newPC) {
	auto bankIndex = modeToBank[mode & 0xF];
//	log("Raising exception mode %x, saving PC %08x, CPSR %08x", mode, savedPC, CPSR);
	SPSRs[bankIndex] = CPSR;

	switchMode(mode);

	prefetchCount = 0;
	GPRs[14] = savedPC;
	GPRs[15] = newPC;
}

void ARM710::requestFIQ() {
	raiseException(FIQ32, getRealPC() + 4, 0x1C);
	CPSR |= CPSR_FIQDisable;
	CPSR |= CPSR_IRQDisable;
}

void ARM710::requestIRQ() {
//	log("Requesting IRQ: Last exec = %08x, setting LR = %08x", lastPcExecuted(), getRealPC() + 4);
	raiseException(IRQ32, getRealPC() + 4, 0x18);
	CPSR |= CPSR_IRQDisable;
}

void ARM710::reset() {
#ifdef ARM710T_CACHE
	clearCache();
#endif
	raiseException(Supervisor32, 0, 0);
}



uint32_t ARM710::tick() {
	// pop an instruction off the end of the pipeline
	bool haveInsn = false;
	uint32_t insn;
	MMUFault insnFault;
	if (prefetchCount == 2) {
		haveInsn = true;
		insn = prefetch[1];
		insnFault = prefetchFaults[1];
	}

	// move the instruction we fetched last tick once along
	if (prefetchCount >= 1) {
		prefetch[1] = prefetch[0];
		prefetchFaults[1] = prefetchFaults[0];
	}

	// fetch a new instruction
	auto newInsn = readVirtual(GPRs[15], V32);
	GPRs[15] += 4;
	prefetch[0] = newInsn.first.value_or(0);
	prefetchFaults[0] = newInsn.second;
	if (prefetchCount < 2)
		prefetchCount++;

	// now deal with the one we popped
	uint32_t clocks = 1;
	if (haveInsn) {
		pcHistory[pcHistoryIndex] = {GPRs[15] - 0xC, insn};
		pcHistoryIndex = (pcHistoryIndex + 1) % PcHistoryCount;
		if (insnFault != NoFault) {
			// Raise a prefetch error
			// These do not set FSR or FAR
			log("prefetch error! %08x", insnFault >> MMUFaultAddressShift);
			logPcHistory();
			raiseException(Abort32, GPRs[15] - 8, 0xC);
		} else {
			clocks += executeInstruction(insn);
		}
	}

	if (faultTriggeredThisCycle) {
		// data abort time!
		faultTriggeredThisCycle = false;
		raiseException(Abort32, GPRs[15] - 4, 0x10);
	}

	return clocks;
}


static inline uint32_t extract(uint32_t value, uint32_t hiBit, uint32_t loBit) {
	return (value >> loBit) & ((1 << (hiBit - loBit + 1)) - 1);
//	return (value >> (32 - offset - length)) & ((1 << length) - 1);
}
static inline bool extract1(uint32_t value, uint32_t bit) {
	return (value >> bit) & 1;
}


uint32_t ARM710::executeInstruction(uint32_t i) {
	uint32_t cycles = 1;
//	log("executing insn %08x @ %08x", i, GPRs[15] - 0xC);

	// a big old dispatch thing here
	// but first, conditions!
	if (!checkCondition(extract(i, 31, 28)))
		return cycles;

	if ((i & 0x0F000000) == 0x0F000000)
		raiseException(Supervisor32, GPRs[15] - 8, 0x08);
	else if ((i & 0x0F000F10) == 0x0E000F10)
		cycles += execCP15RegisterTransfer(extract(i,23,21), extract1(i,20), extract(i,19,16), extract(i,15,12), extract(i,7,5), extract(i,3,0));
	else if ((i & 0x0E000000) == 0x0A000000)
		cycles += execBranch(extract1(i,24), extract(i,23,0));
	else if ((i & 0x0E000000) == 0x08000000)
		cycles += execBlockDataTransfer(extract(i,24,20), extract(i,19,16), extract(i,15,0));
	else if ((i & 0x0C000000) == 0x04000000)
		cycles += execSingleDataTransfer(extract(i,25,20), extract(i,19,16), extract(i,15,12), extract(i,11,0));
	else if ((i & 0x0FB00FF0) == 0x01000090)
		cycles += execSingleDataSwap(extract1(i,22), extract(i,19,16), extract(i,15,12), extract(i,3,0));
	else if ((i & 0x0F8000F0) == 0x00000090)
		cycles += execMultiply(extract(i,21,20), extract(i,19,16), extract(i,15,12), extract(i,11,8), extract(i,3,0));
	else if ((i & 0x0C000000) == 0x00000000)
		cycles += execDataProcessing(extract1(i,25), extract(i,24,21), extract1(i,20), extract(i,19,16), extract(i,15,12), extract(i,11,0));
	else
		raiseException(Undefined32, GPRs[15] - 8, 0x04);

	return cycles;
}

uint32_t ARM710::execDataProcessing(bool I, uint32_t Opcode, bool S, uint32_t Rn, uint32_t Rd, uint32_t Operand2)
{
	uint32_t cycles = 0; // TODO increment me semi-accurately
	bool shifterCarryOutput;

	// compute our Op1 (may be unnecessary but that's ok)
	uint32_t op1 = GPRs[Rn];

	// compute our Op2
	uint32_t op2;
	if (!I) {
		// REGISTER
		uint32_t Rm = extract(Operand2, 3, 0);
		op2 = GPRs[Rm];

		uint8_t shiftBy;

		// this is the real painful one, honestly
		if (extract(Operand2, 4, 4)) {
			// Shift by Register
			uint32_t Rs = extract(Operand2, 11, 8);
			shiftBy = GPRs[Rs] & 0xFF;
		} else {
			// Shift by Immediate
			shiftBy = extract(Operand2, 11, 7);

			if (Rn == 15) // if PC is fetched...
				op1 -= 4; // compensate for prefetching
			if (Rm == 15)
				op2 -= 4;
		}

		if (extract(Operand2, 4, 4) && (shiftBy == 0)) {
			// register shift by 0 never does anything
			shifterCarryOutput = flagC();
		} else {
			switch (extract(Operand2, 6, 5)) {
			case 0: // Logical Left (LSL)
				if (shiftBy == 0) {
					shifterCarryOutput = flagC();
					// no change to op2!
				} else if (shiftBy <= 31) {
					shifterCarryOutput = extract1(op2, 31 - shiftBy);
					op2 <<= shiftBy;
				} else if (shiftBy == 32) {
					shifterCarryOutput = extract1(op2, 0);
					op2 = 0;
				} else /*if (shiftBy >= 33)*/ {
					shifterCarryOutput = false;
					op2 = 0;
				}
				break;
			case 1: // Logical Right (LSR)
				if (shiftBy == 0 || shiftBy == 32) {
					shifterCarryOutput = extract1(op2, 31);
					op2 = 0;
				} else if (shiftBy <= 31) {
					shifterCarryOutput = extract1(op2, shiftBy - 1);
					op2 >>= shiftBy;
				} else /*if (shiftBy >= 33)*/ {
					shifterCarryOutput = false;
					op2 = 0;
				}
				break;
			case 2: // Arithmetic Right (ASR)
				if (shiftBy == 0 || shiftBy >= 32) {
					shifterCarryOutput = extract1(op2, 31);
					op2 = (int32_t)op2 >> 31;
				} else /*if (shiftBy <= 31)*/ {
					shifterCarryOutput = extract1(op2, shiftBy - 1);
					op2 = (int32_t)op2 >> shiftBy;
				}
				break;
			case 3: // Rotate Right (ROR)
				if (shiftBy == 0) { // treated as RRX
					shifterCarryOutput = op2 & 1;
					op2 >>= 1;
					op2 |= flagC() ? 0x80000000 : 0;
				} else {
					shiftBy %= 32;
					if (shiftBy == 0) { // like 32
						shifterCarryOutput = extract1(op2, 31);
						// no change to op2
					} else {
						shifterCarryOutput = extract1(op2, shiftBy - 1);
						op2 = ROR(op2, shiftBy);
					}
				}
				break;
			}
		}
	} else {
		// IMMEDIATE
		if (Rn == 15) // if PC is fetched...
			op1 -= 4; // compensate for prefetching

		uint32_t Rotate = extract(Operand2, 11, 8);
		uint32_t Imm = extract(Operand2, 7, 0);
		op2 = ROR(Imm, Rotate * 2);
		shifterCarryOutput = flagC(); // correct? unsure...
	}

	// we have our operands, what next
	uint64_t result = 0;
	uint32_t flags = 0;

#define LOGICAL_OP(v) \
	result = v; \
	flags |= (result & 0xFFFFFFFF) ? 0 : CPSR_Z; \
	flags |= (result & 0x80000000) ? CPSR_N : 0; \
	flags |= shifterCarryOutput ? CPSR_C : 0; \
	flags |= (CPSR & CPSR_V);

#define ADD_OP(a, b, c) \
	result = (uint64_t)(a) + (uint64_t)(b) + (uint64_t)(c); \
	flags |= (result & 0xFFFFFFFF) ? 0 : CPSR_Z; \
	flags |= (result & 0x80000000) ? CPSR_N : 0; \
	flags |= (result & 0x100000000) ? CPSR_C : 0; \
	flags |= ((((a) & 0x80000000) == ((b) & 0x80000000)) && (((a) & 0x80000000) != (result & 0x80000000))) ? CPSR_V : 0;

#define SUB_OP(a, b, c) ADD_OP(a, ~b, c)


	switch (Opcode) {
	case 0:   LOGICAL_OP(op1 & op2);     break; // AND
	case 1:   LOGICAL_OP(op1 ^ op2);     break; // EOR
	case 2:   SUB_OP(op1, op2, 1);       break; // SUB
	case 3:   SUB_OP(op2, op1, 1);       break; // RSB
	case 4:   ADD_OP(op1, op2, 0);       break; // ADD
	case 5:   ADD_OP(op1, op2, flagC()); break; // ADC
	case 6:   SUB_OP(op1, op2, flagC()); break; // SBC
	case 7:   SUB_OP(op2, op1, flagC()); break; // RSC
	case 8:   LOGICAL_OP(op1 & op2);     break; // TST
	case 9:   LOGICAL_OP(op1 ^ op2);     break; // TEQ
	case 0xA: SUB_OP(op1, op2, 1);       break; // CMP
	case 0xB: ADD_OP(op1, op2, 0);       break; // CMN
	case 0xC: LOGICAL_OP(op1 | op2);     break; // ORR
	case 0xD: LOGICAL_OP(op2);           break; // MOV
	case 0xE: LOGICAL_OP(op1 & ~op2);    break; // BIC
	case 0xF: LOGICAL_OP(~op2);          break; // MVN
	}

	if (Opcode >= 8 && Opcode <= 0xB) {
		// Output-less opcodes: special behaviour
		if (S) {
			CPSR = (CPSR & ~CPSR_FlagMask) | flags;
//			log("CPSR setflags=%08x results in CPSR=%08x", flags, CPSR);
		} else if (Opcode == 8) {
			// MRS, CPSR -> Reg
			GPRs[Rd] = CPSR;
//			log("r%d <- CPSR(%08x)", Rd, GPRs[Rd]);
		} else if (Opcode == 9) {
			// MSR, Reg -> CPSR
			bool canChangeMode = extract1(Rn, 0);
			if (canChangeMode && isPrivileged()) {
				auto newCPSR = GPRs[extract(Operand2, 3, 0)];
				switchMode(modeFromCPSR(newCPSR));
				CPSR = newCPSR;
//				log("CPSR change privileged: %08x", CPSR);
			} else {
				// for the flag-only version, immediates are allowed
				// so we just re-use what was calculated earlier...
				auto newFlag = I ? op2 : GPRs[extract(Operand2, 3, 0)];
				CPSR &= ~CPSR_FlagMask;
				CPSR |= (newFlag & CPSR_FlagMask);
//				log("CPSR change unprivileged: new=%08x result=%08x", newFlag, CPSR);
			}
		} else if (Opcode == 0xA) {
			// MRS, SPSR -> Reg
			if (isPrivileged()) {
				GPRs[Rd] = SPSRs[currentBank()];
//				log("r%d <- SPSR(%08x)", Rd, GPRs[Rd]);
			}
		} else /*if (Opcode == 0xB)*/ {
			bool canChangeMode = extract1(Rn, 0);
			if (isPrivileged()) {
				if (canChangeMode) {
					SPSRs[currentBank()] = GPRs[extract(Operand2, 3, 0)];
//					log("SPSR change privileged: %08x", SPSRs[currentBank()]);
				} else {
					// same hat
					auto newFlag = I ? op2 : GPRs[extract(Operand2, 3, 0)];
					SPSRs[currentBank()] &= ~CPSR_FlagMask;
					SPSRs[currentBank()] |= (newFlag & CPSR_FlagMask);
//					log("SPSR change unprivileged: new=%08x result=%08x", newFlag, SPSRs[currentBank()]);
				}
			}
		}
	} else {
		GPRs[Rd] = result & 0xFFFFFFFF;

		if (Rd == 15) {
			// Writing to PC
			// Special things occur here!
			prefetchCount = 0;
			if (S && isPrivileged()) {
				// We SHOULD be privileged
				// (Raise an error otherwise...?)
				auto saved = SPSRs[currentBank()];
				switchMode(modeFromCPSR(saved));
				CPSR = saved;
//				log("dataproc restore CPSR: %08x", saved);
			}
		} else if (S) {
			CPSR = (CPSR & ~CPSR_FlagMask) | flags;
//			log("dataproc flag change: flags=%08x CPSR=%08x", flags, CPSR);
		}
	}

	return cycles;
}

uint32_t ARM710::execMultiply(uint32_t AS, uint32_t Rd, uint32_t Rn, uint32_t Rs, uint32_t Rm)
{
	// no need for R15 fuckery
	// datasheet says it's not allowed here
	if (AS & 2)
		GPRs[Rd] = GPRs[Rm] * GPRs[Rs] + GPRs[Rn];
	else
		GPRs[Rd] = GPRs[Rm] * GPRs[Rs];

	if (AS & 1) {
		CPSR &= ~(CPSR_N | CPSR_Z);
		CPSR |= GPRs[Rd] ? 0 : CPSR_Z;
		CPSR |= (GPRs[Rd] & 0x80000000) ? CPSR_N : 0;
	}

	return 0;
}

uint32_t ARM710::execSingleDataSwap(bool B, uint32_t Rn, uint32_t Rd, uint32_t Rm)
{
	auto valueSize = B ? V8 : V32;
	auto readResult = readVirtual(GPRs[Rn], valueSize);
	auto fault = readResult.second;

	if (fault == NoFault) {
		fault = writeVirtual(GPRs[Rm], GPRs[Rn], valueSize);
		if (fault == NoFault)
			GPRs[Rd] = readResult.first.value();
	}

	if (fault != NoFault)
		reportFault(fault);

	return 1;
}

uint32_t ARM710::execSingleDataTransfer(uint32_t IPUBWL, uint32_t Rn, uint32_t Rd, uint32_t offset)
{
	bool load = extract1(IPUBWL, 0);
	bool writeback = extract1(IPUBWL, 1);
	auto valueSize = extract1(IPUBWL, 2) ? V8 : V32;
	bool up = extract1(IPUBWL, 3);
	bool preIndex = extract1(IPUBWL, 4);
	bool immediate = !extract1(IPUBWL, 5);

	// calculate the offset
	uint32_t calcOffset;
	if (!immediate) {
		// REGISTER
		uint32_t Rm = extract(offset, 3, 0);
		calcOffset = GPRs[Rm];

		uint8_t shiftBy = extract(offset, 11, 7);

		switch (extract(offset, 6, 5)) {
		case 0: // Logical Left (LSL)
			if (shiftBy > 0)
				calcOffset <<= shiftBy;
			break;
		case 1: // Logical Right (LSR)
			if (shiftBy == 0)
				calcOffset = 0;
			else
				calcOffset >>= shiftBy;
			break;
		case 2: // Arithmetic Right (ASR)
			if (shiftBy == 0)
				calcOffset = (int32_t)calcOffset >> 31;
			else
				calcOffset = (int32_t)calcOffset >> shiftBy;
			break;
		case 3: // Rotate Right (ROR)
			if (shiftBy == 0) { // treated as RRX
				calcOffset >>= 1;
				calcOffset |= flagC() ? 0x80000000 : 0;
			} else
				calcOffset = ROR(calcOffset, shiftBy);
			break;
		}
	} else {
		// IMMEDIATE
		// No rotation or anything here
		calcOffset = offset;
	}

	uint32_t base = GPRs[Rn];
	if (Rn == 15) base -= 4; // prefetch adjustment
	uint32_t modifiedBase = up ? (base + calcOffset) : (base - calcOffset);
	uint32_t transferAddr = preIndex ? modifiedBase : base;

	bool changeModes = !preIndex && writeback && isPrivileged();
	auto saveMode = currentMode();

	MMUFault fault;

	if (load) {
		if (changeModes) switchMode(User32);
		auto readResult = readVirtual(transferAddr, valueSize);
		if (changeModes) switchMode(saveMode);
		if (readResult.first.has_value()) {
			GPRs[Rd] = readResult.first.value();
			if (Rd == 15) prefetchCount = 0;
		}
		fault = readResult.second;
	} else {
		uint32_t value = GPRs[Rd];
		if (changeModes) switchMode(User32);
		fault = writeVirtual(value, transferAddr, valueSize);
		if (changeModes) switchMode(saveMode);
	}

	if ((preIndex && writeback) || !preIndex)
		GPRs[Rn] = modifiedBase;

	if (fault != NoFault)
		reportFault(fault);

	return 2;
}

uint32_t ARM710::execBlockDataTransfer(uint32_t PUSWL, uint32_t Rn, uint32_t registerList)
{
	bool load = extract1(PUSWL, 0);
	bool store = !load;
	bool writeback = extract1(PUSWL, 1);
	bool psrForceUser = extract1(PUSWL, 2);
	bool up = extract1(PUSWL, 3);
	bool preIndex = extract1(PUSWL, 4);

	MMUFault fault = NoFault;
	uint32_t base = GPRs[Rn] & ~3;
	uint32_t blockSize = popcount32(registerList) * 4;

	uint32_t lowAddr, updatedBase;
	if (up) {
		updatedBase = base + blockSize;
		lowAddr = base + (preIndex ? 4 : 0);
	} else {
		updatedBase = base - blockSize;
		lowAddr = updatedBase + (preIndex ? 0 : 4);
	}

	auto saveBank = bank;
	if (psrForceUser && (store || !(registerList & 0x8000)))
		switchBank(MainBank);

	bool doneWriteback = false;
	if (load && writeback) {
		doneWriteback = true;
		GPRs[Rn] = updatedBase;
	}

	uint32_t addr = lowAddr;
	for (int i = 0; i < 16; i++) {
		if (registerList & (1 << i)) {
			// work on this one
			if (load) {
				// handling for LDM faults may be kinda iffy...
				// wording on datasheet is a bit unclear
				auto readResult = readVirtual(addr, V32);
				if (readResult.first.has_value())
					GPRs[i] = readResult.first.value();
				if (readResult.second != NoFault) {
					fault = readResult.second;
					break;
				}
			} else {
				auto newFault = writeVirtual(GPRs[i], addr, V32);
				if (newFault != NoFault)
					fault = newFault;
			}

			addr += 4;

			if (writeback && !doneWriteback) {
				doneWriteback = true;
				GPRs[Rn] = updatedBase;
			}
		}
	}

	// datasheet specifies that base register must be
	// restored if an error occurs during LDM
	if (load && fault != NoFault)
		GPRs[Rn] = writeback ? updatedBase : base;

	if (psrForceUser && (store || !(registerList & 0x8000)))
		switchBank(saveBank);

	if (load && (registerList & 0x8000)) {
		prefetchCount = 0;
		if (psrForceUser && isPrivileged() && fault == NoFault) {
			auto saved = SPSRs[currentBank()];
			switchMode(modeFromCPSR(saved));
			CPSR = saved;
//			log("reloading saved SPSR: %08x", saved);
		}
	}

	if (fault != NoFault)
		reportFault(fault);

	return 0; // fixme
}

uint32_t ARM710::execBranch(bool L, uint32_t offset)
{
	if (L)
		GPRs[14] = GPRs[15] - 8;

	// start with 24 bits, shift left 2, sign extend to 32
	int32_t sextOffset = (int32_t)(offset << 8) >> 6;

	prefetchCount = 0;
	GPRs[15] -= 4; // account for our prefetch being +4 too much
	GPRs[15] += sextOffset;
	return 0;
}

uint32_t ARM710::execCP15RegisterTransfer(uint32_t CPOpc, bool L, uint32_t CRn, uint32_t Rd, uint32_t CP, uint32_t CRm)
{
	(void)CP;
	(void)CRm;

	if (!isPrivileged())
		return 0;

	if (L) {
		// read a value
		uint32_t what = 0;

		switch (CRn) {
		case 0: what = cp15_id; break;
		case 5: what = cp15_faultStatus; break;
		case 6: what = cp15_faultAddress; break;
		}

		if (Rd == 15)
			CPSR = (CPSR & ~CPSR_FlagMask) | (what & CPSR_FlagMask);
		else
			GPRs[Rd] = what;
	} else {
		// store a value
		uint32_t what = GPRs[Rd];

		switch (CRn) {
		case 1: cp15_control = what; log("setting cp15_control to %08x", what); break;
		case 2: cp15_translationTableBase = what; break;
		case 3: cp15_domainAccessControl = what; break;
		case 5:
			if (isTVersion)
				cp15_faultStatus = what;
			else
				flushTlb();
			break;
		case 6:
			if (isTVersion)
				cp15_faultAddress = what;
			else
				flushTlb(what);
			break;
		case 7:
#ifdef ARM710T_CACHE
			clearCache();
			log("cache cleared");
#endif
			break;
		case 8: {
			if (isTVersion) {
				if (CPOpc == 1)
					flushTlb(what);
				else
					flushTlb();
			}
			break;
		}
		}
	}

	return 0;
}



#ifdef ARM710T_CACHE
void ARM710T::clearCache() {
	for (uint32_t i = 0; i < CacheSets; i++) {
		for (uint32_t j = 0; j < CacheBlocksPerSet; j++) {
			cacheBlockTags[i][j] = 0;
		}
	}
}

uint8_t *ARM710T::findCacheLine(uint32_t virtAddr) {
	uint32_t set = virtAddr & CacheAddressSetMask;
	uint32_t tag = virtAddr & CacheAddressTagMask;
	set >>= CacheAddressSetShift;

	for (uint32_t i = 0; i < CacheBlocksPerSet; i++) {
		if (cacheBlockTags[set][i] & CacheBlockEnabled) {
			if ((cacheBlockTags[set][i] & ~CacheBlockEnabled) == tag)
				return &cacheBlocks[set][i][0];
		}
	}

	return nullptr;
}

pair<MaybeU32, ARM710T::MMUFault> ARM710T::addCacheLineAndRead(uint32_t physAddr, uint32_t virtAddr, ValueSize valueSize, int domain, bool isPage) {
	uint32_t set = virtAddr & CacheAddressSetMask;
	uint32_t tag = virtAddr & CacheAddressTagMask;
	set >>= CacheAddressSetShift;

	// "it will be randomly placed in a cache bank"
	//    - the ARM710a data sheet, 6-2 (p90)
	uint32_t i = rand() % CacheBlocksPerSet;
	uint8_t *block = &cacheBlocks[set][i][0];
	MaybeU32 result;
	MMUFault fault = NoFault;

	for (uint32_t j = 0; j < CacheBlockSize; j += 4) {
		auto word = readPhysical((physAddr & ~CacheAddressLineMask) + j, V32);
		if (word.has_value()) {
			write32LE(&block[j], word.value());
			if (valueSize == V8 && j == (virtAddr & CacheAddressLineMask & ~3))
				result = (word.value() >> ((virtAddr & 3) * 8)) & 0xFF;
			else if (valueSize == V32 && j == (virtAddr & CacheAddressLineMask))
				result = word.value();
		} else {
			// read error, great
			// TODO: should probably prioritise specific kinds of faults over others
			fault = encodeFaultSorP(SorPLinefetchError, isPage, domain, virtAddr & ~CacheAddressLineMask);
			break;
		}
	}

	// the cache block is only stored if it's complete
	if (fault == NoFault)
		cacheBlockTags[set][i] = tag | CacheBlockEnabled;

	return make_pair(result, fault);
}

MaybeU32 ARM710T::readCached(uint32_t virtAddr, ValueSize valueSize) {
	uint8_t *line = findCacheLine(virtAddr);
	if (line) {
		if (valueSize == V8)
			return line[virtAddr & CacheAddressLineMask];
		else /*if (valueSize == V32)*/
			return read32LE(&line[virtAddr & CacheAddressLineMask]);
	}
	return {};
}


bool ARM710T::writeCached(uint32_t value, uint32_t virtAddr, ValueSize valueSize) {
	uint8_t *line = findCacheLine(virtAddr);
	if (line) {
		if (valueSize == V8)
			line[virtAddr & CacheAddressLineMask] = value & 0xFF;
		else /*if (valueSize == V32)*/
			write32LE(&line[virtAddr & CacheAddressLineMask], value);
		return true;
	}
	return false;
}
#endif


uint32_t ARM710::physAddrFromTlbEntry(TlbEntry *tlbEntry, uint32_t virtAddr) {
	if ((tlbEntry->lv2Entry & 3) == 2) {
		// Smøl page
		return (tlbEntry->lv2Entry & 0xFFFFF000) | (virtAddr & 0xFFF);
	} else if ((tlbEntry->lv2Entry & 3) == 1) {
		// Lørge page
		return (tlbEntry->lv2Entry & 0xFFFF0000) | (virtAddr & 0xFFFF);
	} else {
		// Section
		return (tlbEntry->lv1Entry & 0xFFF00000) | (virtAddr & 0xFFFFF);
	}
}


MaybeU32 ARM710::virtToPhys(uint32_t virtAddr) {
	if (!isMMUEnabled())
		return virtAddr;

	TlbEntry tempEntry;
	auto translated = translateAddressUsingTlb(virtAddr, &tempEntry);
	if (holds_alternative<TlbEntry *>(translated)) {
		auto tlbEntry = get<TlbEntry *>(translated);
		return physAddrFromTlbEntry(tlbEntry, virtAddr);
	} else {
		return MaybeU32();
	}
}


MaybeU32 ARM710::readVirtualDebug(uint32_t virtAddr, ValueSize valueSize) {
	if (auto v = virtToPhys(virtAddr); v.has_value())
		return readPhysical(v.value(), valueSize);
	else
		return {};
}


pair<MaybeU32, ARM710::MMUFault> ARM710::readVirtual(uint32_t virtAddr, ValueSize valueSize) {
	if (isAlignmentFaultEnabled() && valueSize == V32 && virtAddr & 3)
		return make_pair(MaybeU32(), encodeFault(AlignmentFault, 0, virtAddr));

	// fast path: cache
#ifdef ARM710T_CACHE
	if (auto v = readCached(virtAddr, valueSize); v.has_value())
		return make_pair(v.value(), NoFault);
#endif

	if (!isMMUEnabled()) {
		// things are very simple without a MMU
		if (auto v = readPhysical(virtAddr, valueSize); v.has_value())
			return make_pair(v.value(), NoFault);
		else
			return make_pair(MaybeU32(), encodeFault(NonMMUError, 0, virtAddr));
	}

	auto translated = translateAddressUsingTlb(virtAddr);
	if (holds_alternative<MMUFault>(translated))
		return make_pair(MaybeU32(), get<MMUFault>(translated));

	// resolve this boy
	auto tlbEntry = get<TlbEntry *>(translated);

	if (auto f = checkAccessPermissions(tlbEntry, virtAddr, false); f != NoFault)
		return make_pair(MaybeU32(), f);

	int domain = (tlbEntry->lv1Entry >> 5) & 0xF;
	bool isPage = (tlbEntry->lv2Entry != 0);

	uint32_t physAddr = physAddrFromTlbEntry(tlbEntry, virtAddr);

#ifdef ARM710T_CACHE
	bool cacheable = tlbEntry->lv2Entry ? (tlbEntry->lv2Entry & 8) : (tlbEntry->lv1Entry & 8);
	if (cacheable && isCacheEnabled())
		return addCacheLineAndRead(physAddr, virtAddr, valueSize, domain, isPage);
	else
#endif
	if (auto result = readPhysical(physAddr, valueSize); result.has_value())
		return make_pair(result, NoFault);
	else
		return make_pair(result, encodeFaultSorP(SorPOtherBusError, isPage, domain, virtAddr));
}

ARM710::MMUFault ARM710::writeVirtual(uint32_t value, uint32_t virtAddr, ValueSize valueSize) {
	if (isAlignmentFaultEnabled() && valueSize == V32 && virtAddr & 3)
		return encodeFault(AlignmentFault, 0, virtAddr);

	if (!isMMUEnabled()) {
		// direct virtual -> physical mapping, sans MMU
		if (!writePhysical(value, virtAddr, valueSize))
			return encodeFault(NonMMUError, 0, virtAddr);
	} else {
		auto translated = translateAddressUsingTlb(virtAddr);
		if (holds_alternative<MMUFault>(translated))
			return get<MMUFault>(translated);

		// resolve this boy
		auto tlbEntry = get<TlbEntry *>(translated);

		if (auto f = checkAccessPermissions(tlbEntry, virtAddr, true); f != NoFault)
			return f;

		uint32_t physAddr = physAddrFromTlbEntry(tlbEntry, virtAddr);
		int domain = (tlbEntry->lv1Entry >> 5) & 0xF;
		bool isPage = (tlbEntry->lv2Entry != 0);

		if (!writePhysical(value, physAddr, valueSize))
			return encodeFaultSorP(SorPOtherBusError, isPage, domain, virtAddr);
	}

	// commit to cache if all was good
#ifdef ARM710T_CACHE
	writeCached(value, virtAddr, valueSize);
#endif
	return NoFault;
}



// TLB
void ARM710::flushTlb() {
	for (TlbEntry &e : tlb)
		e = {0, 0, 0, 0};
}
void ARM710::flushTlb(uint32_t virtAddr) {
	for (TlbEntry &e : tlb) {
		if (e.addrMask && (virtAddr & e.addrMask) == e.addr) {
			e = {0, 0, 0, 0};
			break;
		}
	}
}

ARM710::TlbEntry *ARM710::_allocateTlbEntry(uint32_t addrMask, uint32_t addr) {
	TlbEntry *entry = &tlb[nextTlbIndex];
	entry->addrMask = addrMask;
	entry->addr = addr & addrMask;
	nextTlbIndex = (nextTlbIndex + 1) % TlbSize;
	return entry;
}

variant<ARM710::TlbEntry *, ARM710::MMUFault> ARM710::translateAddressUsingTlb(uint32_t virtAddr, TlbEntry *useMe) {
	// first things first, do we have a matching entry in the TLB?
	for (TlbEntry &e : tlb) {
		if (e.addrMask && (virtAddr & e.addrMask) == e.addr)
			return &e;
	}

	// no, so do a page table walk
	TlbEntry *entry;
	uint32_t tableIndex = virtAddr >> 20;

	// fetch the Level 1 entry
	auto lv1EntryOpt = readPhysical(cp15_translationTableBase | (tableIndex << 2), V32);
	if (!lv1EntryOpt.has_value())
		return Lv1TranslationError;
	auto lv1Entry = lv1EntryOpt.value();
	int domain = (lv1Entry >> 5) & 0xF;

	switch (lv1Entry & 3) {
	case 0:
	case 3:
		// invalid!
		return encodeFault(SectionTranslationFault, domain, virtAddr);
	case 2:
		// a Section entry is straightforward
		// we just throw that immediately into the TLB
		entry = useMe ? useMe : _allocateTlbEntry(0xFFF00000, virtAddr);
		entry->lv1Entry = lv1Entry;
		entry->lv2Entry = 0;
		return entry;
	case 1:
		// a Page requires a Level 2 read
		uint32_t pageTableAddr = lv1Entry & 0xFFFFFC00;
		uint32_t lv2TableIndex = (virtAddr >> 12) & 0xFF;

		auto lv2EntryOpt = readPhysical(pageTableAddr | (lv2TableIndex << 2), V32);
		if (!lv2EntryOpt.has_value())
			return encodeFault(Lv2TranslationError, domain, virtAddr);
		auto lv2Entry = lv2EntryOpt.value();

		switch (lv2Entry & 3) {
		case 0:
		case 3:
			// invalid!
			return encodeFault(PageTranslationFault, domain, virtAddr);
		case 1:
			// Large 64kb page
			entry = useMe ? useMe : _allocateTlbEntry(0xFFFF0000, virtAddr);
			entry->lv1Entry = lv1Entry;
			entry->lv2Entry = lv2Entry;
			return entry;
		case 2:
			// Small 4kb page
			entry = useMe ? useMe : _allocateTlbEntry(0xFFFFF000, virtAddr);
			entry->lv1Entry = lv1Entry;
			entry->lv2Entry = lv2Entry;
			return entry;
		}
	}

	// we should never get here as the switch covers 0, 1, 2, 3
	// but this satisfies a compiler warning
	return SectionTranslationFault;
}



ARM710::MMUFault ARM710::checkAccessPermissions(ARM710::TlbEntry *entry, uint32_t virtAddr, bool isWrite) const {
	int domain;
	int accessPerms;
	bool isPage;

	// extract info from the entries
	domain = (entry->lv1Entry >> 5) & 0xF;
	if (entry->lv2Entry) {
		// Page
		accessPerms = (entry->lv2Entry >> 4) & 0xFF;

		int permIndex;
		if ((entry->lv2Entry & 3) == 1) // Large 64kb
			permIndex = (virtAddr >> 14) & 3;
		else                            // Small 4kb
			permIndex = (virtAddr >> 10) & 3;

		accessPerms >>= (permIndex * 2);
		accessPerms &= 3;
		isPage = true;
	} else {
		// Section
		accessPerms = (entry->lv1Entry >> 10) & 3;
		isPage = false;
	}

	// now, do our checks
	int primaryAccessControls = (cp15_domainAccessControl >> (domain * 2)) & 3;

	// Manager: always allowed
	if (primaryAccessControls == 3)
		return NoFault;

	// Client: enforce checks!
	if (primaryAccessControls == 1) {
#define OK_IF_TRUE(b) return ((b) ? NoFault : encodeFaultSorP(SorPPermissionFault, isPage, domain, virtAddr))
		bool System = cp15_control & 0x100;
		bool ROM = cp15_control & 0x200;

		if (accessPerms == 0) {
			if (!System && !ROM) {
				// 00/0/0: Any access generates a permission fault
				OK_IF_TRUE(false);
			} else if (System && !ROM) {
				// 00/1/0: Supervisor read only permitted
				OK_IF_TRUE(!isWrite && isPrivileged());
			} else if (!System && ROM) {
				// 00/0/1: Any write generates a permission fault
				OK_IF_TRUE(!isWrite);
			} else /*if (System && ROM)*/ {
				// Reserved
				OK_IF_TRUE(false);
			}
		} else if (accessPerms == 1) {
			// 01/x/x: Access allowed only in Supervisor mode
			OK_IF_TRUE(isPrivileged());
		} else if (accessPerms == 2) {
			// 10/x/x: Writes in User mode cause permission fault
			OK_IF_TRUE(!isWrite || isPrivileged());
		} else /*if (accessPerms == 3)*/ {
			// 11/x/x: All access types permitted in both modes
			OK_IF_TRUE(true);
		}
#undef OK_IF_TRUE
	}

	// No Access or Reserved: never allowed (Domain Fault)
	return encodeFaultSorP(SorPDomainFault, isPage, domain, virtAddr);
}


void ARM710::reportFault(MMUFault fault) {
	if (fault != NoFault) {
		if ((fault & 0xF) != NonMMUError) {
			cp15_faultStatus = fault & (MMUFaultTypeMask | MMUFaultDomainMask);
			cp15_faultAddress = fault >> MMUFaultAddressShift;
		}

		static const char *faultTypes[] = {
			"NoFault",
			"AlignmentFault",
			"???",
			"NonMMUError",
			"SectionLinefetchError",
			"SectionTranslationFault",
			"PageLinefetchError",
			"PageTranslationFault",
			"SectionOtherBusError",
			"SectionDomainFault",
			"PageOtherBusError",
			"PageDomainFault",
			"Lv1TranslationError",
			"SectionPermissionFault",
			"Lv2TranslationError",
			"PagePermissionFault"
		};
		log("⚠️ Fault type=%s domain=%d address=%08x pc=%08x lr=%08x",
			faultTypes[fault & MMUFaultTypeMask],
			(fault & MMUFaultDomainMask) >> MMUFaultDomainShift,
			fault >> MMUFaultAddressShift,
			GPRs[15], GPRs[14]);

		// this signals a branch to DataAbort after the
		// instruction is done executing
		faultTriggeredThisCycle = true;
	}
}


void ARM710::log(const char *format, ...) {
	if (logger) {
		char buffer[1024];

		va_list vaList;
		va_start(vaList, format);
		vsnprintf(buffer, sizeof(buffer), format, vaList);
		va_end(vaList);

		logger(buffer);
	}
}

void ARM710::logPcHistory() {
	for (int i = 0; i < PcHistoryCount; i++) {
		pcHistoryIndex = (pcHistoryIndex + 1) % PcHistoryCount;
		log("%03d: %08x %08x", i, pcHistory[pcHistoryIndex].addr, pcHistory[pcHistoryIndex].insn);
	}
}
