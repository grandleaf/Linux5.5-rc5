/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_util.h				CI macros
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#define ptrint_t									int)(ptrdiff_t

#define ci_swap_type(a, b, t)						do { t __t__ = (a); (a) = (b); (b) = __t__; } while (0)
#define ci_swap(a, b)								do { ci_typeof(a) __t__ = (a);  (a) = (b); (b) = __t__; } while (0)
#define ci_is_power_of_two(n)						(!((n) & ((n) - 1)))						/* !(n) || (...) if consider 0 */
#define ci_nr_elm(x)								(ci_sizeof(x) / ci_sizeof((x)[0]))			/* number of elements in a given array */
#define ci_div_ceil(n, d)							(((n) + (d) - 1) / (d))
#define ci_obj_zero(p)								ci_memzero((p), ci_sizeof(*(p)))
#define ci_obj_copy(d, s)							ci_memcpy((d), (s), ci_sizeof(*(d)))
#define ci_strsize(x)								(ci_strlen(x) + 1)
#define ci_va_args_mmaker(prefix, ...)				ci_token_concat(prefix, ci_m_argc(__VA_ARGS__))(__VA_ARGS__)
#define ci_str_ary(...)								(const char *[]){ __VA_ARGS__, NULL }
#define ci_int_ary(...)								(int []){ __VA_ARGS__, CI_INT_MAX }


#define ci_va_list_to_str_ary(count, args, name)		\
	do {	\
		ci_loop(__va_list_i__, count)		\
			name[__va_list_i__] = va_arg(args, const char *);	\
	} while (0)

#define ci_ary_each(ary, var)		\
	for (ci_typeof(&(ary)[0]) var = (ary); var < &(ary)[ci_nr_elm(ary)]; var++)
#define ci_ary_each_with_index(ary, var, index, ...)	\
	do {	\
		int index = 0;	\
		for (ci_typeof(&(ary)[0]) var = (ary); var < &(ary)[ci_nr_elm(ary)]; var++, index++) {	\
			__VA_ARGS__;	\
		}	\
	} while (0)
	

#define ci_abs(a)									((a) > 0 ? (a) : -(a))
#define ci_max2(a, b)		 \
	({		\
		ci_typeof(a) __a__ = (a);	\
		ci_typeof(b) __b__ = (b);	\
		__a__ > __b__ ? __a__ : __b__;		\
	})
#define ci_min2(a, b)		 \
	({		\
		ci_typeof(a) __a__ = (a);	\
		ci_typeof(b) __b__ = (b);	\
		__a__ < __b__ ? __a__ : __b__;		\
	})
#define ci_max3(a, b, c)							ci_max2(ci_max2(a, b), c)
#define ci_min3(a, b, c)							ci_min2(ci_min2(a, b), c)
#define ci_max(...)									ci_va_args_mmaker(ci_max, __VA_ARGS__)
#define ci_min(...)									ci_va_args_mmaker(ci_min, __VA_ARGS__)
#define ci_max_set(dst, ...)						((dst) = ci_max((dst), __VA_ARGS__))
#define ci_min_set(dst, ...)						((dst) = ci_min((dst), __VA_ARGS__))

#define __ci_m_to_str(x)							#x
#define ci_m_to_str(x)								__ci_m_to_str(x)

#define ci_log2(n)									__ci_log2_32(n)
#define ci_log2_ceil(n)								(ci_log2(n) + 1 - ci_is_power_of_two(n))
#define __ci_log2_32(n)								((n) & 0xFFFF0000	? 16	+ __ci_log2_16((n)	>> 16)	: __ci_log2_16(n))
#define __ci_log2_16(n)								((n) & 0xFF00		? 8		+ __ci_log2_8((n)	>> 8)	: __ci_log2_8(n))
#define __ci_log2_8(n)								((n) & 0xF0			? 4		+ __ci_log2_4((n)	>> 4)	: __ci_log2_4(n))
#define __ci_log2_4(n)								((n) & 0xC			? 2		+ __ci_log2_2((n)	>> 2)	: __ci_log2_2(n))
#define __ci_log2_2(n)								((n) & 0x2			? 1		: 0)

/* how many digits in a base 10 unsigned integer */
#define ci_nr_digit(x)		\
    ((x) >= 10000000000ull /* [11-20] [1-10] */	\
    ?	\
        ((x) >= 1000000000000000ull /* [16-20] [11-15] */	\
        ?   /* [16-20] */	\
            ((x) >= 100000000000000000ull /* [18-20] [16-17] */	\
            ?   /* [18-20] */	\
                ((x) >= 1000000000000000000ull /* [19-20] [18] */	\
                ? /* [19-20] */	\
                    ((x) >=  10000000000000000000ull /* [20] [19] */	\
                    ?   20	\
                    :   19	\
                    )	\
                : 18	\
                )	\
            :   /* [16-17] */	\
                ((x) >=  10000000000000000ull /* [17] [16] */	\
                ?   17	\
                :   16	\
                )	\
            )	\
        :   /* [11-15] */	\
            ((x) >= 1000000000000ull /* [13-15] [11-12] */	\
            ?   /* [13-15] */	\
                ((x) >= 10000000000000ull /* [14-15] [13] */	\
                ? /* [14-15] */	\
                    ((x) >=  100000000000000ull /* [15] [14] */	\
                    ?   15	\
                    :   14	\
                    )	\
                : 13	\
                )	\
            :   /* [11-12] */	\
                ((x) >=  100000000000ull /* [12] [11] */	\
                ?   12	\
                :   11	\
                )	\
            )	\
        )	\
    :   /* [1-10] */	\
        ((x) >= 100000ull /* [6-10] [1-5] */	\
        ?   /* [6-10] */	\
            ((x) >= 10000000ull /* [8-10] [6-7] */	\
            ?   /* [8-10] */	\
                ((x) >= 100000000ull /* [9-10] [8] */	\
                ? /* [9-10] */	\
                    ((x) >=  1000000000ull /* [10] [9] */	\
                    ?   10	\
                    :    9	\
                    )	\
                : 8	\
                )	\
            :   /* [6-7] */	\
                ((x) >=  1000000ull /* [7] [6] */	\
                ?   7	\
                :   6	\
                )	\
            )	\
        :   /* [1-5] */	\
            ((x) >= 100ull /* [3-5] [1-2] */	\
            ?   /* [3-5] */	\
                ((x) >= 1000ull /* [4-5] [3] */	\
                ? /* [4-5] */	\
                    ((x) >=  10000ull /* [5] [4] */	\
                    ?   5	\
                    :   4	\
                    )	\
                : 3	\
                )	\
            :   /* [1-2] */	\
                ((x) >=  10ull /* [2] [1] */	\
                ?   2	\
                :   1	\
                )	\
            )	\
        )	\
    )	


/*
 * shared random, with spinlock, might be slow because of conflict
 */
#define __ci_rand_shr_2(l, u)						(ci_assert((l) < (u)), pal_rand_shr(l, u))
#define __ci_rand_shr_1(u)							__ci_rand_shr_2(0, u)
#define ci_rand_shr(...)							ci_va_args_mmaker(__ci_rand_shr_, __VA_ARGS__)

#define __ci_rand_shr_i_2(l, u)						(ci_assert((l) < (u) + 1), pal_rand_shr(l, (u) + 1))
#define __ci_rand_shr_i_1(u)						__ci_rand_shr_i_2(0, u)
#define ci_rand_shr_i(...)							ci_va_args_mmaker(__ci_rand_shr_i_, __VA_ARGS__)


/*
 * random with context, faster (without spinlock)
 */
#define __ci_rand_3(ctx, l, u)						(ci_assert((l) < (u)), pal_rand(ctx, l, u))
#define __ci_rand_2(ctx, u)							__ci_rand_3(ctx, 0, u)
#define ci_rand(ctx, ...)							ci_va_args_mmaker(__ci_rand_, ctx, __VA_ARGS__)

#define __ci_rand_i_3(ctx, l, u)					(ci_assert((l) < (u) + 1), pal_rand(ctx, l, (u) + 1))
#define __ci_rand_i_2(ctx, u)						__ci_rand_i_3(ctx, 0, u)
#define ci_rand_i(ctx, ...)							ci_va_args_mmaker(__ci_rand_i_, ctx, __VA_ARGS__)



/* for alignments, a must be power of two */
#define ci_align_upper(x, a)						__ci_align_upper_mask(x, (ci_typeof(x))(a) - 1)
#define ci_align_upper_asg(x, a)					((x) = ci_align_upper(x, a))
#define __ci_align_upper_mask(x, m)					(((x) + (m)) & ~(m))
#define ci_align_lower(x, a)						((x) & ~((ci_typeof(x))(a) - 1))
#define ci_is_aligned(x, a)							(!((x) & ((ci_typeof(x))(a) - 1)))
#define ci_align_cpu(x)								ci_align_upper(x, PAL_CPU_ALIGN_SIZE)
#define ci_align_cpu_asg(x)							ci_align_upper_asg(x, PAL_CPU_ALIGN_SIZE)

#define ci_ptr_align_upper(x, a)					((ci_typeof(x))ci_align_upper((uintptr_t)(x), a))
#define ci_ptr_align_upper_asg(x, a)				((x) = ci_ptr_align_upper(x, a))
#define ci_ptr_align_lower(x, a)					((ci_typeof(x))ci_align_lower((uintptr_t)(x), a))
#define ci_ptr_align_lower_asg(x, a)				((x) = ci_ptr_align_lower(x, a))

#define ci_ptr_align_cpu(x)							((ci_typeof(x))ci_ptr_align_upper(x, PAL_CPU_ALIGN_SIZE))
#define ci_ptr_align_cpu_asg(x)						ci_ptr_align_upper_asg(x, PAL_CPU_ALIGN_SIZE)
#define ci_ptr_align_cache(x)						((ci_typeof(x))ci_ptr_align_upper(x, PAL_CPU_CACHE_LINE_SIZE))
#define ci_ptr_align_cache_asg(x)					ci_ptr_align_upper_asg(x, PAL_CPU_CACHE_LINE_SIZE)

#define ci_ptr_is_aligned(x, a)						ci_is_aligned((uintptr_t)(x), a)
#define ci_ptr_align_check(x, a)					ci_assert(ci_ptr_is_aligned(x, a))


#define ci_offset_of(type, member)					((ptrint_t)&((type *)0)->member)
#define ci_container_of(ptr, type, member)			((type *)((u8 *)(ptr) - ci_offset_of(type, member)))
#define ci_container_of_safe(ptr, type, member)		({ void *__ptr__ = (ptr); __ptr__ ? ci_container_of(__ptr__, type, member) : NULL; })
#define ci_member_size(type, member)				ci_sizeof(((type *)0)->member)


#define ci_token_concat(...)						__ci_token_concat(__ci_token_concat, ci_m_argc(__VA_ARGS__))(__VA_ARGS__)
#define __ci_token_concat(a, b)						ci_m_concat(a, b)
#define __ci_token_concat2(a, b)					__ci_token_concat(a, b)
#define __ci_token_concat3(a, b, c)					__ci_token_concat(__ci_token_concat(a, b), c)
#define __ci_token_concat4(a, b, c, d)				__ci_token_concat(__ci_token_concat3(a, b, c), d)
#define __ci_token_concat5(a, b, c, d, e)			__ci_token_concat(__ci_token_concat4(a, b, c, d), e)
#define ci_rsvd										ci_token_concat(__ci_rsvd, __COUNTER__)		/* use this to define a reserved field in a structure */


/* range */
#define ci_in_range(x, l, u)						(((x) >= (l)) && ((x) < (u)))
#define ci_in_range_i(x, l, u)						ci_in_range(x, l, (u) + 1)

/* checking macros */
#define ci_assert(x, ...)							pal_assert(x, __VA_ARGS__)
#define ci_bug(...)									ci_assert(0, __VA_ARGS__)
#define ci_panic(...)								pal_panic(__VA_ARGS__)
#define ci_panic_if(x, ...)							pal_panic_if(x, __VA_ARGS__)
#define ci_panic_unless(x, ...)						ci_panic_if(!(x), __VA_ARGS__)
#define ci_align_check(x, a, ...)					ci_assert(ci_is_aligned((u64)(x), a), __VA_ARGS__)
#define ci_range_check(x, l, u, ...)				ci_assert(((x) >= (l)) && ((x) < (u)), __VA_ARGS__)
#define ci_range_check_i(x, l, u, ...)				ci_assert(((x) >= (l)) && ((x) <= (u)), __VA_ARGS__)	/* includes upper boundary */
#define ci_print_exp_loc()							ci_printf("Exception at:\n    file : %s\n    func : %s()\n    line : %d\n", \
															   __FILE__, __FUNCTION__, __LINE__)
#define ci_flag_range_check(x, f)					ci_assert(((x) & ~(f)) == 0)
#define ci_flag_all_set(v, f)						(((v) & (f)) == (f))

#if 0
/* Too bad that C++ doesn't support designated initializer */
#define __MSVC_COMMA__				,
#define ci_obj_def(type, var, ...)					static ci_obj_local_def(type, var, __VA_ARGS__)
#define ci_obj_local_def(type, var, ...)		\
	type var = (ci_m_each_m1(__ci_obj_local_def_helper, var, __VA_ARGS__) __MSVC_COMMA__ var)
#define __ci_obj_local_def_helper(var, assign, ...)	var assign, 
#endif

#if 0
/* range operators: e.g. for (i = from; i < to; i++) */
#define __ci_loop_1(to)								for (int __times_count__ = 0; __times_count__ < (to); __times_count__++)
#define __ci_loop_2(var, to)						for (ci_typeof(to) var = 0; var < (to); var++)
#define __ci_loop_3(var, from, to)					for (ci_typeof(to) var = (from); var < (to); var++)
#define __ci_loop_4(var, from, to, step)			for (ci_typeof(to) var = (from); (step) > 0 ? (var < (to)) : (var > (to)); var += (step))
#define ci_loop(...)								ci_va_args_mmaker(__ci_loop_, __VA_ARGS__)
#define ci_dead_loop()								do { for (;;) ; } while (0)

/* range operators: e.g. for (i = from; i <= to; i++) */
#define __ci_loop_i_1(to)							for (int __times_count__ = 0; __times_count__ <= (to); __times_count__++)
#define __ci_loop_i_2(var, to)						for (ci_typeof(to) var = 0; var <= (to); var++)
#define __ci_loop_i_3(var, from, to)				for (ci_typeof(to) var = (from); var <= (to); var++)
#define __ci_loop_i_4(var, from, to, step)			for (ci_typeof(to) var = (from); (step) > 0 ? (var <= (to)) : (var >= (to)); var += (step))
#define ci_loop_i(...)								ci_va_args_mmaker(__ci_loop_i_, __VA_ARGS__)
#endif

/* range operators: e.g. for (i = from; i < to; i++) */
#define __ci_loop_1(to)		\
	for (ci_typeof(to) __loop_1_cnt__ = 0, __loop_1_to__ = (to); __loop_1_cnt__ < __loop_1_to__; __loop_1_cnt__++)
#define __ci_loop_2(var, to)	\
	for (ci_typeof(to) var = 0, __loop_2_to__ = (to); var < (__loop_2_to__); var++)
#define __ci_loop_3(var, from, to)	\
	for (ci_typeof(to) var = (from), __loop_3_to__ = (to); var < (__loop_3_to__); var++)
#define __ci_loop_4(var, from, to, step)	\
	for (ci_typeof(to) var = (from), __loop_4_to__ = (to), __loop_4_step__ = (step); 	\
		(__loop_4_step__) > 0 ? (var < (__loop_4_to__)) : (var > (__loop_4_to__)); 	\
		var += (__loop_4_step__))
#define ci_loop(...)								ci_va_args_mmaker(__ci_loop_, __VA_ARGS__)
#define ci_dead_loop()								do { for (;;) ; } while (0)

/* range operators: e.g. for (i = from; i <= to; i++) */
#define __ci_loop_i_1(to)	\
	for (ci_typeof(to) __loop_1_i_cnt__ = 0, __loop_1_i_to__ = (to); __loop_1_i_cnt__ <= __loop_1_i_to__; __loop_1_i_cnt__++)
#define __ci_loop_i_2(var, to)		\
	for (ci_typeof(to) var = 0, __loop_2_i_to__ = (to); var <= (__loop_2_i_to__); var++)
#define __ci_loop_i_3(var, from, to)	\
	for (ci_typeof(to) var = (from), __loop_3_i_to__ = (to); var <= (__loop_3_i_to__); var++)
#define __ci_loop_i_4(var, from, to, step)	\
	for (ci_typeof(to) var = (from), __loop_4_i_to__ = (to), __loop_4_i_step__ = (step); 	\
		(__loop_4_i_step__) > 0 ? (var <= (__loop_4_i_to__)) : (var >= (__loop_4_i_to__)); 	\
		var += (__loop_4_i_step__))
#define ci_loop_i(...)								ci_va_args_mmaker(__ci_loop_i_, __VA_ARGS__)

/* KiB, MiB, ... */
#define ci_kib(size)								((size) << 10ull)
#define ci_mib(size)								((size) << 20ull)
#define ci_gib(size)								((size) << 30ull)
#define ci_tib(size)								((size) << 40ull)

#define ci_to_kib(size)								((size) >> 10ull)
#define ci_to_mib(size)								((size) >> 20ull)
#define ci_to_gib(size)								((size) >> 30ull)
#define ci_to_tib(size)								((size) >> 40ull)


/*
 * 	string switch wrapper
 *  note: no fall-through, no multi entries
 *
 * 	ci_ssw_switch(str, {
 *		ci_ssw_case("str_a"):
 *			......
 *			ci_ssw_break;
 *		ci_ssw_case("str_b"):
 *			......
 *			ci_ssw_break;
 *		ci_ssw_break:
 *			......
 *			ci_ssw_break;
 *	});
 */
#define ci_ssw_switch(s, ...)		\
	do {	\
		char *__sw_str__ = (s); if (0)	\
		__VA_ARGS__		\
	} while (0)
#define ci_ssw_case(s)								} else if (ci_strequal(__sw_str__, (s))) { switch (0) { case 0
#define ci_ssw_break								; }
#define ci_ssw_default								} else { switch (0) { case 0


/* chores */
#define ci_here()									ci_printf("file:%s, func:%s(), line:%d\n", __FILE__, __FUNCTION__, __LINE__)
#define ci_nop(...)									({ NULL; })
#define ci_dbg_nullify(ptr)							ci_dbg_exec((ptr) = NULL)

#define __ci_exec_upto(n, x)				\
	do {	\
		static int __ci_exec_counter__;		\
		if (__ci_exec_counter__++ < (n))	\
			x;		\
	} while (0)
#define ci_exec_upto(n, ...)						__ci_exec_upto(n, ({__VA_ARGS__;}))
#define ci_exec_once(...)							ci_exec_upto(1, __VA_ARGS__)
#define ci_exec_no_err(...)			\
	({		\
		int __exec_rv__;		\
		__exec_rv__ = (__VA_ARGS__);	\
		ci_assert(__exec_rv__ >= 0);		\
		__exec_rv__;		\
	})
#define ci_exec_ptr_not_nil(x)	/* exec x, check return value as pointer *, make sure it is not null */	\
	({	\
		ci_typeof(x) __vp__ = (x);		\
		ci_assert(__vp__);		\
		__vp__;	\
	})	

#define ci_dump_pre(pre_str, dump_func)		\
	({		\
		int __ci_dump_pre_suf_rv__ = ci_printf(pre_str);	\
		__ci_dump_pre_suf_rv__ += dump_func;	\
	})
#define ci_dump_preln(pre_str, dump_func)		\
	({		\
		int __ci_dump_pre_suf_rv__ = ci_printf(pre_str);	\
		__ci_dump_pre_suf_rv__ += dump_func;	\
		__ci_dump_pre_suf_rv__ += ci_printfln();	\
	})
#define ci_dump_pre_suf(pre_str, dump_func, suf_str)		\
	({		\
		int __ci_dump_pre_suf_rv__ = ci_printf(pre_str);	\
		__ci_dump_pre_suf_rv__ += dump_func;	\
		__ci_dump_pre_suf_rv__ += ci_printf(suf_str);	\
	})

#define ci_argv_each(argv, pp, ...)		\
	do {	\
		if (argv) {		\
			for (const char **pp = (argv); *pp; pp++) {	\
				__VA_ARGS__;	\
			}	\
		}	\
	} while (0)
#define ci_argv_each_with_index(argv, pp, index, ...)		\
	do {	\
		if (argv) {		\
			int index = 0;	\
			for (const char **pp = (argv); *pp; pp++, index++) {	\
				__VA_ARGS__;	\
			}	\
		}	\
	} while (0)	
	

/* dead loop detection */
#define __ci_big_loop0()							__ci_big_loop1(CI_INF_LOOP_DETECT_CNT)
#define __ci_big_loop1(n)			\
	for (int __infinity_loop_counter__ = 0; 	\
		 ci_panic_if(__infinity_loop_counter__ > (n)) || 1;	\
		 __infinity_loop_counter__++)
#define ci_big_loop(...)							ci_va_args_mmaker(__ci_big_loop, __VA_ARGS__)

#define ci_inf_loop_detect()	/* not a common block, use with caution */		\
	ci_dbg_paste(	\
		int __infinity_loop_detect_counter__ = 0;		\
		if (__infinity_loop_detect_counter__++ > CI_INF_LOOP_DETECT_CNT)	\
			ci_panic("INFINITY LOOP DETECTED!");		\
	)


/* inline functions */
static inline void *ci_memset64(void *buf, u64 val, int cnt /* in byte */)
{
	u64 *p = (u64 *)buf;

	ci_assert(buf && (cnt > 0));
	cnt >>= 3;
	while (cnt--)
		*p++ = val;
	
	return buf;
}


/* functions */
u64	 ci_factorial(int n);				/* n!, might overflow */
u64  ci_permutation(int n, int k);		/* P(n, k), might overflow */
u64  ci_combination(int n, int k);		/* C(n, k), might overflow */

void ci_memdump(void *addr, int len, char *desc);
const char *ci_int_to_name(ci_int_to_name_t *i2n, int code);

int  ci_gcd(int a, int b);		 	/* greatest common divisor */
void ci_quick_sort(void *a, int l, int r, int (*compare)(void *, void *));	/* quick sort, a[l..r] */
int  ci_get_argc(const char **argv);

