/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor_intrin.h					Wrapper for Intel intrinsics
 *                                                          hua.ye@Hua Ye.com
 *
 * NOTE: no #pragma once
 */

#include "pal_cfg.h"

#define SIMD_REG_SIZE_BYTE						(SIMD_REG_SIZE_BIT >> 3)
#define SIMD_WORK_SIZE							SIMD_REG_SIZE_BYTE


#if !XOR_CACHE_LINE_REPEAT
	#define SIMD_REPEAT_COUNT									1	
	#define xor_prefetch_sequence(from, NR, ptr, ...)			ci_nop()	
#else
	#define xor_prefetch_sequence(from, NR, ptr, ...)			__ci_m_call_up_to(from, ci_m_dec(NR), xor_prefetch_next, ptr)

	#if PAL_CPU_CACHE_LINE_SIZE / SIMD_REG_SIZE_BYTE == 2
		#define SIMD_REPEAT_COUNT								2	
	#elif PAL_CPU_CACHE_LINE_SIZE / SIMD_REG_SIZE_BYTE == 4
		#define SIMD_REPEAT_COUNT								4	
	#elif PAL_CPU_CACHE_LINE_SIZE / SIMD_REG_SIZE_BYTE == 8
		#define SIMD_REPEAT_COUNT								8	
	#else
		#error "Unknown SIMD_REPEAT_COUNT"
	#endif
#endif


#ifdef PAL_XOR_NON_TEMPORAL
	#define simd_store(a, b)					ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _stream_ps((float *)(a), b))
#else
	#define simd_store(a, b)					ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _storeu_ps((float *)(a), b))
#endif


/*
 *	wrapper for u64 operations 
 */
typedef __m64		__m64i;

typedef union {		/* helper data structure for using __m128 on __m64 */
	__m128		f128;
	__m128i		i128;
	__m64		f64;
} __m64_128_t;

#if SIMD_REG_SIZE_BIT != 64		/* minimum length of the table is 16 bytes */
#define simd_gf_table_t							simd_i_t
#define simd_gf_table_load(a)					((simd_i_t)simd_load(a))
#else
#define simd_gf_table_t							__m128i  
#define simd_gf_table_load(a)					((__m128i)_mm128_loadu_ps((float *)(a)))
#endif

#define _mm64_loadu_ps(x)						*(__m64 *)(x)
#define _mm64_and_ps(a, b)						((__m64)(a) & (__m64)(b))	
#define _mm64_xor_ps(a, b)						((__m64)(a) ^ (__m64)(b))
#define _mm64_storeu_ps(a, b)					(*(__m64 *)(a) = (__m64)(b))
#define _mm64_stream_ps(a, b)					_mm_stream_pi((__m64 *)a, b)
#define _mm64_set1_epi8(a)			\
	({		\
		__m64_128_t __v__;		\
		__v__.i128 = _mm_set1_epi8(a);		\
		__v__.f64;		\
	})
#define _mm64_shuffle_epi8(a, b)		\
	({		\
		__m64_128_t __va__, __vb__;		\
		__va__.i128 = (a);	\
		__vb__.f64 = (b);	\
		__va__.i128 = _mm_shuffle_epi8(__va__.i128, __vb__.i128);		\
		__va__.f64;		\
	})	
#define _mm64_srli_epi64(a, b)	\
	({		\
		__m64_128_t __v__;		\
		__v__.f64 = (a);	\
		__v__.i128 = _mm_srli_epi64(__v__.i128, (b));		\
		__v__.f64;		\
	})



/*
 *	wrapper for 128bit operations 
 */
#define _mm128_xor_ps(a, b)						_mm_xor_ps(a, b)
#define _mm128_and_ps(a, b)						_mm_and_ps(a, b)
#define _mm128_loadu_ps(a)						_mm_loadu_ps(a)
#define _mm128_storeu_ps(a, b)					_mm_storeu_ps(a, b)
#define _mm128_stream_ps(a, b)					_mm_stream_ps(a, b)
#define _mm128_set1_epi8(a)						_mm_set1_epi8(a)
#define _mm128_srli_epi64(a, b)					_mm_srli_epi64(a, b)


/*
 *	Clang issues work around
 */
#ifdef WIN_SIM
#define simd_shuffle(a, b)						ci_token_concat(pal_c_simd_shuffle_, SIMD_REG_SIZE_BIT)(a, b)
#define simd_rshift(a, b)						ci_token_concat(pal_c_simd_rshift_, SIMD_REG_SIZE_BIT)(a, b)
#elif defined(__GNUC__)
#define _mm128_shuffle_epi8(a, b)				_mm_shuffle_epi8(a, b)
#define simd_shuffle(a, b)						(simd_t)ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _shuffle_epi8((simd_gf_table_t)(a), (simd_i_t)(b))) /* table lookup */
#define simd_rshift(a, b)						((simd_t)ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _srli_epi64((simd_i_t)(a), b))) /* each byte rshift b */	
#endif


/*
 *	definition of simd operations 
 */
#define simd_t									ci_token_concat(__m, SIMD_REG_SIZE_BIT)
#define simd_i_t								ci_token_concat(__m, SIMD_REG_SIZE_BIT, i)
#define simd_load(a)							ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _loadu_ps((float *)(a)))
#define simd_xor(a, b)							ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _xor_ps(a, b))
#define simd_and(a, b)							ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _and_ps(a, b))
#define simd_repeat(a)							((simd_t)ci_token_concat(_mm, SIMD_REG_SIZE_BIT, _set1_epi8(a)))			/* memset with a */


#define xor_ptr(n, ptr, ...)					ptr[n],
#define xor_ptr_load_inc(n, ptr, ...)			simd_load(ptr[n]++),
#define gf_xor_ptr_load_inc(n, ptr, t, ...)		\
	t[n] = simd_load(ptr[n]++); 	
#define gf_xor_ptr_mul(n, ptr, t_dst, t_src, mask0, mask1, table0, table1, ...)		\
	t_dst[n] = simd_gf_xor(t_src[n], mask0, mask1, table0[n], table1[n]);	
#define xor_prefetch_next(n, ptr, ...)			_mm_prefetch((char *)ptr[n] + XOR_PREFETCH_STEP * PAL_CPU_CACHE_LINE_SIZE, _MM_HINT_NTA);


#define xor_repeat_helper(n, ...)				xor_repeat_helper2(__VA_ARGS__);
#define xor_repeat_helper2(m, len, ptr, ...)	m(len, ptr, __VA_ARGS__)
#define xor_repeat_sequence(NR, m, len, ptr, ...)		\
	__ci_m_call_up_to_lv2(0, ci_m_dec(NR), xor_repeat_helper, m, len, ptr, __VA_ARGS__)


#define simd_print(var)		\
	({	\
		ci_memdump(&(var), ci_sizeof(var), #var);		\
		ci_printf("\n\n");		\
		var;		\
	})

#define simd_gf_xor(__A__, __mask1__, __mask2__, __table1__, __table2__)		\
	({	\
		simd_t __h__, __l__;		\
		__l__ = simd_and(__A__, __mask1__);		\
		__l__ = simd_shuffle(__table1__, __l__);	\
		__h__ = simd_and(__A__, __mask2__);		\
		__h__ = simd_rshift(__h__, 4);		\
		__h__ = simd_shuffle(__table2__, __h__);	\
		simd_xor(__l__, __h__);		\
	})

#define xor_init_sequence(NR, ptr, buf, offset, type, prefetch_from)			\
	do {	\
		ci_loop(i, NR) {	\
			ptr[i] = (type *)((u8 *)buf[i] + offset);		\
			if (i < prefetch_from)		\
				continue;		\
			ci_loop(j, XOR_PREFETCH_STEP)	\
				_mm_prefetch((char *)ptr[i] + j * PAL_CPU_CACHE_LINE_SIZE, _MM_HINT_NTA);		\
		}		\
	} while (0)

#define xor_p_simd_sequence(NR, ptr, ...)		\
	do {	\
		simd_t __t__ = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(1, ci_m_dec(NR), xor_ptr_load_inc, ptr));	\
		simd_store(ptr[0]++, __t__);	\
	} while (0)

#define xor_q_simd_sequence(NR, ptr, t, mask0, mask1, table0, table1, ...)		\
	do {	\
		__ci_m_call_up_to(1, ci_m_dec(NR), gf_xor_ptr_load_inc, ptr, t);	\
		__ci_m_call_up_to(1, ci_m_dec(NR), gf_xor_ptr_mul, ptr, t, t, mask0, mask1, table0, table1);	\
		t[0] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(1, ci_m_dec(NR), xor_ptr, t));		\
		simd_store(ptr[0]++, t[0]);	\
	} while (0)	

#define xor_pq_rot_simd_sequence(NR, ptr, t, mask0, mask1, table0, table1, ...)		\
	do {	\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_load_inc, ptr, t);	\
		t[0] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(2, ci_m_dec(NR), xor_ptr, t));		\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_mul, ptr, t, t, mask0, mask1, table0, table1);	\
		t[1] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(2, ci_m_dec(NR), xor_ptr, t));		\
		simd_store(ptr[0]++, t[0]);	\
		simd_store(ptr[1]++, t[1]);	\
	} while (0)	

#define xor_pq_rmw_simd_sequence(NR, ptr, t, mask0, mask1, table0, table1, ...)		\
	do {	\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_load_inc, ptr, t);	\
		t[0] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(4, ci_m_dec(NR), xor_ptr, t));		\
		t[0] = simd_xor(t[0], t[2]);		\
		__ci_m_call_up_to(3, ci_m_dec(NR), gf_xor_ptr_mul, ptr, t, t, mask0, mask1, table0, table1);	\
		t[1] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(3, ci_m_dec(NR), xor_ptr, t));		\
		simd_store(ptr[0]++, t[0]);	\
		simd_store(ptr[1]++, t[1]);	\
	} while (0)	

#define xor_qq_simd_sequence(NR, ptr, t0, t1, mask0, mask1, table00, table01, table10, table11, ...)		\
	do {	\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_load_inc, ptr, t0);	\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_mul, ptr, t1, t0, mask0, mask1, table00, table01);	\
		__ci_m_call_up_to(2, ci_m_dec(NR), gf_xor_ptr_mul, ptr, t0, t0, mask0, mask1, table10, table11);	\
		\
		t1[0] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(2, ci_m_dec(NR), xor_ptr, t1));		\
		t0[0] = ci_m_defer64(____ci_m_call_recursive_m2)()(simd_xor, __ci_m_call_up_to(2, ci_m_dec(NR), xor_ptr, t0));		\
		simd_store(ptr[0]++, t1[0]);	\
		simd_store(ptr[1]++, t0[0]);	\
	} while (0)		


#define xor_p_simd_fn_name(NR, ...)			xor_p_simd_ ## NR ci_m_if(ci_m_has_args(__VA_ARGS__))(,)
#define xor_q_simd_fn_name(NR, ...)			xor_q_simd_ ## NR ci_m_if(ci_m_has_args(__VA_ARGS__))(,)
#define xor_pq_rot_simd_fn_name(NR, ...)	xor_pq_rot_simd_ ## NR ci_m_if(ci_m_has_args(__VA_ARGS__))(,)
#define xor_pq_rmw_simd_fn_name(NR, ...)	xor_pq_rmw_simd_ ## NR ci_m_if(ci_m_has_args(__VA_ARGS__))(,)
#define xor_qq_simd_fn_name(NR, ...)		xor_qq_simd_ ## NR ci_m_if(ci_m_has_args(__VA_ARGS__))(,)


#define xor_p_simd_fn_def(NR, ...)		\
	static void xor_p_simd_fn_name(NR) (void **buf, int size, int offset)		\
	{		\
		simd_t *p[NR];		\
		\
		size /= SIMD_REG_SIZE_BYTE * SIMD_REPEAT_COUNT;		\
		xor_init_sequence(NR, p, buf, offset, simd_t, 1);		\
		\
		while (size--) {		\
			xor_prefetch_sequence(1, NR, p);		\
			xor_repeat_sequence(SIMD_REPEAT_COUNT, xor_p_simd_sequence, NR, p);		\
		}		\
	}

#define xor_q_pq_simd_fn_def(NR, simd_fn_name, simd_sequence, prefetch_from, ...)		\
	static void simd_fn_name(NR) (void **buf, u8 *coef, int size, int offset)		\
	{		\
		simd_t *p[NR], t[NR];		\
		simd_t mask0, mask1;		\
		simd_gf_table_t table0[NR], table1[NR];		\
		\
		mask0 = simd_repeat(0x0F);		\
		mask1 = simd_repeat(0xF0);		\
		\
		ci_loop(i, prefetch_from, NR) {	 \
			table0[i] = simd_gf_table_load(pal_gf_xor_table[coef[i]][0]);		\
			table1[i] = simd_gf_table_load(pal_gf_xor_table[coef[i]][1]);		\
		}		\
		\
		size /= SIMD_REG_SIZE_BYTE * SIMD_REPEAT_COUNT;		\
		xor_init_sequence(NR, p, buf, offset, simd_t, prefetch_from);	\
		\
		while (size--) {		\
			xor_prefetch_sequence(prefetch_from, NR, p);	\
			xor_repeat_sequence(SIMD_REPEAT_COUNT, simd_sequence, NR, p, t, mask0, mask1, table0, table1);		\
		}		\
	}

#define xor_qq_simd_fn_def(NR, ...)		\
	static void xor_qq_simd_fn_name(NR) (void **buf, u8 *coef0, u8 *coef1, int size, int offset)		\
	{		\
		simd_t *p[NR], t0[NR], t1[NR];		\
		simd_t mask0, mask1;		\
		simd_gf_table_t table00[NR], table01[NR], table10[NR], table11[NR]; 	\
		\
		mask0 = simd_repeat(0x0F);		\
		mask1 = simd_repeat(0xF0);		\
		\
		ci_loop(i, 2, NR) { 	\
			table00[i] = simd_gf_table_load(pal_gf_xor_table[coef0[i]][0]);		\
			table01[i] = simd_gf_table_load(pal_gf_xor_table[coef0[i]][1]);		\
			table10[i] = simd_gf_table_load(pal_gf_xor_table[coef1[i]][0]);		\
			table11[i] = simd_gf_table_load(pal_gf_xor_table[coef1[i]][1]);		\
		}		\
		\
		size /= SIMD_REG_SIZE_BYTE * SIMD_REPEAT_COUNT; 	\
		xor_init_sequence(NR, p, buf, offset, simd_t, 2);		\
		\
		while (size--) {		\
			xor_prefetch_sequence(2, NR, p);		\
			xor_repeat_sequence(SIMD_REPEAT_COUNT, xor_qq_simd_sequence, NR, p, t0, t1, mask0, mask1, table00, table01, table10, table11);		\
		}		\
	}


#define xor_q_simd_fn_def(NR, ...)		\
	xor_q_pq_simd_fn_def(NR, xor_q_simd_fn_name, xor_q_simd_sequence, 1)

#define xor_pq_rot_simd_fn_def(NR, ...)		\
	xor_q_pq_simd_fn_def(NR, xor_pq_rot_simd_fn_name, xor_pq_rot_simd_sequence, 2)

#define xor_pq_rmw_simd_fn_def(NR, ...)		\
	xor_q_pq_simd_fn_def(NR, xor_pq_rmw_simd_fn_name, xor_pq_rmw_simd_sequence, 2)

	
#define xor_p_simd_code_gen()		\
	ci_m_call_up_to_lv3(3, RG_MAX_XOR_ELM, xor_p_simd_fn_def)		\
	static void(*xor_p_simd_fn[])(void **, int, int) = { NULL, NULL, NULL, ci_m_call_up_to(3, RG_MAX_XOR_ELM, xor_p_simd_fn_name, ~) }

#define xor_q_simd_code_gen()		\
	ci_m_call_up_to_lv3(3, RG_MAX_XOR_ELM, xor_q_simd_fn_def)		\
	static void(*xor_q_simd_fn[])(void **, u8 *, int, int) = { NULL, NULL, NULL, ci_m_call_up_to(3, RG_MAX_XOR_ELM, xor_q_simd_fn_name, ~) }

#define xor_pq_rot_simd_code_gen()		\
	ci_m_call_up_to_lv3(4, RG_MAX_XOR_ELM, xor_pq_rot_simd_fn_def)		\
	static void(*xor_pq_rot_simd_fn[])(void **, u8 *, int, int) = { NULL, NULL, NULL, NULL, ci_m_call_up_to(4, RG_MAX_XOR_ELM, xor_pq_rot_simd_fn_name, ~) }

#define xor_pq_rmw_simd_code_gen()		\
	ci_m_call_up_to_lv3(6, RG_MAX_XOR_ELM, xor_pq_rmw_simd_fn_def)		\
	static void(*xor_pq_rmw_simd_fn[])(void **, u8 *, int, int) = { NULL, NULL, NULL, NULL, NULL, NULL, ci_m_call_up_to(6, RG_MAX_XOR_ELM, xor_pq_rmw_simd_fn_name, ~) }

#define xor_qq_simd_code_gen()		\
	ci_m_call_up_to_lv3(4, RG_MAX_XOR_ELM, xor_qq_simd_fn_def)		\
	static void(*xor_qq_simd_fn[])(void **, u8 *, u8 *, int, int) = { NULL, NULL, NULL, NULL, ci_m_call_up_to(4, RG_MAX_XOR_ELM, xor_qq_simd_fn_name, ~) }
	


