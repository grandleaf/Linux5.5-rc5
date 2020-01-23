/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor_q_128.c  					Do GF-XOR with Intel's AVX 128
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#ifdef PAL_XOR_SIMD

#define XOR_PREFETCH_STEP					2			/* prefetch N cache lines for the best performance */
#define SIMD_REG_SIZE_BIT					128			/* register width, general=64, XMM=128 and YMM=256 */
#define XOR_CACHE_LINE_REPEAT				1			/* if 1, iterate one cache line */

/* include this file after above definitions */
#include "pal_xor_intrin.h"

xor_q_simd_code_gen();

void pal_xor_q_128(void **buf, u8 *coef, int nr, int size, int offset)
{
	ci_align_check((uintptr_t)buf[0] + offset, PAL_CPU_CACHE_LINE_SIZE);
	ci_align_check(size, PAL_CPU_CACHE_LINE_SIZE);

	xor_q_simd_fn[nr](buf, coef, size, offset);
}

#endif	/* !PAL_XOR_SIMD */

