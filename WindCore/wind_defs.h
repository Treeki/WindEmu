#pragma once
#include <stdint.h>

enum {
	CLOCK_SPEED = 0x9000*1000,
	TICK_INTERVAL = CLOCK_SPEED / 64
};

enum Interrupt {
	EXTFIQ = 0,  // FiqExternal
	BLINT = 1,   // FiqBatLow
	WEINT = 2,   // FiqWatchDog
	MCINT = 3,   // FiqMediaChg
	CSINT = 4,   // IrqCodec
	EINT1 = 5,   // IrqExt1
	EINT2 = 6,   // IrqExt2
	EINT3 = 7,   // IrqExt3
	TC1OI = 8,   // IrqTimer1
	TC2OI = 9,   // IrqTimer2
	RTCMI = 10,  // IrqRtcMatch
	TINT = 11,   // IrqTick
	UART1 = 12,  // IrqUart1
	UART2 = 13,  // IrqUart1
	LCDINT = 14, // IrqLcd
	SSEOTI = 15,  // IrqSpi
	FIQ_INTERRUPTS = 0x000F,
	IRQ_INTERRUPTS = 0xFFF0
};

enum WindermereReg {
	MEMCFG1 = 0,
	MEMCFG2 = 4,
	DRAM_CFG = 0x100,
	LCDCTL = 0x200,
	LCDST = 0x204,
	LCD_DBAR1 = 0x210,
	LCDT0 = 0x220,
	LCDT1 = 0x224,
	LCDT2 = 0x228,
	PWRSR = 0x400,
	PWRCNT = 0x404,
	HALT = 0x408,
	STBY = 0x40C,
	BLEOI = 0x410,
	MCEOI = 0x414,
	TEOI = 0x418,
	STFCLR = 0x41C,
	E2EOI = 0x420,
	INTSR = 0x500,
	INTRSR = 0x504,
	INTENS = 0x508,
	INTENC = 0x50C,
	INTTEST1 = 0x514,
	INTTEST2 = 0x518,
	PUMPCON = 0x900,
	CODR = 0xA00,
	CONFG = 0xA04,
	COLFG = 0xA08,
	COEOI = 0xA0C,
	COTEST = 0xA10,
	SSCR0 = 0xB00,
	SSCR1 = 0xB04,
	SSDR = 0xB0C,
	SSSR = 0xB14,
	TC1LOAD = 0xC00,
	TC1VAL = 0xC04,
	TC1CTRL = 0xC08,
	TC1EOI = 0xC0C,
	TC2LOAD = 0xC20,
	TC2VAL = 0xC24,
	TC2CTRL = 0xC28,
	TC2EOI = 0xC2C,
	BZCONT = 0xC40,
	RTCDRL = 0xD00,
	RTCDRU = 0xD04,
	RTCMRL = 0xD08,
	RTCMRU = 0xD0C,
	RTCEOI = 0xD10,
	PADR = 0xE00,
	PBDR = 0xE04,
	PCDR = 0xE08,
	PDDR = 0xE0C,
	PADDR = 0xE10,
	PBDDR = 0xE14,
	PCDDR = 0xE18,
	PDDDR = 0xE1C,
	PEDR = 0xE20,
	PEDDR = 0xE24,
	KSCAN = 0xE28,
	LCDMUX = 0xE2C
};

void windDiffPorts(uint32_t oldval, uint32_t newval);
void windDiffInterrupts(uint16_t oldval, uint16_t newval);
