#include "ci.h"
//#include "pal_xor_intrin.h"



static void gf_128_test();
//static void gf_256_test();



static void print_array(char *name, u8 *ary)
{
	ci_printf("%8s: ", name);
	ci_loop(i, 16)
		ci_printf("%02X ", ary[i]);
	ci_printf("\n");
}

static void print_array32(char *name, u8 *ary)
{
	ci_printf("%8s: ", name);
	ci_loop(i, 32)
		ci_printf("%02X ", ary[i]);
	ci_printf("\n");
}

static void and_array(u8 *dst, u8 *src0, u8 *src1)
{
	ci_loop(i, 16)
		dst[i] = src0[i] & src1[i]; 
}

static void xor_array(u8 *dst, u8 *src0, u8 *src1)
{
	ci_loop(i, 16)
		dst[i] = src0[i] ^ src1[i]; 
}

static void shuffle_array(u8 *dst, u8 *src, u8 *table)
{
	ci_loop(i, 16) 
		dst[i] = table[src[i]];
}

static void right_shift_array(u8 *ary, int shift)
{
	ci_loop(i, 16)
		ary[i] >>= shift;
}

void test_gf_xor_dev()
{
	ci_here();


	u8 y = 7;
	u8 idx[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

	u8 A[16] = { 0x39, 0x1D, 0x9F, 0x5A, 0xAA, 0xAB, 0x15, 0xC3, 0x63, 0xE0, 0x7C, 0x43, 0xFB, 0x83, 0x16, 0x23 };
	u8 l[16], h[16], yA[16], chk[16];

	u8 table1[16], table2[16];
	u8 mask1[16] = { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F };
	u8 mask2[16] = { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 };

	print_array("idx", idx);
	ci_loop(60)
		ci_printf("-");
	ci_printf("\n");

	ci_loop(i, 16) 
		table1[i] = rg_gf_mul(y, i);
	print_array("table1", table1);

	ci_loop(i, 16) 
		table2[i] = rg_gf_mul(y, i << 4);
	print_array("table2", table2);

	print_array("mask1", mask1);
	print_array("mask2", mask2);
	print_array("A", A);

	and_array(l, A, mask1);
	print_array("l", l);

	shuffle_array(l, l, table1);
	print_array("l", l);

	and_array(h, A, mask2);
	print_array("h", h);

	right_shift_array(h, 4);
	print_array("h", h);

	shuffle_array(h, h, table2);
	print_array("h", h);

	xor_array(yA, h, l);
	print_array("yA", yA);

	ci_loop(60)
		ci_printf("-");
	ci_printf("\n");

	ci_loop(i, 16)
		chk[i] = rg_gf_mul(y, A[i]);
	print_array("chk", chk);

	ci_printf("\n\n\n\n");
	gf_128_test();

//	ci_printf("\n\n\n\n");
//	gf_256_test();
}

#define print__m128(name, var)		print_array(name, (u8 *)&(var))

#ifdef WIN_SIM
static __m128i simd_shuffle(__m128i __a, __m128i __b)
{
	__m128i rv;

	u8 *a = (u8 *)&__a, *b = (u8 *)&__b, *c = (u8 *)&rv;

	ci_loop(i, 16) 
		c[i] = b[i] & 0x80 ? 0 : a[b[i]];

	return rv;
}
#elif defined(__GNUC__)
#define simd_shuffle(a, b)		_mm_shuffle_epi8(a, b)
#endif

static void gf_128_test()
{
	__m128i A, mask1, mask2, l, h, yA, table1, table2;

	u8 y = 7;
	u8 chk[16];
	u8 idx[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
	u8 __A[16] = { 0x39, 0x1D, 0x9F, 0x5A, 0xAA, 0xAB, 0x15, 0xC3, 0x63, 0xE0, 0x7C, 0x43, 0xFB, 0x83, 0x16, 0x23 };

	print_array("idx", idx);
	ci_loop(60)
		ci_printf("-");
	ci_printf("\n");

	ci_memcpy(&table1, pal_gf_xor_table[y][0], ci_sizeof(table1));
	print__m128("table1", table1);

	ci_memcpy(&table2, pal_gf_xor_table[y][1], ci_sizeof(table2));
	print__m128("table2", table2);

	mask1 = _mm_set1_epi8(0x0F);
	print__m128("mask1", mask1);

	mask2 = _mm_set1_epi8(0xF0);
	print__m128("mask2", mask2);

	ci_memcpy(&A, __A, ci_sizeof(A));
	print__m128("A", A);

	l = _mm_and_si128(A, mask1);
	print__m128("l", l);

//	l = _mm_shuffle_epi8(table1, l);
	l = simd_shuffle(table1, l);
	print__m128("l", l);

	h = _mm_and_si128(A, mask2);
	print__m128("h", h);

	h = _mm_srli_epi64(h, 4);
	print__m128("h", h);

//	h = _mm_shuffle_epi8(table2, h);
	h = simd_shuffle(table2, h);
	print__m128("h", h);

	yA = _mm_xor_si128(h, l);
	print__m128("yA", yA);


	// CHECKING
	ci_loop(60)
		ci_printf("-");
	ci_printf("\n");

	ci_loop(i, 16)
		chk[i] = rg_gf_mul(y, __A[i]);
	print_array("chk", chk);
}

#if 0
#define print__m256(name, var)		print_array32(name, (u8 *)&(var))

static void gf_256_test()
{
	__m256i A, mask1, mask2, l, h, yA, table1, table2;

	u8 y = 7;
	u8 chk[32];
	u8 idx[32];
	u8 __A[32] = { 0x39, 0x1D, 0x9F, 0x5A, 0xAA, 0xAB, 0x15, 0xC3, 0x63, 0xE0, 0x7C, 0x43, 0xFB, 0x83, 0x16, 0x23, 
				   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

	ci_loop(i, 32) idx[i] = i;
	print_array32("idx", idx);
	ci_loop(110)
		ci_printf("-");
	ci_printf("\n");

	ci_memcpy(&table1, pal_gf_xor_table[y][0], ci_sizeof(table1));
	print__m256("table1", table1);

	ci_memcpy(&table2, pal_gf_xor_table[y][1], ci_sizeof(table2));
	print__m256("table2", table2);

	mask1 = _mm256_set1_epi8(0x0F);
	print__m256("mask1", mask1);

	mask2 = _mm256_set1_epi8(0xF0);
	print__m256("mask2", mask2);

	ci_memcpy(&A, __A, ci_sizeof(A));
	print__m256("A", A);

#if 0
	l = _mm256_and_si256(A, mask1);
	print__m128("l", l);

//	l = _mm_shuffle_epi8(table1, l);
	l = simd_shuffle(table1, l);
	print__m128("l", l);

	h = _mm_and_si128(A, mask2);
	print__m128("h", h);

	h = _mm_srli_epi64(h, 4);
	print__m128("h", h);

//	h = _mm_shuffle_epi8(table2, h);
	h = simd_shuffle(table2, h);
	print__m128("h", h);

	yA = _mm_xor_si128(h, l);
	print__m128("yA", yA);
#endif

	// CHECKING
	ci_loop(110)
		ci_printf("-");
	ci_printf("\n");

	ci_loop(i, 32)
		chk[i] = rg_gf_mul(y, __A[i]);
	print_array32("chk", chk);
}

#endif





















