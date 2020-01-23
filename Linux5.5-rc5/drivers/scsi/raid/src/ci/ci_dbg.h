/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_dbg.h				CI Debug Switches
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

//#define CI_MEM_GUARD_CACHE_LINE		/* memory guard align to cache line */

#ifndef NDEBUG
#define CI_DEBUG						/* must be defined if you want to define the following */

#define CI_LIST_DEBUG					/* detect double add/free, etc ... */
#define CI_CLIST_COUNT_CHECK			/* very slow, use with caution */
#define CI_BALLOC_DEBUG					/* memory overwritten detection, tail non-strict detection (has hole) */
#define CI_SCHED_DEBUG					/* scheduler debug */
#define CI_PAVER_DEBUG					/* paver debug */
#define CI_PAVER_DEBUG_EXTRA			/* extra debug for paver, slow */
#define CI_STA_DEBUG					/* for statistics */

#define __RELEASE_DEBUG__						"Debug"
#else
#define __RELEASE_DEBUG__						"Release"
#endif

#define CI_POISON_LIST_PREV						0x01
#define CI_POISON_LIST_NEXT						0x03

#define CI_BUF_PTN_INIT							0xCC


#define ci_ptr_is_uninited(p)					(((p) == NULL) || ((uintptr_t)(p) == (uintptr_t)0xCCCCCCCCCCCCCCCCull))	/* see CI_BUF_PTN_INIT */


#ifdef CI_DEBUG
#define ci_dbg_paste(...)						__VA_ARGS__
#define ci_dbg_exec(...)						do { __VA_ARGS__; } while (0)
#else
#define ci_dbg_paste(...)							
#define ci_dbg_exec(...)						do {} while (0)	
#endif


