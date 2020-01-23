/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_bitops.h			PAL bit operations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal_cfg.h"
#include "ci_const.h"


/*
 *	32 bit operations
 */
#define u32_is_all_clear(val)							((val) == 0)
#define u32_mask(from, to)								u32_mask_c(from, to)

#define u32_bit_is_set(val, bit)						u32_bit_is_set_c(val, bit)
#define u32_set_bit(val, bit)							u32_set_bit_c(val, bit)					
#define u32_first_set(val)								u32_first_set_c(val)				
#define u32_next_set(val, from)							u32_next_set_c(val, from)					
#define u32_last_set(val)								u32_last_set_c(val)					
#define u32_prev_set(val, from)							u32_prev_set_c(val, from)				
#define u32_count_set(val)								u32_count_set_c(val)
#define u32_set_range(val, from, to)					u32_set_range_c(val, from, to)
#define u32_get_range_set(val, from, start, end)		u32_get_range_set_c(val, from, start, end)

#define u32_bit_is_clear(val, bit)						u32_bit_is_clear_c(val, bit)
#define u32_clear_bit(val, bit)							u32_clear_bit_c(val, bit)					
#define u32_first_clear(val)							u32_first_clear_c(val)				
#define u32_next_clear(val, from)						u32_next_clear_c(val, from)					
#define u32_last_clear(val)								u32_last_clear_c(val)					
#define u32_prev_clear(val, from)						u32_prev_clear_c(val, from)				
#define u32_count_clear(val)							u32_count_clear_c(val)
#define u32_clear_range(val, from, to)					u32_clear_range_c(val, from, to)
#define u32_get_range_clear(val, from, start, end)		u32_get_range_clear_c(val, from, start, end)

#define u32_each_set(val, idx)		\
	for (int idx = 0; (idx = u32_next_set(val, idx)) >= 0; idx++)

/*
 *	64 bit operations
 */
#define u64_is_all_clear(val)							((val) == 0ull)
	
#ifdef PAL_BITOPS_C
#define u64_mask(from, to)								u64_mask_c(from, to)

#define u64_bit_is_set(val, bit)						u64_bit_is_set_c(val, bit)
#define u64_set_bit(val, bit)							u64_set_bit_c(val, bit)					
#define u64_first_set(val)								u64_first_set_c(val)				
#define u64_next_set(val, from)							u64_next_set_c(val, from)					
#define u64_last_set(val)								u64_last_set_c(val)					
#define u64_prev_set(val, from)							u64_prev_set_c(val, from)				
#define u64_count_set(val)								u64_count_set_c(val)
#define u64_set_range(val, from, to)					u64_set_range_c(val, from, to)
#define u64_get_range_set(val, from, start, end)		u64_get_range_set_c(val, from, start, end)

#define u64_bit_is_clear(val, bit)						u64_bit_is_clear_c(val, bit)
#define u64_clear_bit(val, bit)							u64_clear_bit_c(val, bit)					
#define u64_first_clear(val)							u64_first_clear_c(val)				
#define u64_next_clear(val, from)						u64_next_clear_c(val, from)					
#define u64_last_clear(val)								u64_last_clear_c(val)					
#define u64_prev_clear(val, from)						u64_prev_clear_c(val, from)				
#define u64_count_clear(val)							u64_count_clear_c(val)
#define u64_clear_range(val, from, to)					u64_clear_range_c(val, from, to)
#define u64_get_range_clear(val, from, start, end)		u64_get_range_clear_c(val, from, start, end)
#else
#define u64_mask(from, to)								u64_mask_c(from, to)

#define u64_bit_is_set(val, bit)						u64_bit_is_set_c(val, bit)
#define u64_set_bit(val, bit)							u64_set_bit_c(val, bit)					
#define u64_first_set(val)				\
	({	\
		u64 __first_set_val__ = val;		\
		int __first_set_rv__ = __builtin_ffsll(__first_set_val__) - 1;		\
		ci_assert(__first_set_rv__ == u64_first_set_c(__first_set_val__));	\
		__first_set_rv__;		\
	})
#define u64_next_set(val, from)			\
	({	\
		u64 __next_set_val__ = val;		\
		int __bit_from__ = from;		\
		int __next_set_rv__ = (__bit_from__) < 64 ? u64_first_set((CI_U64_MAX << (__bit_from__)) & (__next_set_val__)) : -1;		\
		ci_assert(((__bit_from__) >= 0) && (__next_set_rv__ == u64_next_set_c(__next_set_val__, __bit_from__)));		\
		__next_set_rv__;	\
	})
#define u64_last_set(val)		\
	({	\
		u64 __last_set_val__ = val;		\
		int __last_set_rv__ = (__last_set_val__) ? 64 - __builtin_clzll(__last_set_val__) - 1 : -1;		\
		ci_assert(__last_set_rv__ == u64_last_set_c(__last_set_val__));		\
		__last_set_rv__;	\
	})
#define u64_prev_set(val, from)							u64_prev_set_c(val, from)				
#define u64_count_set(val)								u64_count_set_c(val)
#define u64_set_range(val, from, to)					u64_set_range_c(val, from, to)
#define u64_get_range_set(val, from, start, end)		u64_get_range_set_c(val, from, start, end)

#define u64_bit_is_clear(val, bit)						u64_bit_is_clear_c(val, bit)
#define u64_clear_bit(val, bit)							u64_clear_bit_c(val, bit)					
#define u64_first_clear(val)							u64_first_clear_c(val)				
#define u64_next_clear(val, from)						u64_next_clear_c(val, from)					
#define u64_last_clear(val)								u64_last_clear_c(val)					
#define u64_prev_clear(val, from)						u64_prev_clear_c(val, from)				
#define u64_count_clear(val)							u64_count_clear_c(val)
#define u64_clear_range(val, from, to)					u64_clear_range_c(val, from, to)
#define u64_get_range_clear(val, from, start, end)		u64_get_range_clear_c(val, from, start, end)
#endif

#define u64_each_set(val, idx)		\
	for (int idx = 0; (idx = u64_next_set(val, idx)) >= 0; idx++)


