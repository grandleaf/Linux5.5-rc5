/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor_p_256.c  					Do XOR with Intel's AVX 256
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#ifdef PAL_XOR_SIMD

#define XOR_PREFETCH_STEP					2			/* prefetch N cache lines for the best performance */
#define SIMD_REG_SIZE_BIT					256			/* register width, general=64, XMM=128 and YMM=256 */
#define XOR_CACHE_LINE_REPEAT				1			/* if 1, iterate one cache line */

/* include this file after above definitions */
#include "pal_xor_intrin.h"

xor_p_simd_code_gen();

void pal_xor_p_256(void **buf, int nr, int size, int offset)
{
	ci_align_check((uintptr_t)buf[0] + offset, PAL_CPU_CACHE_LINE_SIZE);
	ci_align_check(size, PAL_CPU_CACHE_LINE_SIZE);

	xor_p_simd_fn[nr](buf, size, offset);
}

#endif	/* !PAL_XOR_SIMD */

