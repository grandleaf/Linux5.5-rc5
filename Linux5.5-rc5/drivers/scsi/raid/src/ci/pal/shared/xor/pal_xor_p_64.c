/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor_p_64.c  					Do XOR with C
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#ifdef PAL_XOR_SIMD

#define XOR_PREFETCH_STEP					1			/* prefetch N cache lines for the best performance */
#define SIMD_REG_SIZE_BIT					64			/* register width, general=64, XMM=128 and YMM=256 */
#define XOR_CACHE_LINE_REPEAT				0			/* Dont change this.  If 1, iterate one cache line */

/* include this file after above definitions */
#include "pal_xor_intrin.h"

xor_p_simd_code_gen();

void pal_xor_p_64(void **buf, int nr, int size, int offset)
{
	ci_align_check(size, sizeof(__m64));
	xor_p_simd_fn[nr](buf, size, offset);
}

#endif	/* !PAL_XOR_SIMD */

