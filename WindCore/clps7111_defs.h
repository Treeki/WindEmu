#pragma once
#include <stdint.h>

enum {
	CLOCK_SPEED = 0x4800*1000,
	TICK_INTERVAL = CLOCK_SPEED / 64
};

enum Interrupt {
	EXTFIQ = 0,   // FiqExternal
	BLINT = 1,    // FiqBatLow
	WEINT = 2,    // FiqWatchDog
	MCINT = 3,    // FiqMediaChg
	CSINT = 4,    // IrqCodec
	EINT1 = 5,    // IrqExt1
	EINT2 = 6,    // IrqExt2
	EINT3 = 7,    // IrqExt3
	TC1OI = 8,    // IrqTimer1
	TC2OI = 9,    // IrqTimer2
	RTCMI = 10,   // IrqRtcMatch
	TINT = 11,    // IrqTick
	UTXINT = 12,  // IrqUartTx?
	URXINT1 = 13, // IrqUartRx?
	UMSINT = 14,  // IrqUartModem?
	SSEOTI = 15,  // IrqSpi
	KBDINT = 16,  // IrqKeyPress?
	UTXINT2 = 28, // IrqSpi2Tx?
	URXINT2 = 29, // IrqSpi2Rx?
	FIQ_INTERRUPTS = 0x000F,
	IRQ_INTERRUPTS = 0xFFF0
};

enum Clps7111Reg {
	PADR = 0,
	PBDR = 1,
	PDDR = 3,
	PADDR = 0x40,
	PBDDR = 0x41,
	PDDDR = 0x43,
	PEDR = 0x80,
	PEDDR = 0xC0,
	SYSCON1 = 0x100,
	SYSFLG1 = 0x140,
	MEMCFG1 = 0x180,
	MEMCFG2 = 0x1C0,
	DRFPR = 0x200,
	INTSR1 = 0x240,
	INTMR1 = 0x280,
	LCDCON = 0x2C0,
	TC1D = 0x300,
	TC2D = 0x340,
	RTCDR = 0x380,
	RTCMR = 0x3C0,
	PMPCON = 0x400,
	CODR = 0x440,
	UARTDR1 = 0x480,
	UBRLCR1 = 0x4C0,
	SYNCIO = 0x500,
	PALLSW = 0x540,
	PALMSW = 0x580,
	STFCLR = 0x5C0,
	BLEOI = 0x600,
	MCEOI = 0x640,
	TEOI = 0x680,
	TC1EOI = 0x6C0,
	TC2EOI = 0x700,
	RTCEOI = 0x740,
	UMSEOI = 0x780,
	COEOI = 0x7C0,
	HALT = 0x800,
	STDBY = 0x840,
	FRBADDR = 0x1000,
	SYSCON2 = 0x1100,
	SYSFLG2 = 0x1140,
	INTSR2 = 0x1240,
	INTMR2 = 0x1280,
	UARTDR2 = 0x1480,
	UBRLCR2 = 0x14C0,
	KBDEOI = 0x1700
};

