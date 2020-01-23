/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_bitops.h				Bit operations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

/*
 *	u32 c implementation
 */
#define u32_mask_c(from, to)							__type_mask_c(32, from, to)

#define u32_set_bit_c(val, bit)							__type_set_bit_c(32, val, bit)
#define u32_bit_is_set_c(val, bit)						__type_bit_is_set_c(32, val, bit)
#define u32_first_set_c(val)							__type_next_c(32, set, val, 0)
#define u32_last_set_c(val)								__type_prev_c(32, set, val, (32 - 1))
#define u32_next_set_c(val, from)						__type_next_c(32, set, val, from)
#define u32_prev_set_c(val, from)						__type_prev_c(32, set, val, from)
#define u32_count_set_c(val)							__type_count_c(32, set, val)
#define u32_set_range_c(val, from, to)					__type_set_range_c(32, val, from, to)
#define u32_get_range_set_c(val, from, start, end)		__type_get_range_c(32, set, val, from, start, end)

#define u32_clear_bit_c(val, bit)						__type_clear_bit_c(32, val, bit)
#define u32_bit_is_clear_c(val, bit)					__type_bit_is_clear_c(32, val, bit)
#define u32_first_clear_c(val)							__type_next_c(32, clear, val, 0)
#define u32_last_clear_c(val)							__type_prev_c(32, clear, val, (32 - 1))
#define u32_next_clear_c(val, from)						__type_next_c(32, clear, val, from)
#define u32_prev_clear_c(val, from)						__type_prev_c(32, clear, val, from)
#define u32_count_clear_c(val)							__type_count_c(32, clear, val)
#define u32_clear_range_c(val, from, to)				__type_clear_range_c(32, val, from, to)
#define u32_get_range_clear_c(val, from, start, end)	__type_get_range_c(32, clear, val, from, start, end)


/*
 *	u64 c implementation
 */
#define u64_mask_c(from, to)							__type_mask_c(64, from, to)

#define u64_set_bit_c(val, bit)							__type_set_bit_c(64, val, bit)
#define u64_bit_is_set_c(val, bit)						__type_bit_is_set_c(64, val, bit)
#define u64_first_set_c(val)							__type_next_c(64, set, val, 0)
#define u64_last_set_c(val)								__type_prev_c(64, set, val, (64 - 1))
#define u64_next_set_c(val, from)						__type_next_c(64, set, val, from)
#define u64_prev_set_c(val, from)						__type_prev_c(64, set, val, from)
#define u64_count_set_c(val)							__type_count_c(64, set, val)
#define u64_set_range_c(val, from, to)					__type_set_range_c(64, val, from, to)
#define u64_get_range_set_c(val, from, start, end)		__type_get_range_c(64, set, val, from, start, end)

#define u64_clear_bit_c(val, bit)						__type_clear_bit_c(64, val, bit)
#define u64_bit_is_clear_c(val, bit)					__type_bit_is_clear_c(64, val, bit)
#define u64_first_clear_c(val)							__type_next_c(64, clear, val, 0)
#define u64_last_clear_c(val)							__type_prev_c(64, clear, val, (64 - 1))
#define u64_next_clear_c(val, from)						__type_next_c(64, clear, val, from)
#define u64_prev_clear_c(val, from)						__type_prev_c(64, clear, val, from)
#define u64_count_clear_c(val)							__type_count_c(64, clear, val)
#define u64_clear_range_c(val, from, to)				__type_clear_range_c(64, val, from, to)
#define u64_get_range_clear_c(val, from, start, end)	__type_get_range_c(64, clear, val, from, start, end)


/*
 *	template for clear/set, u32 or u64
 */
#define __type_set_bit_c(width, val, bit)				((val) |= ((u ## width)1 << (bit)))
#define __type_bit_is_set_c(width, val, bit)			(!!((val) & ((u ## width)1 << (bit))))
#define __type_clear_bit_c(width, val, bit)				((val) &= ~((u ## width)1 << (bit)))
#define __type_bit_is_clear_c(width, val, bit)			(!__type_bit_is_set_c(width, val, bit))
#define __type_set_range_c(width, val, from, to)		((val) |= u ## width ## _mask_c(from, to))
#define __type_clear_range_c(width, val, from, to)		((val) &= ~u ## width ## _mask_c(from, to))

#define __type_next_c(width, set_clear, val, from)		\
	({		\
		u ## width __val__ = val;		\
		int __from__ = from, __check__ = 1;		\
		\
		ci_assert(__from__ >= 0);		\
		while (__check__ && (__from__ < width))		\
			if ((__check__ = !u ## width ## _bit_is_ ## set_clear ## _c(__val__, __from__)))	\
				__from__++;		\
		__check__ ? -1 : __from__;		\
	})

#define __type_prev_c(width, set_clear, val, from)		\
	({		\
		u ## width __val__ = val;		\
		int __from__ = from, __check__ = 1;		\
		\
		ci_assert(__from__ < width);		\
		while (__check__ && (__from__ >= 0)) 	\
			if ((__check__ = !u ## width ## _bit_is_ ## set_clear ## _c(__val__, __from__)))	\
				__from__--;		\
		__check__ ? -1 : __from__;		\
	})

#define __type_count_c(width, set_clear, val)		\
	({	\
		int __cnt__ = 0;		\
		u ## width __val__ = val;		\
		\
		ci_loop(__idx__, width)		\
			if (u ## width ## _bit_is_ ## set_clear ## _c(__val__, __idx__))	\
				__cnt__++;		\
		\
		__cnt__;		\
	})

#define __type_mask_c(width, from, to)		\
	({		\
		int __from__ = (from), __to__ = (to);		\
		\
		ci_range_check(__from__, 0, width);		\
		ci_range_check_i(__to__, 1, width);		\
		ci_assert(__from__ < __to__);		\
		\
		(CI_U ## width ## _MAX << __from__) & (CI_U ## width ## _MAX >> (width - __to__));		\
	})

/* output: start, end.  return < 0 if not found */
#define ci_m_cmp_set(x)			x
#define ci_m_cmp_clear(x)		x
#define __type_get_range_c(width, set_clear, val, from, start, end)		\
	({	\
		int __start__, __end__;		\
		__start__ = __type_next_c(width, set_clear, val, from);		\
		ci_m_if_else(ci_m_equal(set_clear, set))	\
		( __end__ = __type_next_c(width, clear, val, __start__); )			\
		( __end__ = __type_next_c(width, set, val, __start__); )			\
		ci_unlikely(__end__ < 0) && (__end__ = width);	\
		end = __end__;	\
		start = __start__;	\
	})

