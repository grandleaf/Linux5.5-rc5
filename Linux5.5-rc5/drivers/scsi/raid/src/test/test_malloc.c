#include "ci.h"

static void halloc_test()
{
	u8 *first = ci_shr_halloc(11300, 0,  "first");
	ci_shr_halloc(25536, 4096, "second");
	ci_shr_halloc(33234, 0,  "third");
	ci_shr_halloc(1048576 * 3 + 231, 0,  "forth");
	ci_shr_halloc(-1, 0,  "buf_seg");
//	ci_shr_halloc(25536, 4096, "second");

	ci_memzero(first, 11300);

	ci_shr_halloc_dump();

	ci_node_each(node, {
		ci_loop(10)
			ci_node_halloc(node->node_id, ci_rand_shr(1333, 54444), 0, "node_xx_first");
	});

	ci_node_each(node, {
		ci_node_halloc_dump(node->node_id);
	});
}

static void balloc_stress_test()
{
#define BA_CHECK_DATA
#define BA_TEST_LOOP			(1000000ull * 1000000 / 100000)
#define BA_MEM_SIZE				(ci_mib(128) + ci_mib(128) / 64 + 64)
#define BA_NR_POINTER			512
#define BA_MIN_ALLOC_SIZE		1
#define BA_MAX_ALLOC_SIZE		ci_kib(750)
#define BA_ALLOC_PERCENTAGE		50		/* d+% */

	ci_balloc_t *ba = (ci_balloc_t *)pal_malloc(ci_sizeof(*ba));
	u8 *ptr = pal_malloc(BA_MEM_SIZE);
	u8 **vector = (u8 **)pal_malloc(ci_sizeof(u8 *) * BA_NR_POINTER);
	u8 *pattern = (u8 *)pal_malloc(ci_sizeof(u8) * BA_NR_POINTER);
	int *alloc_size = (int *)pal_malloc(ci_sizeof(int) * BA_NR_POINTER);

	ci_memzero(vector, ci_sizeof(u8 *) * BA_NR_POINTER);
	ci_memzero(pattern, ci_sizeof(u8) * BA_NR_POINTER);

	ci_balloc_init(ba, "Buddy System Test", ptr, ptr + BA_MEM_SIZE);
	ba->flag |= CI_BALLOC_LAZY_CONQUER;
	ci_balloc_dump(ba);

#if 0
	u8 *__t = ci_balloc(ba, 1);
	ci_printfln("__t=%p, +64=%p\n", __t - ci_sizeof(ci_mem_guard_head_t), __t - ci_sizeof(ci_mem_guard_head_t) + 64);
	ci_balloc_dump(ba);
#endif

#if 0	// data corruption test
	u8 *bad_ptr = ci_balloc(ba, 345);
	bad_ptr[-1] = 0xFF; 
	ci_bfree(ba, bad_ptr);
#endif

	ci_loop(loop_cnt, BA_TEST_LOOP) {
		if (loop_cnt && !(loop_cnt % 10000)) {
			ci_printfln();
			ci_imp_printfln("%lli -> " CI_PR_PCT_FMT, (u64)loop_cnt, ci_pr_pct_val(loop_cnt, BA_TEST_LOOP));
			ci_balloc_dump(ba);
		}

		int ptr_idx = ci_rand_shr(BA_NR_POINTER);
		int do_alloc = ci_rand_shr(100) <= BA_ALLOC_PERCENTAGE;

		if (do_alloc) {
			if (vector[ptr_idx])
				continue;

			int size = ci_rand_shr_i(BA_MIN_ALLOC_SIZE, BA_MAX_ALLOC_SIZE);
			if ((vector[ptr_idx] = ci_balloc(ba, size))) {	/* success */
//				vector[ptr_idx][size] = 0xCC;	// data corruption test, cannot be caught every time (hole)

#ifdef BA_CHECK_DATA
				u8 fill = ci_rand_shr_i(0xFF);
				ci_memset(vector[ptr_idx], fill, size);
				pattern[ptr_idx] = fill;
#endif
				alloc_size[ptr_idx] = size;
			}
		} else {
			if (!vector[ptr_idx])
				continue;

#ifdef BA_CHECK_DATA
			u8 fill = pattern[ptr_idx];		
			ci_loop(idx, alloc_size[ptr_idx])
				ci_panic_if(vector[ptr_idx][idx] != fill);
#endif			
			ci_bfree(ba, vector[ptr_idx]);
			vector[ptr_idx] = NULL;
		}
	}

	ci_balloc_dump(ba);
	
	ci_balloc_free_cache(ba);	/* so the pending will the right value (not in cache) */
	ci_balloc_dump(ba);

	ci_pause();

	int pending = 0;
	ci_loop(ptr_idx, BA_NR_POINTER) 
		if (vector[ptr_idx]) 
			pending++;
	ci_printfln();
	ci_panic_unless(pending == ci_balloc_nr_pending(ba));

	ci_balloc_dump_pending(ba);
#if 0
	ci_imp_printfln("pending=%d, try freeing...\n", pending);

	ci_loop(ptr_idx, BA_NR_POINTER) 
		if (vector[ptr_idx]) 
			ci_bfree(ba, vector[ptr_idx]);
	ci_balloc_dump(ba);
#endif
}

static void shr_balloc_test()
{
	void *ptr = ci_shr_balloc(33);
	ci_printf("ptr=%p\n", ptr);
	ci_shr_balloc_dump();

	ci_shr_bfree(ptr);
	ci_shr_balloc_dump();
}

void test_malloc()
{
//	shr_balloc_test();
	balloc_stress_test();
}

