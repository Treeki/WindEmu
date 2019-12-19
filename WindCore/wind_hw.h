#pragma once
#include "wind.h"
#include <stdio.h>

struct Timer {
	struct ARMCore *cpu;

	enum {
		TICK_INTERVAL_SLOW = CLOCK_SPEED / 2000,
		TICK_INTERVAL_FAST = CLOCK_SPEED / 512000,
		MODE_512KHZ = 1<<3,
		PERIODIC = 1<<6,
		ENABLED = 1<<7
	};
    int64_t lastTicked;
	uint8_t config;
	uint32_t interval;
	int32_t value;

	int tickInterval() const {
		return (config & MODE_512KHZ) ? TICK_INTERVAL_FAST : TICK_INTERVAL_SLOW;
	}
	void load(uint32_t lval) {
		interval = lval;
		value = lval;
	}
    bool tick(int64_t cycles) {
		if (cycles >= (lastTicked + tickInterval())) {
			lastTicked += tickInterval();

			if (config & ENABLED) {
				--value;
				if (value == 0) {
					if (config & PERIODIC)
						value = interval;
					return true;
				}
			}
		}
		return false;
	}
	void dump() {
		printf("enabled=%s periodic=%s interval=%d value=%d\n",
			(config & ENABLED) ? "true" : "false",
			(config & PERIODIC) ? "true" : "false",
			interval, value
		);
	}
};

struct UART {
	struct ARMCore *cpu;

	enum {
		IntRx = 1,
		IntTx = 2,
		IntModemStatus = 4,
		PortCtrlEnable = 1,
		PortCtrlSirEnable = 2,
		PortCtrlIrdaTx = 4,
		FrameCtrlBreak = 1,
		FrameCtrlParityEnable = 2,
		FrameCtrlEvenParity = 4,
		FrameCtrlExtraStopBit = 8,
		FrameCtrlUFifoEn = 0x10,
		FrameCtrlWrdLenMask = 0x60,
		FrameCtrlWlen5 = 0,
		FrameCtrlWlen6 = 0x20,
		FrameCtrlWlen7 = 0x40,
		FrameCtrlWlen8 = 0x60,
		RecvFrameError = 0x100,
		RecvParityError = 0x200,
		RecvOverrunError = 0x400,
		FlagClearToSend = 1,
		FlagDataSetReady = 2,
		FlagDataCarrierDetect = 4,
		FlagBusy = 8,
		FlagReceiveFifoEmpty = 0x10,
		FlagTransmitFifoFull = 0x20
	};
	uint8_t portControl = 0;
	uint8_t frameControl = 0;
	uint8_t interrupts = 0, interruptMask = 0;

	// UART0DATA = 0x600, byte write, long read
	// UART0FCR = 0x604, long
	// UART0LCR = 0x608, long
	// UART0CON = 0x60C, byte
	// UART0FLG = 0x610, byte
	// UART0INT = 0x614, long write, byte read
	// UART0INTM = 0x618, byte
	// UART0INTR = 0x61C, byte
	// UART0TEST1 = 0x620,
	// UART0TEST2 = 0x624,
	// UART0TEST3 = 0x628,
	uint32_t readReg8(uint32_t reg) {
		// UART0DATA
		if (reg == (UART0CON & 0xFF)) {
			return portControl;
		} else if (reg == (UART0FLG & 0xFF)) {
			// we pretend we are never busy, never have full fifo
			return FlagReceiveFifoEmpty;
		// UART0INT?
		// UART0INTM?
		// UART0INTR?
		} else {
			printf("unhandled 8bit uart read %x at pc=%08x lr=%08x\n", reg, cpu->gprs[ARM_PC], cpu->gprs[ARM_LR]);
			return 0xFF;
		}
	}
	uint32_t readReg32(uint32_t reg) {
		// UART0DATA
		if (reg == (UART0FCR & 0xFF)) {
			return frameControl;
		// UART0LCR
		} else {
			printf("unhandled 32bit uart read %x at pc=%08x lr=%08x\n", reg, cpu->gprs[ARM_PC], cpu->gprs[ARM_LR]);
			return 0xFFFFFFFF;
		}
	}
	void writeReg8(uint32_t reg, uint8_t value) {
		// UART0DATA
		if (reg == (UART0CON & 0xFF)) {
			portControl = value;
			printf("portcon updated: enable=%d sirenable=%d irdatx=%d\n", value&1, value&2, value&4);
		} else if (reg == (UART0INTM & 0xFF)) {
			interruptMask = value;
			printf("uart interruptmask updated: %d\n", value);
		// UART0INTR?
		} else {
			printf("unhandled 8bit uart write %x value %02x at pc=%08x lr=%08x\n", reg, value, cpu->gprs[ARM_PC], cpu->gprs[ARM_LR]);
		}
	}
	void writeReg32(uint32_t reg, uint32_t value) {
		if (reg == (UART0FCR & 0xFF)) {
			frameControl = value;
			printf("frameControl updated: break=%d parityEn=%d evenParity=%d extraStop=%d ufifoEn=%d wrdLen=%d\n",
				value&1,
				value&2,
				value&4,
				value&8,
				value&0x10,
				((value&0x60)>>5)+5);
		} else if (reg == (UART0LCR & 0xFF)) {
			printf("** uart writing lcr %x **\n", value);
		} else if (reg == (UART0INT & 0xFF)) {
			printf("uart interrupts %x -> %x\n", interrupts, value);
			interrupts = value;
		} else {
			printf("unhandled 32bit uart write %x value %08x at pc=%08x lr=%08x\n", reg, value, cpu->gprs[ARM_PC], cpu->gprs[ARM_LR]);
		}
	}
};
