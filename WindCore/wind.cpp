#include "wind.h"
#include <stdio.h>

void diffPorts(uint32_t oldval, uint32_t newval) {
	uint32_t changes = oldval ^ newval;
	if (changes & 1) printf("PRT codec enable: %d\n", newval&1);
	if (changes & 2) printf("PRT audio amp enable: %d\n", newval&2);
	if (changes & 4) printf("PRT lcd power: %d\n", newval&4);
	if (changes & 8) printf("PRT etna door: %d\n", newval&8);
	if (changes & 0x10) printf("PRT sled: %d\n", newval&0x10);
	if (changes & 0x20) printf("PRT pump pwr2: %d\n", newval&0x20);
	if (changes & 0x40) printf("PRT pump pwr1: %d\n", newval&0x40);
	if (changes & 0x80) printf("PRT etna err: %d\n", newval&0x80);
	if (changes & 0x100) printf("PRT rs-232 rts: %d\n", newval&0x100);
	if (changes & 0x200) printf("PRT rs-232 dtr toggle: %d\n", newval&0x200);
	if (changes & 0x400) printf("PRT disable power led: %d\n", newval&0x400);
	if (changes & 0x800) printf("PRT enable uart1: %d\n", newval&0x800);
	if (changes & 0x1000) printf("PRT lcd backlight: %d\n", newval&0x1000);
	if (changes & 0x2000) printf("PRT enable uart0: %d\n", newval&0x2000);
	if (changes & 0x4000) printf("PRT dictaphone: %d\n", newval&0x4000);
// PROM read process makes this super spammy in stdout
//	if (changes & 0x10000) printf("PRT EECS: %d\n", newval&0x10000);
//	if (changes & 0x20000) printf("PRT EECLK: %d\n", newval&0x20000);
	if (changes & 0x40000) printf("PRT contrast0: %d\n", newval&0x40000);
	if (changes & 0x80000) printf("PRT contrast1: %d\n", newval&0x80000);
	if (changes & 0x100000) printf("PRT contrast2: %d\n", newval&0x100000);
	if (changes & 0x200000) printf("PRT contrast3: %d\n", newval&0x200000);
	if (changes & 0x400000) printf("PRT case open: %d\n", newval&0x400000);
	if (changes & 0x800000) printf("PRT etna cf power: %d\n", newval&0x800000);
	if (changes & 0x1000000) printf("PRT kb0: %d\n", newval&0x1000000);
	if (changes & 0x2000000) printf("PRT kb1: %d\n", newval&0x2000000);
	if (changes & 0x4000000) printf("PRT kb2: %d\n", newval&0x4000000);
	if (changes & 0x8000000) printf("PRT kb3: %d\n", newval&0x8000000);
	if (changes & 0x10000000) printf("PRT kb4: %d\n", newval&0x10000000);
	if (changes & 0x20000000) printf("PRT kb5: %d\n", newval&0x20000000);
	if (changes & 0x40000000) printf("PRT kb6: %d\n", newval&0x40000000);
	if (changes & 0x80000000) printf("PRT kb7: %d\n", newval&0x80000000);
}

void diffInterrupts(uint16_t oldval, uint16_t newval) {
	uint16_t changes = oldval ^ newval;
	if (changes & 1) printf("INTCHG external=%d\n", newval & 1);
	if (changes & 2) printf("INTCHG lowbat=%d\n", newval & 2);
	if (changes & 4) printf("INTCHG watchdog=%d\n", newval & 4);
	if (changes & 8) printf("INTCHG mediachg=%d\n", newval & 8);
	if (changes & 0x10) printf("INTCHG codec=%d\n", newval & 0x10);
	if (changes & 0x20) printf("INTCHG ext1=%d\n", newval & 0x20);
	if (changes & 0x40) printf("INTCHG ext2=%d\n", newval & 0x40);
	if (changes & 0x80) printf("INTCHG ext3=%d\n", newval & 0x80);
	if (changes & 0x100) printf("INTCHG timer1=%d\n", newval & 0x100);
	if (changes & 0x200) printf("INTCHG timer2=%d\n", newval & 0x200);
	if (changes & 0x400) printf("INTCHG rtcmatch=%d\n", newval & 0x400);
	if (changes & 0x800) printf("INTCHG tick=%d\n", newval & 0x800);
	if (changes & 0x1000) printf("INTCHG uart1=%d\n", newval & 0x1000);
	if (changes & 0x2000) printf("INTCHG uart2=%d\n", newval & 0x2000);
	if (changes & 0x4000) printf("INTCHG lcd=%d\n", newval & 0x4000);
	if (changes & 0x8000) printf("INTCHG spi=%d\n", newval & 0x8000);
}
