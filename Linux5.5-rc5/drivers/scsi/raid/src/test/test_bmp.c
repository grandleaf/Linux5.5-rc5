#include "ci.h"

#define STRIP_BITS				255

ci_bmp_def(rg_strip_map, STRIP_BITS);

static void u32_test();
static void u64_test();
static void op_test();
static void rg_map_test();
static void bmp_dump_test();
static void combo_test();
static void stress_test();

static void scratch_pad()
{
	ci_bmp_t bmp[2];
	bmp[0] = 0;
	bmp[1] = 1;
	int result = ci_bmp_first_set(&bmp, 65);
	ci_printf("result=%d\n", result);
}

void test_bmp()
{
	ci_here();

	ci_printf("ci_bmp_small_const_bits = %d\n", __ci_bmp_small_const_bits(STRIP_BITS));

//	scratch_pad();

//	bmp_dump_test();
//	combo_test();
//	rg_map_test();
//	op_test();
	stress_test();
	

/*
	u64_test();

	ci_printf("sizeof rg_strip_map=%d\n", ci_sizeof(rg_strip_map_t));

	u32 val = 0x80000000;
	int from = 4, to = 8;

	ci_printf("u32_first_set(%#X)=%d\n", val, u32_first_set(val));
	ci_printf("u32_last_set(%#X)=%d\n", val, u32_last_set(val));
	ci_printf("u32_count_set(%#X)=%d\n", val, u32_count_set(val));
	ci_printf("u32_mask(%d, %d)=%#X\n", from, to, u32_mask(from, to));
	
	int start, end;
	u32_get_range_clear(val, 0, start, end);

	ci_printf("start=%d, end=%d\n", start, end);

	ci_bmp_t bmp_mask = ci_bmp_elm_mask(3, 4);
	ci_printf("bmp_mask =%#X\n", bmp_mask);
 */
}

static void u32_test()
{
	u32 val;

	int from, to, from2, to2;

	for (;;) {
		from = ci_rand_shr(0, 32 - 1);
		to = ci_rand_shr(from + 1, 32);

		val = 0;
		u32_set_range_c(val, from, to);
		u32_get_range_set(val, 0, from2, to2);

		ci_assert(from == from2);
		ci_assert(to == to2);
		ci_printf("[ %02d, %02d ), [ %02d, %02d ), val=%#08X\n", from, to, from2, to2, val);
	}
}

static void u64_test()
{
	u64 val;

	int from, to, from2, to2;

	for (;;) {
		from = ci_rand_shr(0, 64 - 1);
		to = ci_rand_shr(from + 1, 64);

		val = 0;
		u64_set_range_c(val, from, to);
		u64_get_range_set(val, 0, from2, to2);

		ci_assert(from == from2);
		ci_assert(to == to2);
		ci_printf("[ %02d, %02d ), [ %02d, %02d ), val=%#016llX\n", from, to, from2, to2, val);
	}
}

static void bmp_dump_test()
{
#define MAX_DUMP_BITMAP			128
	ci_bmp_t dump_map[ci_bits_to_bmp(MAX_DUMP_BITMAP)];

	ci_loop(t, 2) 
		ci_loop_i(i, 1, MAX_DUMP_BITMAP) {
			ci_bmp_zero(&dump_map, MAX_DUMP_BITMAP);
			ci_bmp_fill(&dump_map, i);
			ci_printf("%04d -> ", i);
			t ? ci_bmp_bin_dump(&dump_map, i) : ci_bmp_dump(&dump_map, i); 
			ci_printf("\n");
		}
}

static void op_test()
{
	rg_strip_map_t map1 = {{ 0xFF000000 }};
	rg_strip_map_t map2 = {{ 0x07 }};

	ci_bmp_bin_dump(&map2, STRIP_BITS);
	ci_printf("\n\n\n\n");


	ci_bmp_dump(&map1, STRIP_BITS);
	ci_printf("\n");

	ci_bmp_dump(&map2, STRIP_BITS);
	ci_printf("\n");

	rg_strip_map_or_asg(&map1, &map2);

	ci_bmp_dump(&map1, STRIP_BITS);
	ci_printf("\n");

	rg_strip_map_not_asg(&map1);
	ci_bmp_dump(&map1, STRIP_BITS);
	ci_printf("\n");

	ci_printf("mask=%#llX\n", CI_BMP_MASK);

	rg_strip_map_t map3;
	rg_strip_map_mask(&map3, 7, 8);
	ci_bmp_dump(&map3, STRIP_BITS);
	ci_printf("\n");

	ci_printf("%d\n", ci_bmp_bit_is_set(&map3, 255, 7));

	ci_bmp_set_bit(&map3, 255, 8);
	rg_strip_map_dump(&map3);
	ci_printf("\n");

	ci_loop(i, STRIP_BITS - 2) {
		rg_strip_map_zero(&map3);
		rg_strip_map_set_bit(&map3, i);
		rg_strip_map_set_bit(&map3, i + 2);
		int first_set = rg_strip_map_first_set(&map3);
		ci_printf("first set=%d\n", first_set);
		int next_set = rg_strip_map_next_set(&map3, first_set + 2);
		ci_printf("next_set=%d\n", next_set);
		ci_assert(first_set == i);
		ci_assert(next_set == i + 2);
	}
}

static void rg_map_basic_test()
{
	rg_strip_map_t map1 = {{ 0xAAAAAAAA, 0xFFFFFFFF }};
	rg_strip_map_t map2 = {{ 0x555555, 0xAA }};
	rg_strip_map_t map3;

	ci_printf("*** STRIP_BITS=%d\n", STRIP_BITS);

	ci_printf("map1: "); rg_strip_map_bin_dump(&map1); ci_printf("\n");
	ci_printf("map2: "); rg_strip_map_bin_dump(&map2); ci_printf("\n");

	ci_printf("map1: "); rg_strip_map_dump(&map1); ci_printf("\n");
	ci_printf("map2: "); rg_strip_map_dump(&map2); ci_printf("\n");

	rg_strip_map_copy(&map3, &map1);
	ci_printf("copy map1->map3: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_zero(&map3);
	ci_printf("zero: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_fill(&map3);
	ci_printf("fill: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_not(&map3, &map1);
	ci_printf("!map1: "); rg_strip_map_dump(&map3); ci_printf("\n");
	rg_strip_map_not_asg(&map3);
	ci_printf("!!map1: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_and(&map3, &map1, &map2);
	ci_printf("map1 & map2: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_or(&map3, &map1, &map2);
	ci_printf("map1 | map2: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_xor(&map3, &map1, &map2);
	ci_printf("map1 ^ map2: "); rg_strip_map_dump(&map3); ci_printf("\n");

	rg_strip_map_sub(&map3, &map1, &map2);
	ci_printf("map1 - map2: "); rg_strip_map_dump(&map3); ci_printf("\n");

	ci_loop(start, STRIP_BITS)
		ci_loop_i(end, start + 1, STRIP_BITS) {
			rg_strip_map_mask(&map3, start, end);
			ci_printf("[%04d, %04d) ", start, end); 
			rg_strip_map_bin_dump(&map3); 
			ci_printf("\n");
		}
}

static void rg_map_set_test()
{
	u64 counter = 0;
	int idx1, idx2;
	rg_strip_map_t map1;

	rg_strip_map_zero(&map1);
	rg_strip_map_dump(&map1); ci_printf("\n");
	rg_strip_map_bin_dump(&map1); ci_printf("\n");

	for (;;) {
		idx1 = ci_rand_shr(STRIP_BITS);

		rg_strip_map_set_bit(&map1, idx1);
		ci_assert(rg_strip_map_bit_is_set(&map1, idx1));
		ci_assert(rg_strip_map_first_set(&map1) == idx1);
		ci_assert(rg_strip_map_last_set(&map1) == idx1);
		ci_assert(rg_strip_map_count_set(&map1) == 1);
		ci_assert(rg_strip_map_count_clear(&map1) == STRIP_BITS - 1);

		if (idx1 >= STRIP_BITS - 1)
			goto __next;
		idx2 = ci_rand_shr(idx1, STRIP_BITS);
		if (idx1 == idx2)
			goto __next;

		rg_strip_map_set_bit(&map1, idx2);
		ci_assert(rg_strip_map_bit_is_set(&map1, idx2));
		ci_assert(rg_strip_map_first_set(&map1) == idx1);
		ci_assert(rg_strip_map_last_set(&map1) == idx2);
		ci_assert(rg_strip_map_count_set(&map1) == 2);
		ci_assert(rg_strip_map_count_clear(&map1) == STRIP_BITS - 2);

		ci_assert(rg_strip_map_next_set(&map1, idx1 + 1) == idx2);
		ci_assert(rg_strip_map_prev_set(&map1, idx2 - 1) == idx1);
		ci_assert(rg_strip_map_prev_set(&map1, idx1 - 1) < 0);
		ci_assert(rg_strip_map_next_set(&map1, idx2 + 1) < 0);

//		ci_printf("%03d, %03d, ", idx1, idx2); rg_strip_map_dump(&map1); ci_printf("\n");
		rg_strip_map_clear_bit(&map1, idx2);

__next:
		rg_strip_map_clear_bit(&map1, idx1);

		if (!(++counter % 100000))
			ci_printf("%llu\n", counter);
	}
}

static void rg_map_clear_test()
{
	u64 counter = 0;
	int idx1, idx2;
	rg_strip_map_t map1;

	rg_strip_map_fill(&map1);
	rg_strip_map_dump(&map1); ci_printf("\n");
	rg_strip_map_bin_dump(&map1); ci_printf("\n");

	for (;;) {
		idx1 = ci_rand_shr(STRIP_BITS);

		rg_strip_map_clear_bit(&map1, idx1);
		ci_assert(rg_strip_map_bit_is_clear(&map1, idx1));
		ci_assert(rg_strip_map_first_clear(&map1) == idx1);
		ci_assert(rg_strip_map_last_clear(&map1) == idx1);
		ci_assert(rg_strip_map_count_clear(&map1) == 1);
		ci_assert(rg_strip_map_count_set(&map1) == STRIP_BITS - 1);

		if (idx1 >= STRIP_BITS - 1)
			goto __next;
		idx2 = ci_rand_shr(idx1, STRIP_BITS);
		if (idx1 == idx2)
			goto __next;

		rg_strip_map_clear_bit(&map1, idx2);
		ci_assert(rg_strip_map_bit_is_clear(&map1, idx2));
		ci_assert(rg_strip_map_first_clear(&map1) == idx1);
		ci_assert(rg_strip_map_last_clear(&map1) == idx2);
		ci_assert(rg_strip_map_count_clear(&map1) == 2);
		ci_assert(rg_strip_map_count_set(&map1) == STRIP_BITS - 2);

		ci_assert(rg_strip_map_next_clear(&map1, idx1 + 1) == idx2);
		ci_assert(rg_strip_map_prev_clear(&map1, idx2 - 1) == idx1);
		ci_assert(rg_strip_map_prev_clear(&map1, idx1 - 1) < 0);
		ci_assert(rg_strip_map_next_clear(&map1, idx2 + 1) < 0);

//		ci_printf("%03d, %03d, ", idx1, idx2); rg_strip_map_dump(&map1); ci_printf("\n");
		rg_strip_map_set_bit(&map1, idx2);

__next:
		rg_strip_map_set_bit(&map1, idx1);

		if (!(++counter % 100000))
			ci_printf("%llu\n", counter);
	}
}

static void rg_map_test()
{
	rg_map_basic_test();
//	rg_map_set_test();
//	rg_map_clear_test();
}





static void combo_test()
{
#define COMBO_BITS				32
	static ci_bmp_t map[ci_bits_to_bmp(COMBO_BITS)];

#if 0
	ci_loop(i, COMBO_BITS) {
		ci_bmp_combo_init(&map, COMBO_BITS, i);
		ci_bmp_dump(&map, COMBO_BITS); ci_printf("\n");
		ci_bmp_bin_dump(&map, COMBO_BITS); ci_printf("\n");
	}
#endif

	ci_loop_i(sets, 0, COMBO_BITS) {
//	ci_loop_i(sets, 3, 3) {
		u64 total = 0;

		ci_printf("Working on C(%d, %d) ...\n", COMBO_BITS, sets); 

		ci_bmp_combo(&map, COMBO_BITS, sets, {
			ci_bmp_bin_dump(&map, COMBO_BITS); ci_printf("\n");
//			ci_bmp_each_clear(&map, COMBO_BITS, s) ci_printf("%d, ", s); ci_printf("\n");

			int ss, ee;
			if (ci_bmp_first_range(&map, COMBO_BITS, ss, ee) >= 0)
				ci_printf("1st [%d, %d)\n", ss, ee);

			ci_bmp_each_range(&map, COMBO_BITS, s, e, {
				ci_printf("[%d, %d) ", s, e);
			});

			ci_printf("\n");
			total++;
		});

		ci_printf("Total=%llu, expect=%llu\n\n", total, ci_combination(COMBO_BITS, sets));
//		ci_assert(total == ci_combination(COMBO_BITS, sets));
	}
}

#define STRESS_MAX_BITS				5555
static u8 u8_map[STRESS_MAX_BITS];



static void stress_test()
{
	int count = 0;
	ci_bmp_t map[ci_bits_to_bmp(STRESS_MAX_BITS)];
	ci_bmp_zero(&map, STRESS_MAX_BITS);

	for (;;) {
		int len = ci_rand_shr_i(1, STRESS_MAX_BITS);
		ci_bmp_zero(&map, len);
		ci_memzero(u8_map, len);


		ci_loop(loop, 1000000) {
			int op = ci_rand_shr(100);

			switch (op) {
				case 0:		// ci_bmp_zero
					if (ci_rand_shr(500) == 0) {
						ci_memzero(u8_map, len);
						ci_bmp_zero(&map, len);
					}
					break;
				case 1:		// ci_bmp_fill
					if (ci_rand_shr(500) == 0) {
						ci_memset(u8_map, 1, len);
						ci_bmp_fill(&map, len);
					}
					break;
				case 2: {
					int rand_bit = ci_rand_shr(0, len);
					u8_map[rand_bit] = 1;
					ci_bmp_set_bit(&map, len, rand_bit);
					break;
				}
				case 3: {
					int rand_bit = ci_rand_shr(0, len);
					u8_map[rand_bit] = 0;
					ci_bmp_clear_bit(&map, len, rand_bit);
					break;
				}
				case 4: {		// first_set
					int first_set = ci_bmp_first_set(&map, len);
					int u8_first_set = -1;
					ci_loop(i, len)
						if (u8_map[i]) {
							u8_first_set = i;
							break;
						}
					ci_panic_unless(first_set == u8_first_set);
					continue;
				}
				case 5: {		// last_set
					int last_set = ci_bmp_last_set(&map, len);
					int u8_last_set = -1;
					ci_loop(i, len - 1, -1, -1)
						if (u8_map[i]) {
							u8_last_set = i;
							break;
						}
					ci_panic_unless(last_set == u8_last_set);
					continue;
				}

				case 7: {		// first_clear
					int first_clear = ci_bmp_first_clear(&map, len);
					int u8_first_clear = -1;
					ci_loop(i, len)
						if (!u8_map[i]) {
							u8_first_clear = i;
							break;
						}
					ci_panic_unless(first_clear == u8_first_clear);
					continue;
				}
				case 8: {		// last_clear
					int last_clear = ci_bmp_last_clear(&map, len);
					int u8_last_clear = -1;
					ci_loop(i, len - 1, -1, -1)
						if (!u8_map[i]) {
							u8_last_clear = i;
							break;
						}
					ci_panic_unless(last_clear == u8_last_clear);
					continue;
				}
				
				case 9: {		// next_set
					int from = ci_rand_shr(0, len);
					int next_set = ci_bmp_next_set(&map, len, from);
					int u8_next_set = -1;
					ci_loop(i, from, len)
						if (u8_map[i]) {
							u8_next_set = i;
							break;
						}
					ci_panic_unless(next_set == u8_next_set);
					continue;
				}
				case 10: {		// next_clear
					int from = ci_rand_shr(0, len);
					int next_clear = ci_bmp_next_clear(&map, len, from);
					int u8_next_clear = -1;
					ci_loop(i, from, len)
						if (!u8_map[i]) {
							u8_next_clear = i;
							break;
						}
					ci_panic_unless(next_clear == u8_next_clear);
					continue;
				}

				case 11: {		// prev_set
					int from = ci_rand_shr(0, len);
					int prev_set = ci_bmp_prev_set(&map, len, from);
					int u8_prev_set = -1;
					ci_loop(i, from, -1, -1)
						if (u8_map[i]) {
							u8_prev_set = i;
							break;
						}
					ci_panic_unless(prev_set == u8_prev_set);
					continue;
				}
				case 12: {		// prev_clear
					int from = ci_rand_shr(0, len);
					int prev_clear = ci_bmp_prev_clear(&map, len, from);
					int u8_prev_clear = -1;
					ci_loop(i, from, -1, -1)
						if (!u8_map[i]) {
							u8_prev_clear = i;
							break;
						}
					ci_panic_unless(prev_clear == u8_prev_clear);
					continue;
				}

				case 50: {	// ci_bmp_count_set
					int count = ci_bmp_count_set(&map, len);
					int u8_count = 0;

					ci_loop(i, len)
						if (u8_map[i])
							u8_count++;

					ci_panic_unless(u8_count == count);

					int clear_count = ci_bmp_count_clear(&map, len);
					ci_panic_unless(clear_count + count == len);
				}

				case 51: {		// ci_bmp_mask
					if (ci_rand_shr(300) != 0) 
						continue;

					int from = ci_rand_shr(len);
					int to = ci_rand_shr_i(from + 1, len);
					ci_bmp_zero(&map, len);
					ci_memzero(u8_map, len);
					ci_bmp_mask(&map, len, from, to);

					for (int i = from; i < to; i++)
						u8_map[i] = 1;

					break;		// check out of the switch
				}

				case 52: {		// ci_bmp_set_range
					if (ci_rand_shr(300) != 0) 
						continue;

					int from = ci_rand_shr(len);
					int to = ci_rand_shr_i(from + 1, len);

					ci_bmp_set_range(&map, len, from, to);
					for (int i = from; i < to; i++)
						u8_map[i] = 1;

					break;		// check out of the switch
				}

				case 53: {		// ci_bmp_clear_range
					if (ci_rand_shr(300) != 0) 
						continue;

					int from = ci_rand_shr(len);
					int to = ci_rand_shr_i(from + 1, len);

					ci_bmp_clear_range(&map, len, from, to);
					for (int i = from; i < to; i++)
						u8_map[i] = 0;

					break;		// check out of the switch
				}

				default: {
					int miscompare = 0, rand_bit = ci_rand_shr(0, len);
					if ((u8_map[rand_bit] && ci_bmp_bit_is_clear(&map, len, rand_bit)) ||
						(!u8_map[rand_bit] && ci_bmp_bit_is_set(&map, len, rand_bit)))
						miscompare = 1;

					ci_panic_if(miscompare);
					continue;
				}
			}

			ci_loop(i, len) {
				int miscompare = 0;

				if ((u8_map[i] && ci_bmp_bit_is_clear(&map, len, i)) ||
					(!u8_map[i] && ci_bmp_bit_is_set(&map, len, i)))
					miscompare = 1;

				if (miscompare) {
					ci_printf("i=%d, len=%d, op=%d, u8_map[]=%d, bit_is[%s]\n", i, len, op, u8_map[i], ci_bmp_bit_is_set(&map, len, i) ? "SET" : "CLEAR");
					ci_bmp_dump(&map, len);
					ci_printf("\n");
					ci_bmp_bin_dump(&map, len);
					ci_printf("\n");
					ci_panic("miscompare");
				}
			}
		}

#ifndef WIN_SIM
		ci_printf("%p, count=%d\n", (void *)ci_thread_current(), count++);
#elif defined(__GNUC__)
		ci_printf("%p, count=%d\n", ci_thread_current(), count++);
#endif
	}
}

