/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_cfg.h			PAL configurations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_dbg.h"
#include "pal_dbg.h"

#define PAL_PR_UNICODE									/* printf can use unicode character */

#define PAL_NUMA_NR							4			/* max numa nodes allowed */
#define PAL_CPU_PER_NUMA					128			/* max logical CPUs per numa */
#define PAL_CPU_TOTAL						512			/* do it manually: PAL_CPU_PER_NUMA * PAL_NUMA_NR */

#define PAL_MEMORY_PER_CPU					ci_mib(8)
#define PAL_MEMORY_SHARED					ci_mib(16)	/* shared memory between all numa nodes, might be interleaved */
#define PAL_WORKER_STACK_SIZE				ci_kib(512)

#define PAL_JIFFIE_PER_SEC					200			/* jiffies per second, smaller value more precise but also more overhead */
#define PAL_STACK_INIT_PTN					0xCCCCCC0299792458ull		/* initialize the stack with this value */

/* for print */
#ifdef CI_DEBUG
#define CI_PR_BLOCK_LOCK_TIMEOUT						(50 * 1000000)	/* in nano-second, 50ms */
#else
#define CI_PR_BLOCK_LOCK_TIMEOUT						(10 * 1000000)	/* in nano-second, 10ms */
#endif


//#define PAL_DISABLE_TIMER
//#define PAL_XOR_SIMD								/* use Intel's SSE, AVX, AVX2 */
#define PAL_XOR_NON_TEMPORAL						/* avoid cache pollution */

//#define PAL_BITOPS_C								/* use C code for bitops, slow */




/*
 *	some hardcoded value, not likely to chanage
 */
#define PAL_CPU_CACHE_LINE_SIZE				64		/* in bytes, defined by hardware */
#define PAL_CPU_ALIGN_SIZE					ci_sizeof(void *)

#ifndef __USE_GNU
#define __USE_GNU
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#pragma GCC diagnostic ignored "-Waddress"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"




#if 0
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif






