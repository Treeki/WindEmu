/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ISA_INLINES_H
#define ISA_INLINES_H

#include "macros.h"

#include "arm.h"

#define ARM_COND_EQ (cpu->cpsr.z)
#define ARM_COND_NE (!cpu->cpsr.z)
#define ARM_COND_CS (cpu->cpsr.c)
#define ARM_COND_CC (!cpu->cpsr.c)
#define ARM_COND_MI (cpu->cpsr.n)
#define ARM_COND_PL (!cpu->cpsr.n)
#define ARM_COND_VS (cpu->cpsr.v)
#define ARM_COND_VC (!cpu->cpsr.v)
#define ARM_COND_HI (cpu->cpsr.c && !cpu->cpsr.z)
#define ARM_COND_LS (!cpu->cpsr.c || cpu->cpsr.z)
#define ARM_COND_GE (!cpu->cpsr.n == !cpu->cpsr.v)
#define ARM_COND_LT (!cpu->cpsr.n != !cpu->cpsr.v)
#define ARM_COND_GT (!cpu->cpsr.z && !cpu->cpsr.n == !cpu->cpsr.v)
#define ARM_COND_LE (cpu->cpsr.z || !cpu->cpsr.n != !cpu->cpsr.v)
#define ARM_COND_AL 1

#define ARM_SIGN(I) ((I) >> 31)
#define ARM_SXT_8(I) (((int8_t) (I) << 24) >> 24)
#define ARM_SXT_16(I) (((int16_t) (I) << 16) >> 16)
#define ARM_UXT_64(I) (uint64_t)(uint32_t) (I)

#define ARM_CARRY_FROM(M, N, D) (((uint32_t) (M) >> 31) + ((uint32_t) (N) >> 31) > ((uint32_t) (D) >> 31))
#define ARM_BORROW_FROM(M, N, D) (((uint32_t) (M)) >= ((uint32_t) (N)))
#define ARM_BORROW_FROM_CARRY(M, N, D, C) (ARM_UXT_64(M) >= (ARM_UXT_64(N)) + (uint64_t) (C))
#define ARM_V_ADDITION(M, N, D) (!(ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))))
#define ARM_V_SUBTRACTION(M, N, D) ((ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))))

#define ARM_WAIT_MUL(R)                                                   \
	{                                                                     \
		int32_t wait;                                                     \
		if ((R & 0xFFFFFF00) == 0xFFFFFF00 || !(R & 0xFFFFFF00)) {        \
			wait = 1;                                                     \
		} else if ((R & 0xFFFF0000) == 0xFFFF0000 || !(R & 0xFFFF0000)) { \
			wait = 2;                                                     \
		} else if ((R & 0xFF000000) == 0xFF000000 || !(R & 0xFF000000)) { \
			wait = 3;                                                     \
		} else {                                                          \
			wait = 4;                                                     \
		}                                                                 \
		currentCycles += cpu->memory.stall(cpu, wait);                    \
	}

#define ARM_STUB cpu->irqh.hitStub(cpu, opcode)
#define ARM_ILL cpu->irqh.hitIllegal(cpu, opcode)

static inline int32_t ARMWritePC(struct ARMCore* cpu) {
	cpu->gprs[ARM_PC] = (cpu->gprs[ARM_PC] & -4);
	// cpu->memory.setActiveRegion(cpu, cpu->gprs[ARM_PC]);
	// LOAD_32(cpu->prefetch[0], cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion);
	cpu->prefetch[0] = cpu->memory.load32(cpu, cpu->gprs[ARM_PC], NULL);
	cpu->gprs[ARM_PC] += 4;
	// LOAD_32(cpu->prefetch[1], cpu->gprs[ARM_PC] & cpu->memory.activeMask, cpu->memory.activeRegion);
	cpu->prefetch[1] = cpu->memory.load32(cpu, cpu->gprs[ARM_PC], NULL);
	return 2 + cpu->memory.activeNonseqCycles32 + cpu->memory.activeSeqCycles32;
}

static inline int _ARMModeHasSPSR(enum PrivilegeMode mode) {
	return mode != MODE_SYSTEM && mode != MODE_USER;
}

static inline void _ARMReadCPSR(struct ARMCore* cpu) {
	ARMSetPrivilegeMode(cpu, cpu->cpsr.priv);
	cpu->irqh.readCPSR(cpu);
}

static inline uint32_t _ARMPCAddress(struct ARMCore* cpu) {
	return cpu->gprs[ARM_PC] - 4 * 2;
}

#endif
