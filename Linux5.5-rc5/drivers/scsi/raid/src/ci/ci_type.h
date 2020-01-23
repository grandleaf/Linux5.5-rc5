/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_type.h					CI type defines
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#define CI_EOT								{}		/* end of table or empty table */

#define CI_PR_RANGE_FMT						"[%p, %p)"
#define ci_pr_range_val(x)					(x)->start, (x)->end

#define CI_PR_RANGE_EX_FMT					CI_PR_RANGE_FMT "->%p"
#define ci_pr_range_ex_val(x)				(x)->start, (x)->end, (x)->curr

#define CI_PR_RANGE_BNP_FMT					CI_PR_RANGE_FMT ", " CI_PR_BNP_FMT
#define ci_pr_range_bnp_val(x)				ci_pr_range_val(x), __ci_pr_bnp_val(ci_range_to_bnp(x))

#define CI_PR_RANGE_EX_BNP_FMT				CI_PR_RANGE_EX_FMT ", " CI_PR_BNP_FMT
#define ci_pr_range_ex_bnp_val(x)			ci_pr_range_ex_val(x), __ci_pr_bnp_val(ci_range_to_bnp(x))

#define CI_PR_ACCESS_TAG_FMT				"%#llX, %s, %s(), %d"
#define ci_pr_access_tag_val(x)				(x)->timestamp, ci_str_file_base_name((x)->file), (x)->func, (x)->line

#define ci_range_to_bnp(x)					ci_make_bnp((x)->end - (x)->start)
#define ci_pr_pct_range_ex_used(r)			ci_pr_pct_val((r)->curr - (r)->start, (r)->end - (r)->start)
#define ci_pr_pct_range_ex_avail(r)			ci_pr_pct_val((r)->end - (r)->curr, (r)->end - (r)->start)


typedef struct __ci_mod_t ci_mod_t;
typedef struct __ci_node_t ci_node_t;

typedef struct {
	u8							*start;				/* [start, end) */
	u8							*end;
} ci_mem_range_t; 

typedef struct {
	u8							*start;				/* [start, end) */
	u8							*end;
	u8							*curr;				/* current allocation ptr */
} ci_mem_range_ex_t; 

#define ci_mem_range_len(x)				\
	({	\
		ci_assert((x)->end > (x)->start);	\
		(x)->end - (x)->start;	\
	})
#define ci_mem_range_ex_def(x, s, e)		ci_mem_range_ex_t x = { .start = (u8 *)(s), .end = (u8 *)(e), .curr = (u8 *)(s) }
#define ci_mem_range_ex_init(x, s, e)		\
	({	\
		(x)->start = (x)->curr = (u8 *)(s), (x)->end = (u8 *)(e);	\
		(x);	\
	})
#define ci_mem_range_ex_reinit(x)		\
	({	\
		(x)->curr = (x)->start;		\
		(x);	\
	})
#define ci_mem_range_each(ptr, range)		\
	for (ptr = (ci_typeof(ptr))(range)->start; (u8 *)(ptr + 1) <= (range)->end; ptr++)
#define ci_mem_range_check(ptr, range, ...)		\
	ci_range_check((u8 *)(ptr), (range)->start, (range)->end, __VA_ARGS__)
		
		

typedef struct {
    int                  		 code;          	/* an integer for code/flag */
    const char          		*name;          	/* description for the code */
} ci_int_to_name_t;
#define ci_int_name(x)							{ x, #x }




/*
 *	for detecting memory corruption & monitor memory usage
 */

/* for memory overwritten checking */
typedef struct {
	void						*addr;
} ci_mem_anchor_t;

/* access tracking */
typedef struct {
	const char					*file;
	const char					*func;
	int					 		 line;
	u64							 timestamp;
	void						*cookie;			/* user set this one */
} ci_access_tag_t;

/* memory corruption detection */
typedef struct {
#ifdef CI_MEM_GUARD_CACHE_LINE
	u8 							 __padding[PAL_CPU_CACHE_LINE_SIZE - ci_sizeof(ci_access_tag_t) - ci_sizeof(ci_mem_anchor_t)];
#endif
	ci_access_tag_t				 __access_tag;
	ci_mem_anchor_t				 __mem_anchor;
} ci_mem_guard_head_t;

#ifdef CI_MEM_GUARD_CACHE_LINE
ci_type_size_check(ci_mem_guard_head_t, PAL_CPU_CACHE_LINE_SIZE);
#endif

typedef struct {
	ci_mem_anchor_t				 __mem_anchor;
} ci_mem_guard_tail_t;


#ifdef CI_DEBUG
/* for access tag */
#define __ci_access_tag_def(name)			ci_access_tag_t name
#define __ci_access_tag_set(name)	\
	do {	\
		(name)->file = __FILE__;	\
		(name)->func = __FUNCTION__;	\
		(name)->line = __LINE__;	\
		(name)->timestamp = pal_timestamp(); 	\
	} while (0)

/* for anchor */
#define __ci_mem_anchor_def(name)			ci_mem_anchor_t name
#define __ci_mem_anchor_set(name)			(name)->addr = (name)
#define __ci_mem_anchor_valid(name)			((name)->addr == (name))
#define __ci_mem_anchor_check(name)			ci_assert(__ci_mem_anchor_valid(name), "memory corruption detected @%p", (name)->addr)
#else
#define __ci_access_tag_def(name)
#define __ci_access_tag_set(name)			ci_nop()

#define __ci_mem_anchor_def(name)	
#define __ci_mem_anchor_set(name)			ci_nop()
#define __ci_mem_anchor_valid(name)			1
#define __ci_mem_anchor_check(name)			ci_nop()
#endif

#define ci_access_tag						__ci_access_tag_def(__access_tag)
#define ci_access_tag_set(obj)				__ci_access_tag_set(&(obj)->__access_tag)

#define ci_mem_anchor						__ci_mem_anchor_def(__mem_anchor)
#define ci_mem_anchor_set(obj)				__ci_mem_anchor_set(&(obj)->__mem_anchor)
#define ci_mem_anchor_check(obj)			__ci_mem_anchor_check(&(obj)->__mem_anchor)
#define ci_mem_anchor_valid(obj)			__ci_mem_anchor_valid(&(obj)->__mem_anchor)

#define ci_mem_guard_head_set(obj)		\
	do {	\
		ci_access_tag_set((ci_mem_guard_head_t *)(obj));		\
		ci_mem_anchor_set((ci_mem_guard_head_t *)(obj));		\
	} while (0)
#define ci_mem_guard_head_check(obj)		ci_mem_anchor_check((ci_mem_guard_head_t *)(obj))
#define ci_mem_guard_tail_set(obj)			ci_mem_anchor_set((ci_mem_guard_tail_t *)(obj))
#define ci_mem_guard_tail_check(obj)		ci_mem_anchor_check((ci_mem_guard_tail_t *)(obj))

#define CI_MEM_GUARD_SIZE					(ci_sizeof(ci_mem_guard_head_t) + ci_sizeof(ci_mem_guard_tail_t))

/* head_guard | start = user_start | ...... | end = user_end | tail_guard */
#define ci_mem_guard_intl_set(start, end) 	\
	do {	\
		ci_mem_guard_head_set((ci_mem_guard_head_t *)(start) - 1);	\
		ci_mem_guard_tail_set((ci_mem_guard_tail_t *)(end));	\
	} while (0)
#define ci_mem_guard_intl_check(start, end)	\
	do {	\
		ci_mem_guard_head_check((ci_mem_guard_head_t *)(start) - 1);	\
		ci_mem_guard_tail_check((ci_mem_guard_tail_t *)(end));	\
	} while (0)

/* start | head_guard | user_start | ...... | user_end | tail_guard | end */
#define ci_mem_guard_extl_set(start, end) 	\
	do {	\
		ci_mem_guard_head_set((ci_mem_guard_head_t *)(start));	\
		ci_mem_guard_tail_set((ci_mem_guard_tail_t *)(end) - 1);	\
	} while (0)
#define ci_mem_guard_extl_check(start, end)	\
	do {	\
		ci_mem_guard_head_check((ci_mem_guard_head_t *)(start));	\
		ci_mem_guard_tail_check((ci_mem_guard_tail_t *)(end) - 1);	\
	} while (0)



