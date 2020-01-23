/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_cfg.h				CI Configurations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal.h"
#include "ci_dbg.h"

/* uncomment it if you want generate the same pattern */
//#define CI_RAND_SEED									1


/* statistic for each module, might impact performance */
//#define CI_WORKER_STA
//#define CI_PAVER_STA


/* frequently use */
#define CI_PR_MEM_BUF_SIZE								ci_kib(512)	/* use this if console is not available */
#define CI_BALLOC_SHR_SIZE								ci_balloc_mem_eval(ci_mib(2))		/* shared ba */
#define CI_BALLOC_JSON_SIZE								ci_balloc_mem_eval(ci_kib(512))		/* for json objects */
#define CI_BALLOC_NODE_SIZE								ci_balloc_mem_eval(ci_mib(12))		/* ba for each node */
#define CI_BALLOC_PAVER_SIZE							ci_balloc_mem_eval(ci_mib(12))		/* ba for pavers */


/* for scheduler */
#define CI_SCHED_ENABLE_SMT											/* always enabled: smt scheduling */
#define CI_SCHED_PRIO_NR								16			/* how many priority levels */
#define CI_SCHED_PRIO_NORMAL							8
#define CI_SCEHD_PRIO_MOD_VECT							3			/* for module's vector */
#define CI_SCHED_PRIO_TIMER								1			/* for timer */

#define CI_SCHED_DPT_NR									128			/* total lad/sid for a node */
#define CI_SCHED_EXT_CHK_INT							16			/* external table check interval */
#define CI_SCHED_BURST									8			/* batching */
#define CI_SCHED_QUOTA									24			/* quota */
#define CI_SCHED_GRP_PAVER_NR							8			/* maximum different pavers that requires */


/* for paver */
#define CI_PAVER_NR										64			/* how many paver allocators */
#define CI_PAVER_ALLOC_SHIFT							10			/* minimum paver alloc size from buddy system, 1 << 14 = 16K */
#define CI_PAVER_MUL_AVAIL								3			/* default multiply factor for avail, satisfy, ... */
#define CI_PAVER_MUL_SATISFY							6			/* a paver keep on ask for resource until hit satisfy */
#define CI_PAVER_MUL_RET_LOWER							8			/* when ret_upper hit, return resource until hit ret_lower */
#define CI_PAVER_MUL_RET_UPPER							16			/* work together with ret_lower */
#define CI_PAVER_MUL_HOLD_MIN							10			/* don't return any resource if total resource < min */
#define CI_PAVER_MUL_HOLD_MAX							24			/* don't ask for more resource if total resource > max */ 


/* for bitmap: ci_bmp_t */
#define CI_BMP_SMALL_CONST_BITS							256			/* do optimization for small bitmap */
#define CI_INF_LOOP_DETECT_CNT							1000000		/* consider it is a inf loop if cnt > this */


/* for worker */
#define CI_WORKER_DATA_NR								64			/* per worker data */

/* for timer */
#define CI_TIMER_WHEEL_NR								5			/* 34 years if 1 jiffie = 1ms */


/* for module */
#define CI_MOD_SID_VECT_NAME_SIZE						1024		/* store the built-in module's sid vect name */


#define CI_PR_BUF_SIZE									2048		/* ci_printf's print buffer size */
#define CI_PR_META_MAX_FILE_LEN							20			/* max file length to be printed out */
#define CI_PR_META_MAX_FUNC_LEN							32			/* max function length to be printed out */
#define CI_PR_SSF_LEN									32			/* length for small length string format */
#define CI_PR_MSF_LEN									128			/* length for middle length string format */
#define CI_PR_LSF_LEN									256			/* length for large length string format */
#define CI_PR_FLAG_LEN									128			/* length for print flags */
#define CI_PR_JSON_BIN_LEN								32
#define CI_PRF_META_DFT									(CI_PRF_META_SEQ | CI_PRF_META_LOCAL_TIME | CI_PRF_META_SCHED_CTX)		/* flag: default print meta */

#define CI_MAX_FILE_NAME_LEN							256			/* debug print purpose */
#define CI_MAX_FUNC_NAME_LEN							128


/* for balloc: buddy system memory allocator */
#define CI_BALLOC_MIN_SHIFT								6			/* 1 << 6 == 64 bytes */
#define CI_BALLOC_MAX_SHIFT								24			/* 1 << 24 == 16 MiB */
#define CI_BALLOC_NR_BUCKET								25			/* manually: CI_BALLOC_MAX_SHIFT + 1 */
ci_static_assert((1 << CI_BALLOC_MIN_SHIFT) >= PAL_CPU_CACHE_LINE_SIZE);
ci_static_assert(!(CI_BALLOC_MAX_SHIFT & 0x80));
ci_static_assert(CI_BALLOC_NR_BUCKET == CI_BALLOC_MAX_SHIFT + 1);


/* chores */
#define CI_NODE_NR										PAL_NUMA_NR
#define CI_WORKER_NR									PAL_CPU_PER_NUMA

