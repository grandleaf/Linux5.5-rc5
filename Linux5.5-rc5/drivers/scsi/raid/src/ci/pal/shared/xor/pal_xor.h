/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_xor.h					Master header for PAL XOR operations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal_cfg.h"
#include "pal_type.h"
#include "rg/rg_cfg.h"


void pal_xor_p_safe(void **buf, int nr, int size);							/* slow but guaranteed correct, verification purpose */
void pal_xor_p(void **buf, int nr, int size);								/* buf[0] is P, buf[1..nr) are data, size is buffer length */
void pal_xor_p_64(void **buf, int nr, int size, int offset);				/* uses C (8-bytes) for parity calculation */
void pal_xor_p_128(void **buf, int nr, int size, int offset);				/* uses SIMD 128 for parity calculation */
void pal_xor_p_256(void **buf, int nr, int size, int offset);				/* uses SIMD 256 for parity calculation */

void pal_xor_q_safe(void **buf, u8 *coef, int nr, int size);				/* gf-xor, slow but correct */
void pal_xor_q(void **buf, u8 *coef, int nr, int size);						/* buf[0] is Q, buf[1..nr) are data, size is buffer length */
void pal_xor_q_64(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_q_128(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_q_256(void **buf, u8 *coef, int nr, int size, int offset);			

/*	
 *	buf[0] is P, buf[1] is Q, coef[0..1] are dummies
 *	read others/full stripe write
 *	buf:	P, Q, D0, D1, D2, D3, ...
 *	coef:	X, X, C0, C1, C2, C3, ...
 */
void pal_xor_pq_rot_safe(void **buf, u8 *coef, int nr, int size);				
void pal_xor_pq_rot(void **buf, u8 *coef, int nr, int size);					
void pal_xor_pq_rot_64(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_pq_rot_128(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_pq_rot_256(void **buf, u8 *coef, int nr, int size, int offset);	

/*	
 *	buf[0] is P, buf[1] is Q, buf[2] is P', buf[3] is Q', coef[0..1] are dummies
 *	read modify write/small random write
 *	buf:	P, Q, P', Q', D0, D1, D2, D3, ...
 *	coef:	X, X, 1,  1,  C0, C1, C2, C3, ...
 */
void pal_xor_pq_rmw_safe(void **buf, u8 *coef, int nr, int size);				
void pal_xor_pq_rmw(void **buf, u8 *coef, int nr, int size);					
void pal_xor_pq_rmw_64(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_pq_rmw_128(void **buf, u8 *coef, int nr, int size, int offset);			
void pal_xor_pq_rmw_256(void **buf, u8 *coef, int nr, int size, int offset);	

/*	
 *	buf[0] is Q0, buf[1] is Q1, coef[0..1] are dummies
 *	RAID 6 2-disk fail recovery
 *	buf:	Q0, Q1, D0, D1, D2, D3, ...
 *	coef:	X,  X,  C0, C1, C2, C3, ...
 */
void pal_xor_qq_safe(void **buf, u8 *coef0, u8 *coef1, int nr, int size);				
void pal_xor_qq(void **buf, u8 *coef0, u8 *coef1, int nr, int size);					
void pal_xor_qq_64(void **buf, u8 *coef0, u8 *coef1, int nr, int size, int offset);			
void pal_xor_qq_128(void **buf, u8 *coef0, u8 *coef1, int nr, int size, int offset);			
void pal_xor_qq_256(void **buf, u8 *coef0, u8 *coef1, int nr, int size, int offset);

int  pal_xor_init();


/* put to somewhere else later */
#define RG_GF_ELM_NR		256

#ifdef PAL_XOR_512
#error Do: pal_gf_xor_table[RG_GF_ELM_NR][2][64] 
#endif

extern u8 pal_gf_xor_table[RG_GF_ELM_NR][2][32];	/* [2]: table1, table2; [16] = 0x00~0xFF */

u8   rg_gf_mul(u8 a, u8 b);		// will move to somewhere else later

#ifdef WIN_SIM
/* clang's support for AVX/AVX2 has problem, or maybe it's Microsoft's issue */
__m64  pal_c_simd_shuffle_64(__m128 __a, __m64 __b);
__m128 pal_c_simd_shuffle_128(__m128 __a, __m128 __b);
__m256 pal_c_simd_shuffle_256(__m256 __a, __m256 __b);

__m64  pal_c_simd_rshift_64(__m64 __a, int shift);
__m128 pal_c_simd_rshift_128(__m128 __a, int shift);
__m256 pal_c_simd_rshift_256(__m256 __a, int shift);
#endif
