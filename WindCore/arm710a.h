#pragma once
#include <stdint.h>
#include <optional>
#include <variant>

using namespace std;

// ASSUMPTIONS:
// - Little-endian will be used
// - 26-bit address spaces will not be used
// - Alignment faults will always be on

// Write buffer is 4 address FIFO, 8 data FIFO
// TLB is 64 entries

typedef optional<uint32_t> MaybeU32;

class ARM710a
{
public:
	void test();

	enum ValueSize { V8 = 0, V32 = 1 };

	enum MMUFault : uint64_t {
		// ref: datasheet 9-13 (p111)
		NoFault                 = 0,
		AlignmentFault          = 1,
		// the ARM gods say there is to be no fault 2 or 3
		SectionLinefetchError   = 4,
		SectionTranslationFault = 5,
		PageLinefetchError      = 6,
		PageTranslationFault    = 7,
		SectionOtherBusError    = 8,
		SectionDomainFault      = 9,
		PageOtherBusError       = 0xA,
		PageDomainFault         = 0xB,
		Lv1TranslationError     = 0xC,
		SectionPermissionFault  = 0xD,
		Lv2TranslationError     = 0xE,
		PagePermissionFault     = 0xF,

		// not actually in the ARM datasheet
		// so we are reusing it for nefarious purposes
		NonMMUError             = 3,

		MMUFaultTypeMask        = 0xF,
		MMUFaultDomainMask      = 0xF0,
		MMUFaultDomainShift     = 4,
		MMUFaultAddressMask     = 0xFFFFFFFF00000000,
		MMUFaultAddressShift    = 32
	};



	ARM710a() {
		cp15_id = 0x41047100;
		clearAllValues();
	}
	virtual ~ARM710a() { }

	void clearAllValues() {
		bank = MainBank;
		CPSR = 0;
		for (int i = 0; i < 16; i++) GPRs[i] = 0;
		for (int i = 0; i < 5; i++) {
			fiqBankedRegisters[0][i] = 0;
			fiqBankedRegisters[1][i] = 0;
			SPSRs[i] = 0;
		}
		for (int i = 0; i < 6; i++) {
			allModesBankedRegisters[i][0] = 0;
			allModesBankedRegisters[i][1] = 0;
		}

		cp15_control = 0;
		cp15_translationTableBase = 0;
		cp15_domainAccessControl = 0;
		cp15_faultStatus = 0;
		cp15_faultAddress = 0;
		prefetchCount = 0;
		clearCache();
		flushTlb();
	}

	void setProcessorID(uint32_t v) { cp15_id = v; }
	void requestFIQ(); // pull nFIQ low
	void requestIRQ(); // pull nIRQ low
	void reset();      // pull nRESET low

	bool instructionReady() const { return (prefetchCount == 2); }
	uint32_t tick();   // run the chip for at least 1 clock cycle

	MaybeU32 readVirtualDebug(uint32_t virtAddr, ValueSize valueSize);
	MaybeU32 virtToPhys(uint32_t virtAddr);

	pair<MaybeU32, MMUFault> readVirtual(uint32_t virtAddr, ValueSize valueSize);
	virtual MaybeU32 readPhysical(uint32_t physAddr, ValueSize valueSize) = 0;
	MMUFault writeVirtual(uint32_t value, uint32_t virtAddr, ARM710a::ValueSize valueSize);
	virtual bool writePhysical(uint32_t value, uint32_t physAddr, ARM710a::ValueSize valueSize) = 0;

	uint32_t getGPR(int index) const { return GPRs[index]; }
	uint32_t getCPSR() const { return CPSR; }

	void setLogger(std::function<void(const char *)> newLogger) { logger = newLogger; }
protected:
	void log(const char *format, ...);
private:
	std::function<void(const char *)> logger;

	enum { Nop = 0xE1A00000 };

	enum Mode : uint8_t {
		User32       = 0x10,
		FIQ32        = 0x11,
		IRQ32        = 0x12,
		Supervisor32 = 0x13,
		Abort32      = 0x17,
		Undefined32  = 0x1B
	};

	enum BankIndex : uint8_t {
		FiqBank,
		IrqBank,
		SvcBank,
		AbtBank,
		UndBank,
		MainBank
	};

	constexpr static const BankIndex modeToBank[16] = {
		MainBank, FiqBank,  IrqBank,  SvcBank,
		MainBank, MainBank, MainBank, AbtBank,
		MainBank, MainBank, MainBank, UndBank,
		MainBank, MainBank, MainBank, MainBank
	};

	enum : uint32_t {
		CPSR_ModeMask   = 0x0000001F,
		CPSR_FIQDisable = 0x00000040,
		CPSR_IRQDisable = 0x00000080,
		CPSR_V          = 0x10000000,
		CPSR_C          = 0x20000000,
		CPSR_Z          = 0x40000000,
		CPSR_N          = 0x80000000,
		CPSR_FlagMask   = 0xF0000000
	};

	// active state
	BankIndex bank;
	uint32_t CPSR;
	uint32_t GPRs[16];

	// saved state
	uint32_t fiqBankedRegisters[2][5];      // R8..R12 inclusive
	uint32_t allModesBankedRegisters[6][2]; // R13, R14
	uint32_t SPSRs[5];

	// coprocessor 15
	uint32_t cp15_id;                   // 0: read-only
	uint32_t cp15_control;              // 1: write-only
	uint32_t cp15_translationTableBase; // 2: write-only
	uint32_t cp15_domainAccessControl;  // 3: write-only
	uint8_t  cp15_faultStatus;          // 5: read-only (writing has unrelated effects)
	uint32_t cp15_faultAddress;         // 6: read-only (writing has unrelated effects)

	bool flagV() const { return CPSR & CPSR_V; }
	bool flagC() const { return CPSR & CPSR_C; }
	bool flagZ() const { return CPSR & CPSR_Z; }
	bool flagN() const { return CPSR & CPSR_N; }
	bool checkCondition(int cond) const {
		switch (cond) {
		/*EQ*/ case 0:   return flagZ();
		/*NE*/ case 1:   return !flagZ();
		/*CS*/ case 2:   return flagC();
		/*CC*/ case 3:   return !flagC();
		/*MI*/ case 4:   return flagN();
		/*PL*/ case 5:   return !flagN();
		/*VS*/ case 6:   return flagV();
		/*VC*/ case 7:   return !flagV();
		/*HI*/ case 8:   return flagC() && !flagZ();
		/*LS*/ case 9:   return !flagC() || flagZ();
		/*GE*/ case 0xA: return flagN() == flagV();
		/*LT*/ case 0xB: return flagN() != flagV();
		/*GT*/ case 0xC: return !flagZ() && (flagN() == flagV());
		/*LE*/ case 0xD: return flagZ() || (flagN() != flagV());
		/*AL*/ case 0xE: return true;
		/*NV*/ /*case 0xF:*/
		default:  return false;
		}
	}

	static Mode modeFromCPSR(uint32_t v) { return (Mode)(v & CPSR_ModeMask); }
	Mode currentMode()             const { return modeFromCPSR(CPSR); }
	BankIndex currentBank()        const { return modeToBank[(Mode)(CPSR & 0xF)]; }
	bool isPrivileged()            const { return (CPSR & 0x1F) > User32; }
	bool isMMUEnabled()            const { return (cp15_control & 1); }
	bool isAlignmentFaultEnabled() const { return (cp15_control & 2); }
	bool isCacheEnabled()          const { return (cp15_control & 4); }
	bool isWriteBufferEnabled()    const { return (cp15_control & 8); }

	void switchMode(Mode mode);
	void switchBank(BankIndex bank);
	void raiseException(Mode mode, uint32_t savedPC, uint32_t newPC);

	// MMU/TLB
	enum MMUFaultSorP : uint64_t {
		SorPLinefetchError      = 4,
		SorPTranslationFault    = 5,
		SorPOtherBusError       = 8,
		SorPDomainFault         = 9,
		SorPPermissionFault     = 0xD,
	};

	MMUFault encodeFault(MMUFault fault, int domain, uint32_t virtAddr) const {
		return (MMUFault)(fault | (domain << 4) | ((uint64_t)virtAddr << 32));
	}
	MMUFault encodeFaultSorP(MMUFaultSorP baseFault, bool isPage, int domain, uint32_t virtAddr) const {
		return (MMUFault)(baseFault | (isPage ? 2 : 0) | (domain << 4) | ((uint64_t)virtAddr << 32));
	}

	enum { TlbSize = 64 };
	struct TlbEntry { uint32_t addrMask, addr, lv1Entry, lv2Entry; };
	TlbEntry tlb[TlbSize];
	int nextTlbIndex = 0;

	void flushTlb();
	void flushTlb(uint32_t virtAddr);
	variant<TlbEntry *, MMUFault> translateAddressUsingTlb(uint32_t virtAddr, TlbEntry *useMe=nullptr);
	TlbEntry *_allocateTlbEntry(uint32_t addrMask, uint32_t addr);

	static uint32_t physAddrFromTlbEntry(TlbEntry *tlbEntry, uint32_t virtAddr);
	MMUFault checkAccessPermissions(TlbEntry *entry, uint32_t virtAddr, bool isWrite) const;

	bool faultTriggeredThisCycle = false;
	void reportFault(MMUFault fault);

	// Instruction/Data Cache
	enum {
		CacheSets = 4,
		CacheBlocksPerSet = 128,
		CacheBlockSize = 0x10,

		CacheAddressLineMask = 0x0000000F,
		CacheAddressSetMask  = 0x00000030, CacheAddressSetShift = 4,
		CacheAddressTagMask  = 0xFFFFFFC0,

		CacheBlockEnabled = 1
	};
	uint32_t cacheBlockTags[CacheSets][CacheBlocksPerSet];
	uint8_t cacheBlocks[CacheSets][CacheBlocksPerSet][CacheBlockSize];

	void clearCache();
	uint8_t *findCacheLine(uint32_t virtAddr);
	pair<MaybeU32, MMUFault> addCacheLineAndRead(uint32_t physAddr, uint32_t virtAddr, ValueSize valueSize, int domain, bool isPage);
	MaybeU32 readCached(uint32_t virtAddr, ValueSize valueSize);
	bool writeCached(uint32_t value, uint32_t virtAddr, ValueSize valueSize);

	// Instruction Loop
	int prefetchCount;
	uint32_t prefetch[2];
	MMUFault prefetchFaults[2];

	uint32_t executeInstruction(uint32_t insn);

	uint32_t execDataProcessing(bool I, uint32_t Opcode, bool S, uint32_t Rn, uint32_t Rd, uint32_t Operand2);
	uint32_t execMultiply(uint32_t AS, uint32_t Rd, uint32_t Rn, uint32_t Rs, uint32_t Rm);
	uint32_t execSingleDataSwap(bool B, uint32_t Rn, uint32_t Rd, uint32_t Rm);
	uint32_t execSingleDataTransfer(uint32_t IPUBWL, uint32_t Rn, uint32_t Rd, uint32_t offset);
	uint32_t execBlockDataTransfer(uint32_t PUSWL, uint32_t Rn, uint32_t registerList);
	uint32_t execBranch(bool L, uint32_t offset);
	uint32_t execCP15RegisterTransfer(uint32_t CPOpc, bool L, uint32_t CRn, uint32_t Rd, uint32_t CP, uint32_t CRm);
};
