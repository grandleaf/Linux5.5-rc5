/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_bmp.h				Bitmap operations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_bitops.h"

/* definition */
#define CI_BMP_DUMP_BIN										0x0001		/* dump as binary format, e.g. 00110010 */
#define CI_BMP_DUMP_SHOW_EACH								0x0002		/* dump each bit { 3, 4, 27 } */
#define CI_BMP_DUMP_SHOW_EACH_ONLY							0x0004		/* don't dump the bitmap in hex format */
#define CI_BMP_DUMP_SHOW_EACH_COMMA							0x0008		/* use , as separator */

typedef ci_cpu_word_t										ci_bmp_t;
#define CI_BITS_PER_BMP										CI_CPU_WORD_WIDTH
#define CI_BMP_SHIFT										CI_CPU_WORD_SHIFT
#define CI_BMP_MASK											(((ci_bmp_t)1 << CI_BMP_SHIFT) - 1)
#define CI_BMP_MAX											((ci_bmp_t)-1)

#define ci_bmp_size(bits)									(ci_bits_to_bmp(bits) * ci_sizeof(ci_bmp_t))
#define ci_bits_to_bmp(bits)								ci_div_ceil(bits, CI_BITS_PER_BMP)
#define ci_ct_bits_to_bmp(bits)								ci_token_concat(__, bits, _DIV_, CI_BITS_PER_BMP, _ROUND_UP)	/* compile time */
#define ci_bits_to_byte(bits)								ci_div_ceil(bits, 8)

#if 1
#define __ci_bmp_small_const_bits(bits)						(ci_const_expr(bits) && ((bits) <= CI_BMP_SMALL_CONST_BITS))
#else
/* debugging purpose */
#define __ci_bmp_small_const_bits(bits)						\
	({	\
		ci_exec_upto(10, ci_printf("%s\n", (ci_const_expr(bits) && ((bits) <= CI_BMP_SMALL_CONST_BITS)) ? "SMALL" : "BIG"));	\
		(ci_const_expr(bits) && ((bits) <= CI_BMP_SMALL_CONST_BITS));		\
	})
#endif

/*
 * ci_bmp_t operations
 */
#define ci_bmp_elm_mask(from, to)							ci_token_concat(u, CI_BITS_PER_BMP, _mask)(from, to)

#define ci_bmp_elm_bit_is_set(val, bit)						ci_token_concat(u, CI_BITS_PER_BMP, _bit_is_set)(val, bit)
#define ci_bmp_elm_set_bit(val, bit)						ci_token_concat(u, CI_BITS_PER_BMP, _set_bit)(val, bit)					
#define ci_bmp_elm_first_set(val)							ci_token_concat(u, CI_BITS_PER_BMP, _first_set)(val)				
#define ci_bmp_elm_next_set(val, from)						ci_token_concat(u, CI_BITS_PER_BMP, _next_set)(val, from)					
#define ci_bmp_elm_last_set(val)							ci_token_concat(u, CI_BITS_PER_BMP, _last_set)(val)					
#define ci_bmp_elm_prev_set(val, from)						ci_token_concat(u, CI_BITS_PER_BMP, _prev_set)(val, from)				
#define ci_bmp_elm_count_set(val)							ci_token_concat(u, CI_BITS_PER_BMP, _count_set)(val)
#define ci_bmp_elm_is_all_clear(val)						ci_token_concat(u, CI_BITS_PER_BMP, _is_all_clear)(val)
#define ci_bmp_elm_set_range(val, from, to)					ci_token_concat(u, CI_BITS_PER_BMP, _set_range)(val, from, to)
#define ci_bmp_elm_get_range_set(val, from, start, end)		ci_token_concat(u, CI_BITS_PER_BMP, _get_range_set)(val, from, start, end)

#define ci_bmp_elm_bit_is_clear(val, bit)					ci_token_concat(u, CI_BITS_PER_BMP, _bit_is_clear)(val, bit)
#define ci_bmp_elm_clear_bit(val, bit)						ci_token_concat(u, CI_BITS_PER_BMP, _clear_bit)(val, bit)					
#define ci_bmp_elm_first_clear(val)							ci_token_concat(u, CI_BITS_PER_BMP, _first_clear)(val)				
#define ci_bmp_elm_next_clear(val, from)					ci_token_concat(u, CI_BITS_PER_BMP, _next_clear)(val, from)					
#define ci_bmp_elm_last_clear(val)							ci_token_concat(u, CI_BITS_PER_BMP, _last_clear)(val)					
#define ci_bmp_elm_prev_clear(val, from)					ci_token_concat(u, CI_BITS_PER_BMP, _prev_clear)(val, from)				
#define ci_bmp_elm_count_clear(val)							ci_token_concat(u, CI_BITS_PER_BMP, _count_clear)(val)
#define ci_bmp_elm_clear_range(val, from, to)				ci_token_concat(u, CI_BITS_PER_BMP, _clear_range)(val, from, to)
#define ci_bmp_elm_get_range_clear(val, from, start, end)	ci_token_concat(u, CI_BITS_PER_BMP, _get_range_clear)(val, from, start, end)

#define ci_bmp_elm_zero(dst)								(*(dst) = 0)
#define ci_bmp_elm_fill(dst)								(*(dst) = CI_BMP_MAX)
#define ci_bmp_elm_asg(dst, src)							(*(dst) = (src))
#define ci_bmp_elm_copy(dst, src)							(*(dst) = *(src))
#define ci_bmp_elm_equal(dst, src)							(*(dst) == *(src))
#define ci_bmp_elm_not(dst, src)							(*(dst) = ~*(src))
#define ci_bmp_elm_and(dst, src1, src2)						(*(dst) = *(src1) & *(src2))
#define ci_bmp_elm_or(dst, src1, src2)						(*(dst) = *(src1) | *(src2))
#define ci_bmp_elm_xor(dst, src1, src2)						(*(dst) = *(src1) ^ *(src2))
#define ci_bmp_elm_sub(dst, src1, src2)						(*(dst) = *(src1) & ~*(src2))
/*
#define ci_bmp_elm_not_asg(dst)								ci_bmp_elm_not(dst, dst)
#define ci_bmp_elm_and_asg(dst, src)						ci_bmp_elm_and(dst, dst, src)
#define ci_bmp_elm_or_asg(dst, src)							ci_bmp_elm_or(dst, dst, src)
#define ci_bmp_elm_xor_asg(dst, src)						ci_bmp_elm_xor(dst, dst, src)
#define ci_bmp_elm_sub_asg(dst, src)						ci_bmp_elm_sub(dst, dst, src)
*/
#define ci_bmp_elm_or_mask(dst, mask)						(*(dst) |= (mask))
#define ci_bmp_elm_and_not_mask(dst, mask)					(*(dst) &= ~(mask))


#define __ci_bmp_elm_sop_1(n, op, dst, ...)					op(dst + n);
#define __ci_bmp_elm_sop_2(n, op, dst, src, ...)			op(dst + n, src + n);
#define __ci_bmp_elm_sop_3(n, op, dst, src1, src2, ...)		op(dst + n, src1 + n, src2 + n);
#define __ci_bmp_elm_sop_1_sum(n, op, dst, sum, ...)		sum += op(*(dst + n));
#define __ci_bmp_elm_sop_1_bool(n, op, dst, ...)			&& op(*(dst + n))


/* use CI_BMP_ELM_FILL_INITIALIZER to define a static initializer */
#define __ci_bmp_sfill(n, ...)								CI_BMP_MAX,
#define __ci_bmp_elm_shift(bits)							(CI_BITS_PER_BMP - ((bits) & CI_BMP_MASK))
#define __ci_bmp_elm_last_fill(bits)	\
	(CI_BMP_MAX & (CI_BMP_MAX >> (__ci_bmp_elm_shift(bits) == CI_BITS_PER_BMP ? 0 : __ci_bmp_elm_shift(bits))))
#define __CI_BMP_INITIALIZER(bits)	\
	ci_m_if_else(ci_m_equal(ci_ct_bits_to_bmp(bits), 1)) (	\
		__ci_bmp_elm_last_fill(bits)	\
	)(	\
		__ci_m_call_up_to(2, ci_ct_bits_to_bmp(bits), __ci_bmp_sfill)		\
		__ci_bmp_elm_last_fill(bits)	\
	)	
#define CI_BMP_INITIALIZER(bits)							ci_m_expand8(__CI_BMP_INITIALIZER(bits))


/*
 *  NOTE: bits must be a constant: e.g. 
 *		OK: 	#define BITS 64
 *  	ERROR: 	#define BITS (63 + 1)
 *
 *	Use ci_bmp_def to define a bitmap: e.g. ci_bmp_def(my_bmp, 64) will define type "my_bmp_t".
 *	And following functions and their "asg: assignment" buddy will available:
 *	my_bmp_zero(), my_bmp_fill(), ...
 */
#define ci_bmp_def(prefix, bits)			\
	typedef struct { ci_bmp_t __buf[ci_bits_to_bmp(bits)]; } prefix ## _t;		\
	__ci_bmp_fn_def1(prefix, bits, zero)		\
	__ci_bmp_fn_def1(prefix, bits, fill)		\
	__ci_bmp_fn_def2(prefix, bits, copy)		\
	__ci_bmp_fn_def2(prefix, bits, not)		\
	__ci_bmp_fn_def3(prefix, bits, and)		\
	__ci_bmp_fn_def3(prefix, bits, or)		\
	__ci_bmp_fn_def3(prefix, bits, xor)		\
	__ci_bmp_fn_def3(prefix, bits, sub)		\
	\
	__ci_bmp_fn_bit_def1(prefix, bits, set_bit);		\
	__ci_bmp_fn_bit_def1(prefix, bits, bit_is_set);		\
	__ci_bmp_fn_bit_def0(prefix, bits, first_set);	\
	__ci_bmp_fn_bit_def1(prefix, bits, next_set);	\
	__ci_bmp_fn_bit_def0(prefix, bits, last_set);	\
	__ci_bmp_fn_bit_def1(prefix, bits, prev_set);	\
	__ci_bmp_fn_int_def1(prefix, bits, count_set);		\
	__ci_bmp_fn_int_def1(prefix, bits, is_all_clear);		\
	\
	__ci_bmp_fn_bit_def1(prefix, bits, clear_bit);			\
	__ci_bmp_fn_bit_def1(prefix, bits, bit_is_clear);		\
	__ci_bmp_fn_bit_def0(prefix, bits, first_clear);	\
	__ci_bmp_fn_bit_def1(prefix, bits, next_clear);	\
	__ci_bmp_fn_bit_def0(prefix, bits, last_clear);	\
	__ci_bmp_fn_bit_def1(prefix, bits, prev_clear);	\
	__ci_bmp_fn_int_def1(prefix, bits, count_clear);		\
	\
	static inline int prefix ## _equal(void *dst, void *src)		\
	{ return ci_bmp_equal(dst, src, bits);	}	\
	\
	static inline prefix ## _t *prefix ## _mask(void *bmp, int from, int to)		\
	{ return (prefix ## _t *)ci_bmp_mask(bmp, bits, from, to); }	\
	\
	static inline int prefix ## _empty(void *bmp)	\
	{ return prefix ## _count_set(bmp) == 0; }	\
	\
	static inline int prefix ## _full(void *bmp)	\
	{ return prefix ## _count_set(bmp) == bits; }	\
	\
	static inline prefix ## _t *prefix ## _set_range(void *bmp, int from, int to)	\
	{ return (prefix ## _t *)ci_bmp_set_range(bmp, bits, from, to); }	\
	\
	static inline int prefix ## _dump(void *bmp)		\
	{ return ci_bmp_dump(bmp, bits); }		\
	\
	static inline int prefix ## _dumpln(void *bmp)		\
	{ return ci_bmp_dump(bmp, bits) + ci_printfln(); }		\
	\
	static inline int prefix ## _bin_dump(void *bmp)		\
	{ return ci_bmp_bin_dump(bmp, bits); }		\
	\
	static inline int prefix ## _bin_dumpln(void *bmp)		\
	{ return ci_bmp_bin_dump(bmp, bits) + ci_printfln(); }		\
	\
	static inline int prefix ## _dump_ex(void *bmp, int fmt)		\
	{ return __ci_bmp_dump(bmp, bits, fmt); }		\
	\
	static inline int prefix ## _dump_exln(void *bmp, int fmt)		\
	{ return __ci_bmp_dump(bmp, bits, fmt) + ci_printfln(); }		\
	\
	static inline int prefix ## _sn_dump(char *buf, int len, void *bmp)		\
	{ return ci_bmp_sn_dump(buf, len, bmp, bits, 0); }		\


#define ci_bmp_dump(bmp, bits)								__ci_bmp_dump(bmp, bits, 0)
#define ci_bmp_bin_dump(bmp, bits)							__ci_bmp_dump(bmp, bits, CI_BMP_DUMP_BIN)

#define ci_bmp_zero(bmp, bits)								__ci_bmp_op_1_def(bmp, bits, ci_bmp_elm_zero)		
#define ci_bmp_fill(bmp, bits)								__ci_bmp_op_1_def(bmp, bits, ci_bmp_elm_fill)	
#define ci_bmp_copy(dst, src, bits)							__ci_bmp_op_2_def(dst, src, bits, ci_bmp_elm_copy)
#define ci_bmp_equal(dst, src, bits)						__ci_bmp_equal_def(dst, src, bits)
#define ci_bmp_not(dst, src, bits)							__ci_bmp_op_2_def(dst, src, bits, ci_bmp_elm_not)
#define ci_bmp_and(dst, src1, src2, bits)					__ci_bmp_op_3_def(dst, src1, src2, bits, ci_bmp_elm_and)
#define ci_bmp_or(dst, src1, src2, bits)					__ci_bmp_op_3_def(dst, src1, src2, bits, ci_bmp_elm_or)
#define ci_bmp_xor(dst, src1, src2, bits)					__ci_bmp_op_3_def(dst, src1, src2, bits, ci_bmp_elm_xor)
#define ci_bmp_sub(dst, src1, src2, bits)					__ci_bmp_op_3_def(dst, src1, src2, bits, ci_bmp_elm_sub)
#define ci_bmp_mask(bmp, bits, from, to)					__ci_bmp_range_op(bmp, bits, from, to, ci_bmp_elm_asg, ci_m_expand, ci_bmp_elm_zero)
#define ci_bmp_not_asg(dst, bits)							ci_bmp_not(dst, dst, bits)
#define ci_bmp_and_asg(dst, src, bits)						ci_bmp_and(dst, dst, src, bits)
#define ci_bmp_or_asg(dst, src, bits)						ci_bmp_or(dst, dst, src, bits)
#define ci_bmp_xor_asg(dst, src, bits)						ci_bmp_xor(dst, dst, src, bits)
#define ci_bmp_sub_asg(dst, src, bits)						ci_bmp_sub(dst, dst, src, bits)
#define ci_bmp_copy_asg(dst, bits)							ci_bmp_copy(dst, dst, bits)			/* dummy */


#define ci_bmp_set_bit(bmp, bits, bit)						__ci_bmp_bit_op(bmp, bits, bit, set_bit)
#define ci_bmp_bit_is_set(bmp, bits, bit)					__ci_bmp_bit_op(bmp, bits, bit, bit_is_set)
#define ci_bmp_first_set(bmp, bits)							ci_bmp_next_set(bmp, bits, 0)	/* as fast as a dedicate implementation */
#define ci_bmp_next_set(bmp, bits, from)					__ci_bmp_next(bmp, bits, from, set)	
#define ci_bmp_last_set(bmp, bits)							ci_bmp_prev_set(bmp, bits, (bits - 1))
#define ci_bmp_prev_set(bmp, bits, from)					__ci_bmp_prev(bmp, bits, from, set)	
#define ci_bmp_count_set(bmp, bits)							__ci_bmp_op_1_sum_def(bmp, bits, ci_bmp_elm_count_set)	
#define ci_bmp_is_all_clear(bmp, bits)						__ci_bmp_op_1_bool_def(bmp, bits, ci_bmp_elm_is_all_clear)	
#define ci_bmp_set_range(bmp, bits, from, to)				__ci_bmp_range_op(bmp, bits, from, to, ci_bmp_elm_or_mask, ci_nop, ~)

#define ci_bmp_clear_bit(bmp, bits, bit)					__ci_bmp_bit_op(bmp, bits, bit, clear_bit)
#define ci_bmp_bit_is_clear(bmp, bits, bit)					__ci_bmp_bit_op(bmp, bits, bit, bit_is_clear)
#define ci_bmp_first_clear(bmp, bits)						ci_bmp_next_clear(bmp, bits, 0)
#define ci_bmp_next_clear(bmp, bits, from)					__ci_bmp_next(bmp, bits, from, clear)	
#define ci_bmp_last_clear(bmp, bits)						ci_bmp_prev_clear(bmp, bits, (bits - 1))
#define ci_bmp_prev_clear(bmp, bits, from)					__ci_bmp_prev(bmp, bits, from, clear)	
#define ci_bmp_count_clear(bmp, bits)						((bits) - ci_bmp_count_set(bmp, bits))
#define ci_bmp_clear_range(bmp, bits, from, to)				__ci_bmp_range_op(bmp, bits, from, to, ci_bmp_elm_and_not_mask, ci_nop, ~)

/* C(n, k), n = bits, k = sets */
#define ci_bmp_combo(bmp, bits, sets, ...)			\
	({		\
		ci_bmp_t *__combo_bmp__ = (ci_bmp_t *)(bmp);		\
		ci_bmp_combo_init(__combo_bmp__, bits, sets);		\
		\
		do {	\
			__VA_ARGS__;		\
		} while (ci_bmp_combo_next(__combo_bmp__, COMBO_BITS));		\
		\
		__combo_bmp__;		\
	})

#define ci_bmp_each_set(bmp, bits, idx)		\
	for (int idx = 0; (idx = ci_bmp_next_set(bmp, bits, idx)) >= 0; idx++)
#define ci_bmp_each_clear(bmp, bits, idx)	\
	for (int idx = 0; (idx = ci_bmp_next_clear(bmp, bits, idx)) >= 0; idx++)
#define ci_bmp_each_range(bmp, bits, start, end, ...)		\
	({		\
		int start, end;		\
		ci_bmp_t *__range_bmp__ = (ci_bmp_t *)(bmp);		\
		\
		for (start = end = 0; \
			 (end != bits) &&		\
				((start = ci_bmp_next_set(__range_bmp__, bits, start)) >= 0) &&	\
				((end = ci_bmp_next_clear(__range_bmp__, bits, start + 1))) &&		\
				((end < 0) && (end = bits), 1);		\
			 start = end)		\
		{		\
			__VA_ARGS__;	\
		}	\
		\
		__range_bmp__;		\
	})
#define ci_bmp_next_range(bmp, bits, from, start, end)		\
	({		\
		ci_bmp_t *__range_bmp__ = (ci_bmp_t *)(bmp);		\
		\
		if ((start = ci_bmp_next_set(__range_bmp__, bits, from)) >= 0)	\
			if ((end = ci_bmp_next_clear(__range_bmp__, bits, start + 1)) < 0)		\
				end = bits;		\
		start;	\
	})
#define ci_bmp_first_range(bmp, bits, start, end)		ci_bmp_next_range(bmp, bits, 0, start, end)



#define __ci_bmp_fn_bit_def1(prefix, bits, op)		\
	static inline int prefix ## _ ## op(void *bmp, int arg)		\
		{ return  ci_bmp_ ## op(bmp, bits, arg);	}	

#define __ci_bmp_fn_bit_def0(prefix, bits, op)		\
	static inline int prefix ## _ ## op(void *bmp)		\
		{ return  ci_bmp_ ## op(bmp, bits);	}	

#define __ci_bmp_fn_def1(prefix, bits, op)		\
	static inline prefix ## _t *prefix ## _ ## op(void *bmp)		\
		{ return (prefix ## _t *)ci_bmp_ ## op(bmp, bits);	}	

#define __ci_bmp_fn_int_def1(prefix, bits, op)		\
	static inline int prefix ## _ ## op(void *bmp)		\
		{ return ci_bmp_ ## op(bmp, bits);	}	

#define __ci_bmp_fn_def2(prefix, bits, op)		\
	static inline prefix ## _t *prefix ## _ ## op(void *dst, void *src)		\
		{ return (prefix ## _t *)ci_bmp_ ## op(dst, src, bits);	}	\
	static inline prefix ## _t *prefix ## _ ## op ## _asg(void *dst)		/* designate: dst = op dst */	\
		{ return (prefix ## _t *)ci_bmp_ ## op ## _asg(dst, bits);	}	

#define __ci_bmp_fn_def3(prefix, bits, op)		\
	static inline prefix ## _t *prefix ## _ ## op(void *dst, void *src1, void *src2)	\
		{ return (prefix ## _t *)ci_bmp_ ## op(dst, src1, src2, bits);	}	\
	static inline prefix ## _t *prefix ## _ ## op ## _asg(void *dst, void *src)		/* designate: dst = dst op src */	\
		{ return (prefix ## _t *)ci_bmp_ ## op ## _asg(dst, src, bits);	}	


#define __ci_bmp_op_1_def(bmp, bits, op)		/* dst = op(dst) */ \
	({		\
		int __shift__;		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(bmp),		\
				 *__dst_save__ = __dst__,			\
				 __mask__;		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)),	__ci_bmp_elm_sop_1, op, __dst__);		\
		} else) {	\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				op(__dst__);		\
				__dst__++;		\
			}		\
		}		\
		\
		if (ci_unlikely((__shift__ = bits & CI_BMP_MASK))) {		\
			__mask__ = CI_BMP_MAX >> (CI_BITS_PER_BMP - __shift__);		\
			__dst__ = __dst_save__ + ci_bits_to_bmp(bits) - 1;		\
			*__dst__ &= __mask__;		\
		}		\
		\
		__dst_save__;	\
	})

#define __ci_bmp_op_1_sum_def(bmp, bits, op)		/* sum += op(dst) */ \
	({		\
		int __sum__ = 0;		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(bmp);		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)),	__ci_bmp_elm_sop_1_sum, op, __dst__, __sum__);		\
		} else) {	\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				__sum__ += op(*__dst__);		\
				__dst__++;		\
			}		\
		}		\
		\
		__sum__;	\
	})

#define __ci_bmp_op_1_bool_def(bmp, bits, op)		/* bool = op(dst) */ \
	({		\
		int __bool__;		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(bmp);		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			__bool__ = 1 ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)), __ci_bmp_elm_sop_1_bool, op, __dst__);		\
		} else) {	\
			__bool__ = 1;	\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				if (!op(*__dst__))	{	\
					__bool__ = 0;	\
					break;	\
				}	\
				__dst__++;		\
			}		\
		}		\
		\
		__bool__;	\
	})		

#define __ci_bmp_op_2_def(dst, src, bits, op)		/* dst = op src */ \
	({		\
		int __shift__;		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(dst),		\
				 *__src__ = (ci_bmp_t *)(src),		\
				 *__dst_save__ = __dst__,			\
				 __mask__;		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)), __ci_bmp_elm_sop_2, op, __dst__, __src__);		\
		} else) {	\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				op(__dst__, __src__);		\
				__dst__++, __src__++;		\
			}		\
		}		\
		\
		if ((__shift__ = bits & CI_BMP_MASK)) {		\
			__mask__ = CI_BMP_MAX >> (CI_BITS_PER_BMP - __shift__);		\
			__dst__ = __dst_save__ + ci_bits_to_bmp(bits) - 1;		\
			*__dst__ &= __mask__;		\
		}		\
		\
		__dst_save__;	\
	})

#define __ci_bmp_elm_sequal(n, dst, src, ...)			&& ci_bmp_elm_equal(dst + n, src + n)
#define __ci_bmp_equal_def(dst, src, bits)		\
	({		\
		int __bmp_equal__;		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(dst),		\
				 *__src__ = (ci_bmp_t *)(src);		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			__bmp_equal__ = 1 ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)), __ci_bmp_elm_sequal, __dst__, __src__);	\
		} else) {	\
			__bmp_equal__ = 1;	\
			\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				if (!ci_bmp_elm_equal(__dst__, __src__))	{	\
					__bmp_equal__ = 0;	\
					break;		\
				}	\
				__dst__++, __src__++;		\
			}		\
		}		\
		__bmp_equal__;	\
	})

#define __ci_bmp_op_3_def(dst, src1, src2, bits, op)	/* dst = src1 op src2 */	\
	({		\
		ci_bmp_t *__dst__ = (ci_bmp_t *)(dst),		\
				 *__src1__ = (ci_bmp_t *)(src1),		\
				 *__src2__ = (ci_bmp_t *)(src2),		\
				 *__dst_save__ = __dst__;		\
		\
		ci_m_if(ci_m_is_comparable(bits)) ( 		\
		if (__ci_bmp_small_const_bits(bits)) {		\
			ci_m_call_up_to8(0, ci_m_dec(ci_ct_bits_to_bmp(bits)), __ci_bmp_elm_sop_3, op, __dst__, __src1__, __src2__);	\
		} else) {	\
			ci_loop(__bmp_i__, ci_bits_to_bmp(bits)) {		\
				op(__dst__, __src1__, __src2__);		\
				__dst__++, __src1__++, __src2__++;		\
			}		\
		}		\
		\
		__dst_save__;	\
	})

//		ci_printf("__from_idx__=%d, __to_idx__=%d\n", __from_idx__, __to_idx__);	
//		ci_printf("__from_mask__=%#016llX, __to_mask__=%#016llX\n", __from_mask__, __to_mask__);		
#define __ci_bmp_range_op(bmp, bits, from, to, op, loop, rest_op)		\
	({	\
		int __from__ = from, __to__ = to, __from_idx__, __to_idx__;		\
		ci_bmp_t *__bmp__ = (ci_bmp_t *)(bmp), __from_mask__, __to_mask__;		\
		\
		ci_range_check(__from__, 0, (bits));		\
		ci_range_check_i(__to__, (__from__) + 1, bits);		\
		\
		__from_idx__ = __from__ >> CI_BMP_SHIFT;		\
		__to_idx__ = (__to__ - 1) >> CI_BMP_SHIFT;	\
		ci_loop(__bmp_idx__, __from_idx__ + 1, __to_idx__)		\
			op(__bmp__ + __bmp_idx__, CI_BMP_MAX);	\
		\
		__from_mask__ = CI_BMP_MAX << (__from__ & CI_BMP_MASK);		\
		__to_mask__ = CI_BMP_MAX >> (CI_BITS_PER_BMP - (__to__ & CI_BMP_MASK));		\
		if (__from_idx__ == __to_idx__)		\
			op(__bmp__ + __from_idx__, __from_mask__ & __to_mask__);	\
		else {	\
			op(__bmp__ + __from_idx__, __from_mask__);		\
			op(__bmp__ + __to_idx__, __to_mask__);		\
		}	\
		\
		loop(		\
			ci_loop(__bmp_idx__, __from_idx__)		\
				rest_op(__bmp__ + __bmp_idx__);		\
			ci_loop(__bmp_idx__, __to_idx__ + 1, ci_bits_to_bmp(bits))		\
				rest_op(__bmp__ + __bmp_idx__);		\
		);		\
		\
		__bmp__;		\
	})

#define __ci_bmp_bit_op(bmp, bits, bit, op)				\
	({		\
		ci_bmp_t *__bmp__ = (ci_bmp_t *)(bmp);		\
		int __bit__ = bit, __index__ = __bit__ >> CI_BMP_SHIFT, __offset__ = __bit__ & CI_BMP_MASK;		\
		\
		ci_range_check(__bit__, 0, bits);		\
		ci_bmp_elm_ ## op(*(__bmp__ + __index__), __offset__);		\
	})

#define __ci_bmp_next(bmp, bits, from, set_clear)		\
	({		\
		int __rv__, __from__ = from;	\
		ci_assert(__from__ >= 0);		\
		\
		if (ci_unlikely(__from__ >= bits))	\
			__rv__ = -1;	\
		else {	\
			ci_bmp_t *__bmp__, *__first_bmp__ = (ci_bmp_t *)(bmp), *__last_bmp__ = __first_bmp__ + ci_bits_to_bmp(bits);		\
			int __index__ = __from__ >> CI_BMP_SHIFT, __offset__ = __from__ & CI_BMP_MASK;		\
			\
			__bmp__ = __first_bmp__ + __index__;		\
			\
			if (((__rv__ = ci_bmp_elm_next_ ## set_clear(*__bmp__, __offset__)) < 0) && (__bmp__ < __last_bmp__ - 1))		\
				do {	\
					__bmp__++;	\
					__rv__ = ci_bmp_elm_first_ ## set_clear(*__bmp__);		\
				} while ((__rv__ < 0) && (__bmp__ < __last_bmp__ - 1));		\
			\
			if (__rv__ >= 0) {		\
				__rv__ = ((__bmp__ - __first_bmp__) << CI_BMP_SHIFT) + __rv__;		\
				ci_m_if(ci_m_equal(set_clear, clear))	\
				(	\
					if (ci_unlikely(__rv__ >= bits))		\
						__rv__ = -1;		\
				) 		\
			}		\
		}	\
		__rv__;		\
	})

#define __ci_bmp_prev(bmp, bits, from, set_clear)		\
	({		\
		int __rv__, __from__ = from;	\
		ci_assert(__from__ < bits);		\
		\
		if (ci_unlikely(__from__ < 0))	\
			__rv__ = -1;	\
		else {	\
			ci_bmp_t *__bmp__, *__first_bmp__ = (ci_bmp_t *)(bmp);		\
			int __index__ = __from__ >> CI_BMP_SHIFT, __offset__ = __from__ & CI_BMP_MASK;		\
			\
			__bmp__ = __first_bmp__ + __index__;		\
			\
			ci_assert(__from__ < bits);		\
			if (((__rv__ = ci_bmp_elm_prev_ ## set_clear(*__bmp__, __offset__)) < 0) && (__bmp__ > __first_bmp__))		\
			do {	\
				__bmp__--;		\
				__rv__ = ci_bmp_elm_last_ ## set_clear(*__bmp__);		\
			} while ((__rv__ < 0) && (__bmp__ > __first_bmp__));		\
			\
			if (__rv__ >= 0)	\
				__rv__ = ((__bmp__ - __first_bmp__) << CI_BMP_SHIFT) + __rv__;		\
		}	\
		__rv__;		\
	})

#define __ci_bmp_dump(bmp, bits, fmt)		ci_bmp_sn_dump(NULL, 0, bmp, bits, fmt)



/*
 *	exported functions
 */
ci_bmp_t *ci_bmp_combo_init(void *__bmp, int bits, int sets);
int ci_bmp_combo_next(void *__bmp, int bits);
int ci_bmp_sn_dump(char *buf, int len, void *__bmp, int bits, int fmt);

