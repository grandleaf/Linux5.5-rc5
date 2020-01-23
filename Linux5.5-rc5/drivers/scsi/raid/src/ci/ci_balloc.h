/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_balloc.h			Buddy System Memory Allocator (without free functions)
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_list.h"
#include "ci_bmp.h"

#define CI_BALLOC_MIN_SIZE							(1 << CI_BALLOC_MIN_SHIFT)
#define CI_BALLOC_MAX_SIZE							(1 << CI_BALLOC_MAX_SHIFT)
ci_static_assert(CI_BALLOC_MAX_SIZE >= ci_sizeof(ci_list_t));	/* we use the buf memory to chain */

ci_bmp_def(ci_balloc_bucket_map, CI_BALLOC_NR_BUCKET);
#define ci_balloc_bucket_map_each_set(bmp, idx)		ci_bmp_each_set(bmp, CI_BALLOC_NR_BUCKET, idx)
#define ci_balloc_nr_pending(ba)					((ba)->nr_alloc - (ba)->nr_alloc_fail - (ba)->nr_free)

#ifdef CI_BALLOC_DEBUG
#define ci_ba_dbg_exec(...)							ci_dbg_exec(__VA_ARGS__)
#else
#define ci_ba_dbg_exec(...)				
#endif

/* input buffer size, output map size + buffer size */
#define ci_balloc_mem_eval(size)					((size) + (size) / CI_BALLOC_MIN_SIZE + PAL_CPU_CACHE_LINE_SIZE)


/* balloc: buddy system memory allocator */
typedef struct
{
	const char				*name;					/* a brief description */
	int						 flag;
#define CI_BALLOC_MT					0x0001		/* multi-thread support, thread safe, default on */	
#define CI_BALLOC_LAZY_CONQUER			0x0002		/* do not merge immediately, better performance */
	int						 node_id;				/* node id, -1 means not available */
	
	ci_mem_range_t			 map_range;				/* the range of map */
	ci_mem_range_t			 mem_range;				/* memory range for the buddy system */

	u64						 nr_alloc;				/* number of total allocs */
	u64						 nr_alloc_fail;			/* number of total failure of alloc */
	u64						 nr_free;				/* number of total frees */
	u64						 nr_conquer;			/* number of "defrag" */
	u64						 size_alloc;			/* now: total memory allocated */
	u64						 max_size_alloc;		/* maximum size_alloc */	

	/* when CI_BALLOC_LAZY_CONQUER is set */
	u64						 nr_cache_alloc;		/* this group doesn't included in nr_alloc/nr_free ... */
	u64						 nr_cache_alloc_fail;
	u64						 nr_cache_free;
	u64						 nr_obj_in_cache;		/* how many objects in cache[] */
	u64						 size_in_cache;			/* total size in cache */
	u64						 max_size_in_cache;
	
	
	ci_slk_t				 lock;					/* to make it mt safe */
	ci_balloc_bucket_map_t	 bucket_map;
	ci_list_t				 cache[CI_BALLOC_NR_BUCKET];	/* performance: when LAZY is set */
	ci_list_t				 bucket[CI_BALLOC_NR_BUCKET];
} ci_balloc_t;


#ifdef CI_BALLOC_DEBUG
#define ci_balloc(ba, size)		\
	({	\
		int __ba_size__;	\
		u8 *__ba_ptr__;	\
		\
		ci_assert((ba) && ((size) > 0));	\
		__ba_size__ = 1 << ci_log2_ceil((size) + CI_MEM_GUARD_SIZE);		\
		__ba_ptr__ = __ci_balloc(ba, __ba_size__);	\
		\
		if (__ba_ptr__) {	\
			ci_mem_guard_extl_set(__ba_ptr__, __ba_ptr__ + __ba_size__);		\
			__ba_ptr__ += ci_sizeof(ci_mem_guard_head_t);	\
		}	\
		\
		ci_dbg_exec(__ba_ptr__ && ci_memset(__ba_ptr__, CI_BUF_PTN_INIT, size));		\
		(void *)__ba_ptr__; 	\
	})
#define	ci_bfree(ba, ptr)		\
	do {	\
		int __ba_size__;	\
		u8 *__ba_ptr__ = (u8 *)(ptr);	\
		\
		ci_assert((ba) && (ptr));	\
		__ba_ptr__ -= ci_sizeof(ci_mem_guard_head_t);	\
		__ba_size__ = __ci_bfree_size(ba, __ba_ptr__);	\
		ci_mem_guard_extl_check(__ba_ptr__, __ba_ptr__ + __ba_size__);	\
		\
		__ci_bfree(ba, __ba_ptr__);	\
		ci_dbg_nullify(__ba_ptr__);	\
	} while (0)
#else
#define ci_balloc(ba, size)			__ci_balloc(ba, size)
#define ci_bfree(ba, ptr)			__ci_bfree(ba, ptr)
#endif


int  ci_balloc_init(ci_balloc_t *ba, const char *name, u8 *start, u8 *end);
void *__ci_balloc(ci_balloc_t *ba, int size);
void __ci_bfree(ci_balloc_t *ba, void *ptr);
int  __ci_bfree_size(ci_balloc_t *ba, u8 *ptr);		/* debug purpose, return the size from pointer */
void ci_balloc_free_cache(ci_balloc_t *ba);		/* free all cached memory */
int  ci_balloc_dump(ci_balloc_t *ba);
int  ci_balloc_dump_brief(ci_balloc_t *ba);
int  ci_balloc_dump_pending(ci_balloc_t *ba);


