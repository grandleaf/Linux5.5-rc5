/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_macro.h					Helper Macros
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_macro_inc.h"


#define ci_m_inc(x)							ci_m_concat(ci_m_inc_, x)
#define ci_m_dec(x)							ci_m_concat(ci_m_dec_, x)

#define ci_m_expand_inf(...)				ci_m_expand64(__VA_ARGS__)		/* enlarge this if compile error, smaller value compiles faster */
#define ci_m_expand2048(...)				ci_m_expand1024(ci_m_expand1024(__VA_ARGS__))
#define ci_m_expand1024(...)				ci_m_expand512(ci_m_expand512(__VA_ARGS__))
#define ci_m_expand512(...)					ci_m_expand256(ci_m_expand256(__VA_ARGS__))
#define ci_m_expand256(...)					ci_m_expand128(ci_m_expand128(__VA_ARGS__))
#define ci_m_expand128(...)					ci_m_expand64(ci_m_expand64(__VA_ARGS__))
#define ci_m_expand64(...)					ci_m_expand32(ci_m_expand32(__VA_ARGS__))
#define ci_m_expand32(...)					ci_m_expand16(ci_m_expand16(__VA_ARGS__))
#define ci_m_expand16(...)					ci_m_expand8(ci_m_expand8(__VA_ARGS__))
#define ci_m_expand4(...)					ci_m_expand2(ci_m_expand2(__VA_ARGS__))
#define ci_m_expand8(...)					ci_m_expand4(ci_m_expand4(__VA_ARGS__))
#define ci_m_expand2(...)					ci_m_expand1(ci_m_expand1(__VA_ARGS__))
#define ci_m_expand1(...)					__VA_ARGS__
#define ci_m_expand(...)					__VA_ARGS__

#define ci_m_expand7(...)					ci_m_expand1(ci_m_expand6(__VA_ARGS__))
#define ci_m_expand6(...)					ci_m_expand1(ci_m_expand5(__VA_ARGS__))
#define ci_m_expand5(...)					ci_m_expand1(ci_m_expand4(__VA_ARGS__))
#define ci_m_expand3(...)					ci_m_expand1(ci_m_expand2(__VA_ARGS__))

#define ci_m_empty()						/* used for defered expansion */
#define ci_m_concat(a, b)					a ## b
#define ci_m_first(a, ...)					a
#define ci_m_rest(x, ...)                   __VA_ARGS__
#define ci_m_second(a, b, ...)				b
#define ci_m_probe()						~, 1
#define ci_m_check(...)						ci_m_expand(ci_m_second(__VA_ARGS__, 0))

/* x == () ? */
#define ci_m_is_paren(x)					ci_m_check(ci_m_is_paren_probe x)
#define ci_m_is_paren_probe(...)			ci_m_probe()
#define ci_m_is_comparable(x)				ci_m_is_paren(ci_m_concat(ci_m_cmp_, x) (()))

/* (x) => x */
#define ci_m_args(...)						__VA_ARGS__
#define __ci_m_strip_paren(x)				x
#define ci_m_strip_paran(x)					__ci_m_strip_paren(ci_m_args x)

#define ci_m_not(x)							ci_m_check(ci_m_concat(__ci_m_not_, x))
#define __ci_m_not_0						ci_m_probe()
#define ci_m_bool(x)						ci_m_not(ci_m_not(x))

#define ci_m_and(x)							ci_m_concat(__ci_m_and_, x)
#define __ci_m_and_0(x)						0
#define __ci_m_and_1(x)						x


#define ci_m_if_else(condition)				__ci_m_if_else(ci_m_bool(condition))
#define __ci_m_if_else(condition)			ci_m_concat(__ci_m_if_, condition)

#define __ci_m_if_1(...)					__VA_ARGS__ __ci_m_if_1_else
#define __ci_m_if_0(...)					__ci_m_if_0_else

#define __ci_m_if_1_else(...)
#define __ci_m_if_0_else(...)				__VA_ARGS__

#define ci_m_if(condition)					__ci_m_if(ci_m_bool(condition))
#define __ci_m_if(condition)				ci_m_concat(__ci_m_if_only_, condition)
#define __ci_m_if_only_1(...)				__VA_ARGS__
#define __ci_m_if_only_0(...)

/*
 *	#define ci_m_has_args(...)					ci_m_bool(ci_m_first(__ci_m_end_of_args __VA_ARGS__)())
 *	#define __ci_m_end_of_args()				0
 *
 *	You are in trouble if arg like (u32)val.
 */
#define ci_m_has_args(...)					ci_m_not_equal(ci_m_argc(__VA_ARGS__), 0)


/* helper function internal used by ci_m_equal() */
#define ci_m_cmp(x, y) ci_m_is_paren \
	( \
		ci_m_cmp_ ## x (ci_m_cmp_ ## y)(())  \
	)

/* compare if two tokens are equal */
#define ci_m_equal(a, b)		\
    ci_m_if_else(ci_m_and(ci_m_is_comparable(a))(ci_m_is_comparable(b))) \
    ( \
		ci_m_not(ci_m_cmp(a, b))		\
    )		\
	(	\
		0		\
	)
#define ci_m_not_equal(a, b)						ci_m_not(ci_m_equal(a, b))

/* 
 * return how many arguments in the macro 
 * NOTE: In order to return 0 (empty argc), you need use -std=gnu99, not -std=c99
 *       Since we need a GNU extension to swallow the ','.
 */
#define ci_m_argc(...)		\
	__ci_m_argc(,	##__VA_ARGS__,	\
					255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, \
					239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, \
					223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, \
					207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, \
					191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, \
					175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, \
					159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, \
					143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128, \
					127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, \
					111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100,  99,  98,  97,  96, \
					 95,  94,  93,  92,  91,  90,  89,  88,  87,  86,  85,  84,  83,  82,  81,  80, \
					 79,  78,  77,  76,  75,  74,  73,  72,  71,  70,  69,  68,  67,  66,  65,  64, \
					 63,  62,  61,  60,  59,  58,  57,  56,  55,  54,  53,  52,  51,  50,  49,  48, \
					 47,  46,  45,  44,  43,  42,  41,  40,  39,  38,  37,  36,  35,  34,  33,  32, \
					 31,  30,  29,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17,  16, \
					 15,  14,  13,  12,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0	\
	)
#define __ci_m_argc(_255, _254, _253, _252, _251, _250, _249, _248, _247, _246, _245, _244, _243, _242, _241, _240, \
					_239, _238, _237, _236, _235, _234, _233, _232, _231, _230, _229, _228, _227, _226, _225, _224, \
					_223, _222, _221, _220, _219, _218, _217, _216, _215, _214, _213, _212, _211, _210, _209, _208, \
					_207, _206, _205, _204, _203, _202, _201, _200, _199, _198, _197, _196, _195, _194, _193, _192, \
					_191, _190, _189, _188, _187, _186, _185, _184, _183, _182, _181, _180, _179, _178, _177, _176, \
					_175, _174, _173, _172, _171, _170, _169, _168, _167, _166, _165, _164, _163, _162, _161, _160, \
					_159, _158, _157, _156, _155, _154, _153, _152, _151, _150, _149, _148, _147, _146, _145, _144, \
					_143, _142, _141, _140, _139, _138, _137, _136, _135, _134, _133, _132, _131, _130, _129, _128, \
					_127, _126, _125, _124, _123, _122, _121, _120, _119, _118, _117, _116, _115, _114, _113, _112, \
					_111, _110, _109, _108, _107, _106, _105, _104, _103, _102, _101, _100,  _99,  _98,  _97,  _96, \
					 _95,  _94,  _93,  _92,  _91,  _90,  _89,  _88,  _87,  _86,  _85,  _84,  _83,  _82,  _81,  _80, \
					 _79,  _78,  _77,  _76,  _75,  _74,  _73,  _72,  _71,  _70,  _69,  _68,  _67,  _66,  _65,  _64, \
					 _63,  _62,  _61,  _60,  _59,  _58,  _57,  _56,  _55,  _54,  _53,  _52,  _51,  _50,  _49,  _48, \
					 _47,  _46,  _45,  _44,  _43,  _42,  _41,  _40,  _39,  _38,  _37,  _36,  _35,  _34,  _33,  _32, \
					 _31,  _30,  _29,  _28,  _27,  _26,  _25,  _24,  _23,  _22,  _21,  _20,  _19,  _18,  _17,  _16, \
					 _15,  _14,  _13,  _12,  _11,  _10,   _9,   _8,   _7,   _6,   _5,   _4,   _3,   _2,   _1,   _0, \
					c, ...) c
					



/* iterator each argument by using m */
#define __ci_m_each(m, first, ...)           \
	m(first)                           \
	ci_m_if(ci_m_has_args(__VA_ARGS__))(    \
		ci_m_defer2(____ci_m_each)()(m, __VA_ARGS__)   \
	)	
#define ____ci_m_each()									__ci_m_each
#define ci_m_each(...)									ci_m_expand_inf(__ci_m_each(__VA_ARGS__))


/* iterator each argument by using m with argument1 */
#define __ci_m_each_m1(m, a1, first, ...)           \
	m(a1, first)                           \
	ci_m_if(ci_m_has_args(__VA_ARGS__))(    \
		ci_m_defer2(____ci_m_each_m1)()(m, a1, __VA_ARGS__)   \
	)	
#define ____ci_m_each_m1()									__ci_m_each_m1
#define ci_m_each_m1(...)									ci_m_expand_inf(__ci_m_each_m1(__VA_ARGS__))


/* call recursive macro with 2 arguments */
#define __ci_m_call_recursive_m2(m2, core, first, ...)	\
	ci_m_if_else(ci_m_has_args(__VA_ARGS__))		\
		(		\
			ci_m_defer2(____ci_m_call_recursive_m2)()(m2, m2(core, first), __VA_ARGS__)	\
		)		\
		(	\
			m2(core, first)		\
		)
#define ____ci_m_call_recursive_m2()					__ci_m_call_recursive_m2
#define ci_m_call_recursive_m2(...)						ci_m_expand_inf(__ci_m_call_recursive_m2(__VA_ARGS__))


/* iterates [from, to] incrementally by invoke the macro */
#define __ci_m_call_up_to(from, to, macro, ...)		\
	macro(from, __VA_ARGS__)		\
	\
	ci_m_if(ci_m_not_equal(from, to))	\
	(	\
		ci_m_defer2(____ci_m_call_up_to)()(ci_m_inc(from), to, macro, __VA_ARGS__)		\
	)
#define ____ci_m_call_up_to()							__ci_m_call_up_to
#define ci_m_call_up_to(from, to, macro, ...)			ci_m_expand_inf(__ci_m_call_up_to(from, to, macro, __VA_ARGS__))
#define ci_m_call_up_to8(from, to, macro, ...)			ci_m_expand8(__ci_m_call_up_to(from, to, macro, __VA_ARGS__))


/* Level 2 macro, if you want to call ci_m_call_up_to() nested */
#define __ci_m_call_up_to_lv2(from, to, macro, ...)		\
	macro(from, __VA_ARGS__)			\
	\
	ci_m_if(ci_m_not_equal(from, to))	\
	(	\
		ci_m_defer2(____ci_m_call_up_to_lv2)()(ci_m_inc(from), to, macro, __VA_ARGS__)		\
	)	
#define ____ci_m_call_up_to_lv2()						__ci_m_call_up_to_lv2
#define ci_m_call_up_to_lv2(from, to, macro, ...)		ci_m_expand_inf(__ci_m_call_up_to_lv2(from, to, macro, __VA_ARGS__))
#define ci_m_call_up_to_lv2_8(from, to, macro, ...)		ci_m_expand8(__ci_m_call_up_to_lv2(from, to, macro, __VA_ARGS__))


/* Level 3 macro, if you want to call ci_m_call_up_to_lv2() nested */
#define __ci_m_call_up_to_lv3(from, to, macro, ...)		\
	macro(from, __VA_ARGS__)			\
	\
	ci_m_if(ci_m_not_equal(from, to))	\
	(	\
		ci_m_defer2(____ci_m_call_up_to_lv3)()(ci_m_inc(from), to, macro, __VA_ARGS__)		\
	)	
#define ____ci_m_call_up_to_lv3()						__ci_m_call_up_to_lv3
#define ci_m_call_up_to_lv3(from, to, macro, ...)		ci_m_expand_inf(__ci_m_call_up_to_lv3(from, to, macro, __VA_ARGS__))


/* iterates [from, to] decrementally by invoke the macro */
#define __ci_m_call_down_to(from, to, macro, ...)		\
	macro(from, __VA_ARGS__)		\
	\
    ci_m_if(ci_m_not_equal(from, to)) \
    ( \
		ci_m_defer2(____ci_m_call_down_to)()(ci_m_dec(from), to, macro, __VA_ARGS__)		\
    )
#define ____ci_m_call_down_to()							__ci_m_call_down_to
#define ci_m_call_down_to(from, to, macro, ...)			ci_m_expand_inf(__ci_m_call_down_to(from, to, macro, __VA_ARGS__))


/* repeat "count" times, passing [0, count - 1] */
#define ci_m_call_repeat(count, macro, ...)				ci_m_call_up_to(0, ci_m_dec(count), macro, __VA_ARGS__)


/* repeat "count" times, and do simple duplication */
#define ci_m_dft_itor_copy(x, ...)						__VA_ARGS__
#define ci_m_repeat(count, ...)							ci_m_call_repeat(count, ci_m_dft_itor_copy, __VA_ARGS__)



