/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ISA_INLINES_H
#define ISA_INLINES_H

#include "macros.h"

#define ARM_SIGN(I) ((I) >> 31)
#define ARM_SXT_8(I) (((int8_t) (I) << 24) >> 24)
#define ARM_SXT_16(I) (((int16_t) (I) << 16) >> 16)
#define ARM_UXT_64(I) (uint64_t)(uint32_t) (I)

#define ARM_CARRY_FROM(M, N, D) (((uint32_t) (M) >> 31) + ((uint32_t) (N) >> 31) > ((uint32_t) (D) >> 31))
#define ARM_BORROW_FROM(M, N, D) (((uint32_t) (M)) >= ((uint32_t) (N)))
#define ARM_BORROW_FROM_CARRY(M, N, D, C) (ARM_UXT_64(M) >= (ARM_UXT_64(N)) + (uint64_t) (C))
#define ARM_V_ADDITION(M, N, D) (!(ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))))
#define ARM_V_SUBTRACTION(M, N, D) ((ARM_SIGN((M) ^ (N))) && (ARM_SIGN((M) ^ (D))))

#endif
