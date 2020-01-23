/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor.c					Master header for PAL XOR operations
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"
#include "pal_xor_intrin.h"
#include "rg/rg.h"

static int simd_features;
#define SIMD_AVX			0x0001
#define SIMD_AVX2			0x0002


static const u8 rg_gflog[RG_GF_ELM_NR] =
{	/* note: rg_gflog[0] is invlaid */
	0xFF, 0x00, 0x01, 0x19, 0x02, 0x32, 0x1A, 0xC6, 0x03, 0xDF, 0x33, 0xEE, 0x1B, 0x68, 0xC7, 0x4B,
	0x04, 0x64, 0xE0, 0x0E, 0x34, 0x8D, 0xEF, 0x81, 0x1C, 0xC1, 0x69, 0xF8, 0xC8, 0x08, 0x4C, 0x71,
	0x05, 0x8A, 0x65, 0x2F, 0xE1, 0x24, 0x0F, 0x21, 0x35, 0x93, 0x8E, 0xDA, 0xF0, 0x12, 0x82, 0x45,
	0x1D, 0xB5, 0xC2, 0x7D, 0x6A, 0x27, 0xF9, 0xB9, 0xC9, 0x9A, 0x09, 0x78, 0x4D, 0xE4, 0x72, 0xA6,
	0x06, 0xBF, 0x8B, 0x62, 0x66, 0xDD, 0x30, 0xFD, 0xE2, 0x98, 0x25, 0xB3, 0x10, 0x91, 0x22, 0x88,
	0x36, 0xD0, 0x94, 0xCE, 0x8F, 0x96, 0xDB, 0xBD, 0xF1, 0xD2, 0x13, 0x5C, 0x83, 0x38, 0x46, 0x40,
	0x1E, 0x42, 0xB6, 0xA3, 0xC3, 0x48, 0x7E, 0x6E, 0x6B, 0x3A, 0x28, 0x54, 0xFA, 0x85, 0xBA, 0x3D,
	0xCA, 0x5E, 0x9B, 0x9F, 0x0A, 0x15, 0x79, 0x2B, 0x4E, 0xD4, 0xE5, 0xAC, 0x73, 0xF3, 0xA7, 0x57,
	0x07, 0x70, 0xC0, 0xF7, 0x8C, 0x80, 0x63, 0x0D, 0x67, 0x4A, 0xDE, 0xED, 0x31, 0xC5, 0xFE, 0x18,
	0xE3, 0xA5, 0x99, 0x77, 0x26, 0xB8, 0xB4, 0x7C, 0x11, 0x44, 0x92, 0xD9, 0x23, 0x20, 0x89, 0x2E,
	0x37, 0x3F, 0xD1, 0x5B, 0x95, 0xBC, 0xCF, 0xCD, 0x90, 0x87, 0x97, 0xB2, 0xDC, 0xFC, 0xBE, 0x61,
	0xF2, 0x56, 0xD3, 0xAB, 0x14, 0x2A, 0x5D, 0x9E, 0x84, 0x3C, 0x39, 0x53, 0x47, 0x6D, 0x41, 0xA2,
	0x1F, 0x2D, 0x43, 0xD8, 0xB7, 0x7B, 0xA4, 0x76, 0xC4, 0x17, 0x49, 0xEC, 0x7F, 0x0C, 0x6F, 0xF6,
	0x6C, 0xA1, 0x3B, 0x52, 0x29, 0x9D, 0x55, 0xAA, 0xFB, 0x60, 0x86, 0xB1, 0xBB, 0xCC, 0x3E, 0x5A,
	0xCB, 0x59, 0x5F, 0xB0, 0x9C, 0xA9, 0xA0, 0x51, 0x0B, 0xF5, 0x16, 0xEB, 0x7A, 0x75, 0x2C, 0xD7,
	0x4F, 0xAE, 0xD5, 0xE9, 0xE6, 0xE7, 0xAD, 0xE8, 0x74, 0xD6, 0xF4, 0xEA, 0xA8, 0x50, 0x58, 0xAF
};

static const u8 rg_gfilog[RG_GF_ELM_NR] =
{	/* note: rg_gfilog[255] is invalid */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1D, 0x3A, 0x74, 0xE8, 0xCD, 0x87, 0x13, 0x26,
	0x4C, 0x98, 0x2D, 0x5A, 0xB4, 0x75, 0xEA, 0xC9, 0x8F, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0,
	0x9D, 0x27, 0x4E, 0x9C, 0x25, 0x4A, 0x94, 0x35, 0x6A, 0xD4, 0xB5, 0x77, 0xEE, 0xC1, 0x9F, 0x23,
	0x46, 0x8C, 0x05, 0x0A, 0x14, 0x28, 0x50, 0xA0, 0x5D, 0xBA, 0x69, 0xD2, 0xB9, 0x6F, 0xDE, 0xA1,
	0x5F, 0xBE, 0x61, 0xC2, 0x99, 0x2F, 0x5E, 0xBC, 0x65, 0xCA, 0x89, 0x0F, 0x1E, 0x3C, 0x78, 0xF0,
	0xFD, 0xE7, 0xD3, 0xBB, 0x6B, 0xD6, 0xB1, 0x7F, 0xFE, 0xE1, 0xDF, 0xA3, 0x5B, 0xB6, 0x71, 0xE2,
	0xD9, 0xAF, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88, 0x0D, 0x1A, 0x34, 0x68, 0xD0, 0xBD, 0x67, 0xCE,
	0x81, 0x1F, 0x3E, 0x7C, 0xF8, 0xED, 0xC7, 0x93, 0x3B, 0x76, 0xEC, 0xC5, 0x97, 0x33, 0x66, 0xCC,
	0x85, 0x17, 0x2E, 0x5C, 0xB8, 0x6D, 0xDA, 0xA9, 0x4F, 0x9E, 0x21, 0x42, 0x84, 0x15, 0x2A, 0x54,
	0xA8, 0x4D, 0x9A, 0x29, 0x52, 0xA4, 0x55, 0xAA, 0x49, 0x92, 0x39, 0x72, 0xE4, 0xD5, 0xB7, 0x73,
	0xE6, 0xD1, 0xBF, 0x63, 0xC6, 0x91, 0x3F, 0x7E, 0xFC, 0xE5, 0xD7, 0xB3, 0x7B, 0xF6, 0xF1, 0xFF,
	0xE3, 0xDB, 0xAB, 0x4B, 0x96, 0x31, 0x62, 0xC4, 0x95, 0x37, 0x6E, 0xDC, 0xA5, 0x57, 0xAE, 0x41,
	0x82, 0x19, 0x32, 0x64, 0xC8, 0x8D, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xDD, 0xA7, 0x53, 0xA6,
	0x51, 0xA2, 0x59, 0xB2, 0x79, 0xF2, 0xF9, 0xEF, 0xC3, 0x9B, 0x2B, 0x56, 0xAC, 0x45, 0x8A, 0x09,
	0x12, 0x24, 0x48, 0x90, 0x3D, 0x7A, 0xF4, 0xF5, 0xF7, 0xF3, 0xFB, 0xEB, 0xCB, 0x8B, 0x0B, 0x16,
	0x2C, 0x58, 0xB0, 0x7D, 0xFA, 0xE9, 0xCF, 0x83, 0x1B, 0x36, 0x6C, 0xD8, 0xAD, 0x47, 0x8E, 0x00
};

ci_alignas(64) u8 pal_gf_xor_table[RG_GF_ELM_NR][2][32];	/* [2]: table1, table2; [32] = index */

#ifdef PAL_XOR_512
#error Do: table1[j + 32], table1[j + 48]
#endif
static void init_gf_xor_table()
{
	u8 *table1, *table2;

	ci_loop(i, RG_GF_ELM_NR) {
		table1 = pal_gf_xor_table[i][0];
		table2 = pal_gf_xor_table[i][1];

		ci_loop(j, 16) {
			table1[j] = table1[j + 16] = rg_gf_mul(i, j);
			table2[j] = table2[j + 16] = rg_gf_mul(i, j << 4);
		}
	}
}

u8 rg_gf_mul(u8 a, u8 b)
{
	int sum;

	if (!a || !b) 
		return 0;
		
	if (( sum = rg_gflog[a] + rg_gflog[b]) >= RG_GF_ELM_NR - 1)
		sum -= RG_GF_ELM_NR - 1;

	ci_range_check(sum, 0, RG_GF_ELM_NR);
	return rg_gfilog[sum];
}

void pal_xor_p_safe(void **__buf, int nr, int size)
{
	u64 *p, *q, **buf = (u64 **)__buf;

	ci_align_check(size, ci_sizeof(*p));
	ci_range_check_i(nr, 3, RG_MAX_XOR_ELM);

	ci_memzero(buf[0], size);
	ci_loop(i, 1, nr) {
		p = buf[0];
		q = buf[i];

		ci_loop(j, size / ci_sizeof(u64))
			*p++ ^= *q++;
	}
}

void pal_xor_q_safe(void **__buf, u8 *coef, int nr, int size)
{
	u8 *p, *q, **buf = (u8 **)__buf;

	ci_align_check(size, ci_sizeof(*p));
	ci_range_check_i(nr, 3, RG_MAX_XOR_ELM);

	ci_memzero(buf[0], size);
	ci_loop(i, 1, nr) {
		p = buf[0];
		q = buf[i];

		ci_loop(j, size)
			*p++ ^= rg_gf_mul(coef[i], *q++);
	}
}

void pal_xor_pq_rot_safe(void **__buf, u8 *coef, int nr, int size)
{
	u8 *p, *q, *r, **buf = (u8 **)__buf;

	ci_align_check(size, ci_sizeof(*p));
	ci_range_check_i(nr, 4, RG_MAX_XOR_ELM);

	ci_memzero(buf[0], size);
	ci_memzero(buf[1], size);
	ci_loop(i, 2, nr) {
		p = buf[0];
		q = buf[1];
		r = buf[i];

		ci_loop(j, size) {
			*p ^= *r;
			*q ^= rg_gf_mul(coef[i], *r);
			p++, q++, r++;
		}
	}
}

/*	buf[0] is P, buf[1] is Q, buf[2] is P', buf[3] is Q', coef[0..1] are dummies */
void pal_xor_pq_rmw_safe(void **__buf, u8 *coef, int nr, int size)
{
	u8 *p, *q, *r, **buf = (u8 **)__buf;

	ci_align_check(size, ci_sizeof(*p));
	ci_range_check_i(nr, 4, RG_MAX_XOR_ELM);

	ci_memzero(buf[0], size);
	ci_memzero(buf[1], size);

	ci_loop(i, 2, nr) {	/* do P */
		if (i == 3)		/* skip buf[3], which is Q' */
			continue;

		p = buf[0];
		r = buf[i];

		ci_loop(j, size) {
			*p ^= *r;
			p++, r++;
		}
	}

	ci_loop(i, 3, nr) {	/* do Q */
		q = buf[1];
		r = buf[i];

		ci_loop(j, size) {
			*q ^= rg_gf_mul(coef[i], *r);
			q++, r++;
		}
	}
}

void pal_xor_qq_safe(void **__buf, u8 *coef0, u8 *coef1, int nr, int size)
{
	u8 *q0, *q1, *ptr, **buf = (u8 **)__buf;

	ci_align_check(size, ci_sizeof(*ptr));
	ci_range_check_i(nr, 4, RG_MAX_XOR_ELM);

	ci_memzero(buf[0], size);
	ci_memzero(buf[1], size);
	
	ci_loop(i, 2, nr) {
		q0 	= buf[0];
		q1 	= buf[1];
		ptr = buf[i];

		ci_loop(j, size) {
			*q0++ ^= rg_gf_mul(coef0[i], *ptr);
			*q1++ ^= rg_gf_mul(coef1[i], *ptr);
			ptr++;
		}
	}
}

static void pal_xor_check(void **buf, int nr, int size, int min_nr)
{
	ci_range_check_i(nr, min_nr, RG_MAX_XOR_ELM);
	ci_align_check(size, 8);		/* multiple of 8 */

#ifdef CI_DEBUG
	ci_loop(i, nr)
		ci_ptr_align_check(buf[i], 8);
#endif
}

static void pal_xor_slice(void **buf, int nr, int size, int *upper, int *body, int *lower)
{
	*lower = 0;
	*upper = (int)((u8 *)ci_ptr_align_upper(buf[0], PAL_CPU_CACHE_LINE_SIZE) - (u8 *)buf[0]);
	*upper = ci_min(*upper, size);

	if ((*body = size - *upper)) {
		*lower = (int)(*body & PAL_CPU_CACHE_LINE_OFFSET);
		*body -= *lower;
	}
}

void pal_xor_p(void **buf, int nr, int size)
{
	pal_xor_check(buf, nr, size, 3);

#ifdef PAL_XOR_SIMD
	int upper, body, lower;

	pal_xor_slice(buf, nr, size, &upper, &body, &lower);

	if ((upper))
		pal_xor_p_64(buf, nr, upper, 0);
	if ((body))
		simd_features & SIMD_AVX2 ? pal_xor_p_256(buf, nr, body, upper) : pal_xor_p_128(buf, nr, body, upper);
	if ((lower))
		pal_xor_p_64(buf, nr, lower, upper + body);
#else	/* !PAL_XOR_SIMD */
	pal_xor_p_safe(buf, nr, size);
#endif
}

void pal_xor_q(void **buf, u8 *coef, int nr, int size)
{
	pal_xor_check(buf, nr, size, 3);

#ifdef PAL_XOR_SIMD
	int upper, body, lower;

	pal_xor_slice(buf, nr, size, &upper, &body, &lower);

	if ((upper))
		pal_xor_q_64(buf, coef, nr, upper, 0);
	if ((body))
		simd_features & SIMD_AVX2 ? pal_xor_q_256(buf, coef, nr, body, upper) : pal_xor_q_128(buf, coef, nr, body, upper);
	if ((lower))
		pal_xor_q_64(buf, coef, nr, lower, upper + body);
#else	/* PAL_XOR_SIMD */
	pal_xor_q_safe(buf, coef, nr, size);
#endif
}

void pal_xor_pq_rot(void **buf, u8 *coef, int nr, int size)
{
	pal_xor_check(buf, nr, size, 4);	/* P, Q, D0, D1 */

#ifdef PAL_XOR_SIMD
	int upper, body, lower;

	pal_xor_slice(buf, nr, size, &upper, &body, &lower);

	if ((upper))
		pal_xor_pq_rot_64(buf, coef, nr, upper, 0);
	if ((body))
		simd_features & SIMD_AVX2 ?  pal_xor_pq_rot_256(buf, coef, nr, body, upper) : pal_xor_pq_rot_128(buf, coef, nr, body, upper);
	if ((lower))
		pal_xor_pq_rot_64(buf, coef, nr, lower, upper + body);
#else	/* PAL_XOR_SIMD */
	pal_xor_pq_rot_safe(buf, coef, nr, size);
#endif
}

void pal_xor_pq_rmw(void **buf, u8 *coef, int nr, int size)
{
	pal_xor_check(buf, nr, size, 6);		/* P, Q, P', Q', D0, D0' */

#ifdef PAL_XOR_SIMD
	int upper, body, lower;

	pal_xor_slice(buf, nr, size, &upper, &body, &lower);

	if ((upper))
		pal_xor_pq_rmw_64(buf, coef, nr, upper, 0);
	if ((body))
		simd_features & SIMD_AVX2 ? pal_xor_pq_rmw_256(buf, coef, nr, body, upper) : pal_xor_pq_rmw_128(buf, coef, nr, body, upper);
	if ((lower))
		pal_xor_pq_rmw_64(buf, coef, nr, lower, upper + body);
#else	/* PAL_XOR_SIMD */
	pal_xor_pq_rmw_safe(buf, coef, nr, size);
#endif
}

void pal_xor_qq(void **buf, u8 *coef0, u8 *coef1, int nr, int size)
{
	pal_xor_check(buf, nr, size, 4);		/* Q0, Q1, D0, D1 */

#ifdef PAL_XOR_SIMD
	int upper, body, lower;

	pal_xor_slice(buf, nr, size, &upper, &body, &lower);

	if ((upper))
		pal_xor_qq_64(buf, coef0, coef1, nr, upper, 0);
	if ((body))
		simd_features & SIMD_AVX2 ? pal_xor_qq_256(buf, coef0, coef1, nr, body, upper) : pal_xor_qq_128(buf, coef0, coef1, nr, body, upper);
	if ((lower))
		pal_xor_qq_64(buf, coef0, coef1, nr, lower, upper + body);
#else	/* PAL_XOR_SIMD */
	pal_xor_qq_safe(buf, coef0, coef1, nr, size);
#endif
}

int pal_xor_init()
{
	init_gf_xor_table();

#ifdef WIN_SIM
	simd_features = SIMD_AVX;
#elif defined(__GNUC__)
	__builtin_cpu_init();
	__builtin_cpu_supports("avx") && (simd_features |= SIMD_AVX);
	__builtin_cpu_supports("avx2") && (simd_features |= SIMD_AVX2);
#endif

	pal_imp_printf("xor: avx");
	ci_panic_if(!(simd_features & SIMD_AVX), "AVX Not Supported for this CPU");
	if (simd_features & SIMD_AVX2) 
		pal_printf(", avx2");
#ifdef PAL_XOR_NON_TEMPORAL
	pal_printf(", non_temporal");
#endif
	pal_printf("\n");

#ifndef PAL_XOR_SIMD
	pal_warn_printf("PAL_XOR_SIMD DISABLED\n");
#endif

	return 0;
}





/*
 *	C version of the AVX/AVX2 instructions
 */
#ifdef WIN_SIM
__m64 pal_c_simd_shuffle_64(__m128 __a, __m64 __b)		
{
	__m64 rv;

	u8 *a = (u8 *)&__a, *b = (u8 *)&__b, *c = (u8 *)&rv;

	ci_loop(i, 8) 
		c[i] = b[i] & 0x80 ? 0 : a[b[i]];

	return rv;
}

__m128 pal_c_simd_shuffle_128(__m128 __a, __m128 __b)
{
	__m128 rv;

	u8 *a = (u8 *)&__a, *b = (u8 *)&__b, *c = (u8 *)&rv;

	ci_loop(i, 16) 
		c[i] = b[i] & 0x80 ? 0 : a[b[i]];

	return rv;
}

__m256 pal_c_simd_shuffle_256(__m256 __a, __m256 __b)
{
/*
 *	The Intel's Pseudo code seems to be WRONG
 *	https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm256_shuffle_epi8&expand=5074,4702
 */
	__m256 rv;

	u8 *a = (u8 *)&__a, *b = (u8 *)&__b, *c = (u8 *)&rv;

	ci_loop(i, 32) 
		c[i] = b[i] & 0x80 ? 0 : a[b[i]];

	return rv;
}

__m64 pal_c_simd_rshift_64(__m64 __a, int shift)
{
	__m64_128_t v;
	v.f64 = __a;
	v.i128 = _mm_srli_epi64(v.i128, shift);

	return v.f64;
}

__m128 pal_c_simd_rshift_128(__m128 __a, int shift)
{
	return (__m128)_mm_srli_epi64((__m128i)__a, shift);
}

__m256 pal_c_simd_rshift_256(__m256 __a, int shift)
{
	__m256 rv;

	u8 *a = (u8 *)&__a, *c = (u8 *)&rv;

	ci_loop(i, 32)
		c[i] = a[i] >> shift;

	return rv;
}
#endif


