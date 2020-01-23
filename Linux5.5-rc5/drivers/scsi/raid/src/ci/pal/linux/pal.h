/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 * 
 * pal.h			PAL: Platform Abstraction Layer
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"

#include "pal_atomic.h"
#include "pal_bitops.h"
#include "pal_dbg.h"
#include "pal_imp.h"
#include "pal_lib.h"
#include "pal_shared.h"
#include "pal_type.h"
#include "pal_util.h"
#include "pal_wrap.h"

#include "xor/pal_xor.h"

#define pal_malloc(size)		\
	({		\
		u8 *__ptr__ = (u8 *)malloc(size);		\
		pal_assert(__ptr__, "No Memory");		\
		ci_dbg_exec(ci_memset(__ptr__, CI_BUF_PTN_INIT, size));		\
		__ptr__;		\
	})

#define pal_zalloc(size)		\
	({		\
		u8 *__ptr__ = (u8 *)pal_malloc(size);		\
		ci_memzero(__ptr__, size);	\
		__ptr__;		\
	})

#define pal_aligned_malloc(size, align)		\
	({		\
		int __rv__;		\
		void *__ptr__ = NULL;		\
		__rv__ = posix_memalign(&__ptr__, align, size);		\
		ci_panic_if(__rv__, "NO MEMORY");		\
		ci_dbg_exec(ci_memset(__ptr__, CI_BUF_PTN_INIT, size));		\
		(u8 *)__ptr__;		\
	})

#define pal_numa_alloc(numa_id, size)		\
	({	\
		u8 *__ptr__ = (u8 *)numa_alloc_onnode(size, numa_id);		/* reversed args */		\
		pal_panic_if(!__ptr__, "PAL NUMA allocation failed");	\
		ci_dbg_exec(ci_memset(__ptr__, CI_BUF_PTN_INIT, size));		\
		__ptr__;	\
	})

#define pal_numa_free(ptr, size)			numa_free(ptr, size)


int  pal_init();
int  pal_init_done();
int  pal_finz();
void pal_time_str(char *buf, int size, int local);
void pal_utc_time_str(char *buf, int size);
void pal_local_time_str(char *buf, int size);

int pal_numa_id_by_ptr(void *ptr);
int pal_numa_id_by_stack();

#define pal_msleep(x)							pal_usleep((x) * 1000L)
#define pal_sleep(x)							pal_msleep((x) * 1000L)
void pal_usleep(long usec);
void pal_nsleep(long nsec);
void pal_nsleep_no_ctx(long nsec);	/* without context check */


