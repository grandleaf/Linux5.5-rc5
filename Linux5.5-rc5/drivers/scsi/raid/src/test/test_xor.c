#include "ci.h"

#define SUPPRESS_OUTPUT

#if 0
#define PERFORMANCE_TEST				/* if disabled, do data integrity check, NOW controller by makefile */
#define CACHE_MISSING_TEST				/* Make a big buffer, expect cache missing */
#endif

#define TEST_NR_PASS					20
#define TEST_NR_PASS_BIG				100

#ifdef CI_DEBUG
	#define TEST_XOR_SIMD_LOOP			(50000 / 10)
#else
	#define TEST_XOR_SIMD_LOOP			50000
#endif

#ifdef PERFORMANCE_TEST
#define MAX_NR_BUF						12
#else
#define MAX_NR_BUF						32
#endif

#define BUF_PADDING						1024
#define CORE_AFFINITY_ID				3

#ifdef CACHE_MISSING_TEST
#define MAX_BUF_SIZE					(128 * 1024 * 1024 + BUF_PADDING)
#else
#define MAX_BUF_SIZE					(128 * 1024 + BUF_PADDING)
#endif


#define MAX_DATA_CHK_BUF_SIZE			(64 * 1024)


typedef struct {
	const char			*name;
	int					 flag;
#define RANDOMIZE_PTR			0x0001
#define XOR_TEST_Q				0x0002	/* used for Q calculation */
#define XOR_TEST_PQ_ROT			0x0004	/* used for P + Q ROT calculation */
#define XOR_TEST_PQ_RMW			0x0008	/* used for P + Q ROT calculation */

	int					 loop;			/* each test do how many loops */
	int					 pass;			/* repeat this test */
	int					 nr_buf;		/* how many buffers */
	int					 buf_size;		/* buffer size for calculation */
	int					 hw_mul;		/* host write multiplier */
	int					 mr_mul;		/* memory read multiplier */

	union {
		void (*fn_p_xor)(void **buf, int nr, int size);
		void (*fn_q_xor)(void **buf, u8 *coef, int nr, int size);
		void (*fn_pq_rot_xor)(void **buf, u8 *coef, int nr, int size);
		void (*fn_pq_rmw_xor)(void **buf, u8 *coef, int nr, int size);
	};
} xor_test_cfg_t;

static u8 *xor_buf[MAX_NR_BUF];
static u8 *chk_buf0, *chk_buf1;
static void *xor_ptr[MAX_NR_BUF];

static void perf_memcpy(void **buf, int nr, int size);


/* 64K Sequential Write, Aligned. */
xor_test_cfg_t xor_p_aligned_sw = {
	.name			= "xor_p_aligned_sw: \"full stripe write\"",
	.loop			= TEST_XOR_SIMD_LOOP,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 11,			/* 1 parity + 10 data */
	.buf_size		= 64 * 1024,	/* 64KB */
	.hw_mul			= 10,			/* host write 10 data */
	.mr_mul			= 10,			/* memory read 10 data for xor */
	.fn_p_xor		= pal_xor_p
};

/* 64K Sequential Write, Unaligned. */
xor_test_cfg_t xor_p_unaligned_sw = {
	.name			= "xor_p_unaligned_sw: \"full stripe write\"",
	.flag			= RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP,
	.pass 			= TEST_NR_PASS_BIG,
	.nr_buf 		= 11,			/* 1 parity + 10 data */
	.buf_size		= 64 * 1024,	/* 64KB */
	.hw_mul			= 10,			/* host write 10 data */
	.mr_mul			= 10,			/* memory read 10 data for xor */
	.fn_p_xor		= pal_xor_p
};

/* 4K Random Write, Aligned. */
xor_test_cfg_t xor_p_aligned_rw = {
	.name			= "xor_p_aligned_rw: \"random write\"",
	.loop			= TEST_XOR_SIMD_LOOP * 100,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 4,			/* new parity, old parity, new data and old data */
	.buf_size		= 4 * 1024,		/* 4KB */
	.hw_mul			= 1,			/* host write 1 data */
	.mr_mul			= 3,			/* old parity + old data + new data */
	.fn_p_xor		= pal_xor_p
};

/* 4K Random Write, Unaligned. */
xor_test_cfg_t xor_p_unaligned_rw = {
	.name			= "xor_p_unaligned_rw: \"random write\"",
	.flag			= RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP * 100,
	.pass 			= TEST_NR_PASS_BIG,
	.nr_buf 		= 4,			/* new parity, old parity, new data and old data */
	.buf_size		= 4 * 1024,		/* 4KB */
	.hw_mul			= 1,			/* host write 1 data */
	.mr_mul			= 3,			/* old parity + old data + new data */
	.fn_p_xor		= pal_xor_p
};

#ifdef CACHE_MISSING_TEST
/* Big Sequential Write, Aligned. */
xor_test_cfg_t xor_p_aligned_big_sw = {
	.name			= "xor_p_aligned_big_sw: \"big full stripe write\"",
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 11,							/* 1 parity + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_p_xor		= pal_xor_p
};

/* Big Random Write, Aligned. */
xor_test_cfg_t xor_p_aligned_big_rw = {
	.name			= "xor_p_aligned_big_rw: \"big random write\"",
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 4,							/* new parity, old parity, new data and old data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 3,							/* old parity + old data + new data */
	.fn_p_xor		= pal_xor_p
};

/* Big Sequential Write, Un-Aligned. */
xor_test_cfg_t xor_p_unaligned_big_sw = {
	.name			= "xor_p_unaligned_big_sw: \"unaligned big full stripe write\"",
	.flag			= RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 11,							/* 1 parity + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_p_xor		= pal_xor_p
};

/* Big Random Write, Un-Aligned. */
xor_test_cfg_t xor_p_unaligned_big_rw = {
	.name			= "xor_p_unaligned_big_rw: \"unaligned big random write\"",
	.flag			= RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 4,							/* new parity, old parity, new data and old data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 3,							/* old parity + old data + new data */
	.fn_p_xor		= pal_xor_p
};

/* Big Memory Copy, Aligned. */
xor_test_cfg_t cfg_perf_memcpy = {
	.name			= "memcpy_big: \"big chunk memcpy\"",
	.flag			= RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 2,							/* new parity, old parity, new data and old data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 1,							/* read source, then write */
	.fn_p_xor		= perf_memcpy
};

xor_test_cfg_t xor_q_aligned_big_sw = {
	.name			= "xor_q_aligned_big_sw: \"big full stripe write\"",
	.flag			= XOR_TEST_Q,
	.loop			= TEST_XOR_SIMD_LOOP / 2048,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 11,							/* 1 parity + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_q_xor		= pal_xor_q
};

xor_test_cfg_t xor_q_aligned_big_rw = {
	.name			= "xor_q_aligned_big_rw: \"big random write\"",
	.flag			= XOR_TEST_Q,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 4,							/* new parity, old parity, new data and old data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 3,							/* old parity + old data + new data */
	.fn_q_xor		= pal_xor_q
};

xor_test_cfg_t xor_q_unaligned_big_sw = {
	.name			= "xor_q_unaligned_big_sw: \"unaligned big full stripe write\"",
	.flag			= XOR_TEST_Q | RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 2048,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 11,							/* 1 parity + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_q_xor		= pal_xor_q
};

xor_test_cfg_t xor_q_unaligned_big_rw = {
	.name			= "xor_q_unaligned_big_rw: \"unaligned big random write\"",
	.flag			= XOR_TEST_Q | RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 4,							/* new parity, old parity, new data and old data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 3,							/* old parity + old data + new data */
	.fn_q_xor		= pal_xor_q
};

/* PQ rot */
xor_test_cfg_t xor_pq_rot_aligned_big_sw = {
	.name			= "xor_pq_rot_aligned_big_sw: \"big full stripe write\"",
	.flag			= XOR_TEST_PQ_ROT,
	.loop			= TEST_XOR_SIMD_LOOP / 2048,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 12,							/* P, Q + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_pq_rot_xor	= pal_xor_pq_rot
};

xor_test_cfg_t xor_pq_rot_unaligned_big_sw = {
	.name			= "xor_pq_rot_unaligned_big_sw: \"unaligned big full stripe write\"",
	.flag			= XOR_TEST_PQ_ROT | RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 2048,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 12,							/* P, Q + 10 data */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* Big */
	.hw_mul			= 10,							/* host write 10 data */
	.mr_mul			= 10,							/* memory read 10 data for xor */
	.fn_pq_rot_xor	= pal_xor_pq_rot
};

/* PQ RMW */
xor_test_cfg_t xor_pq_rmw_aligned_big_rw = {
	.name			= "xor_pq_rmw_aligned_big_rw: \"big random write\"",
	.flag			= XOR_TEST_PQ_RMW,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 6,							/* P, Q, P', Q', D, D' */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 4,							/* P', Q', D, D' */
	.fn_pq_rmw_xor	= pal_xor_pq_rmw
};

xor_test_cfg_t xor_pq_rmw_unaligned_big_rw = {
	.name			= "xor_pq_rmw_unaligned_big_rw: \"unaligned big random write\"",
	.flag			= XOR_TEST_PQ_RMW | RANDOMIZE_PTR,
	.loop			= TEST_XOR_SIMD_LOOP / 1024,
	.pass 			= TEST_NR_PASS,
	.nr_buf 		= 6,							/* P, Q, P', Q', D, D' */
	.buf_size		= MAX_BUF_SIZE - BUF_PADDING,	/* big */
	.hw_mul			= 1,							/* host write 1 data */
	.mr_mul			= 4,							/* P', Q', D, D' */
	.fn_pq_rmw_xor	= pal_xor_pq_rmw
};

#endif

static void gf_xor_test();


static void buf_alloc()
{
	ci_loop(i, MAX_NR_BUF)
		xor_buf[i] = pal_aligned_malloc(MAX_BUF_SIZE, PAL_CPU_CACHE_LINE_SIZE);
	chk_buf0 = pal_aligned_malloc(MAX_BUF_SIZE, PAL_CPU_CACHE_LINE_SIZE);
	chk_buf1 = pal_aligned_malloc(MAX_BUF_SIZE, PAL_CPU_CACHE_LINE_SIZE);

#if 1
	ci_loop(i, MAX_NR_BUF)
		memset(xor_buf[i], 0xFF, MAX_BUF_SIZE);
	memset(chk_buf0, 0xFF, MAX_BUF_SIZE);
	memset(chk_buf1, 0xFF, MAX_BUF_SIZE);
#endif
}

static void randomize_xor_buf(int size)
{
#ifdef CACHE_MISSING_TEST
	printf("Big Buffer, skip memory randomization.\n");
#else
	if (size <= 0)
		size = MAX_BUF_SIZE;

	ci_loop(i, MAX_NR_BUF)
		ci_loop(j, size)
			xor_buf[i][j] = (u8)(rand() & 0xFF);
#endif		
}

static void setup_xor_ptr(xor_test_cfg_t *cfg)
{
	int start = 0;

	if (cfg->flag & (XOR_TEST_PQ_ROT | XOR_TEST_PQ_RMW)) {	/* make sure P & Q have the same alignment */
		int offset = (cfg->flag & RANDOMIZE_PTR ? rand() % 9 * 8 : 0);
		start = 2;
		xor_ptr[0] = (u8 *)xor_buf[0] + offset;
		xor_ptr[1] = (u8 *)xor_buf[1] + offset;
	}

	ci_loop(i, start, MAX_NR_BUF)
		xor_ptr[i] = (u8 *)xor_buf[i] + (cfg->flag & RANDOMIZE_PTR ? rand() % 9 * 8 : 0);
}

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

static void status_dump(xor_test_cfg_t *cfg, double xor_w)
{
#define FMT			"%20s : "
	int mem_band_factor = cfg->flag & (XOR_TEST_PQ_ROT | XOR_TEST_PQ_RMW) ? 2 : 1;		/* considering P & Q */

	printf(FMT "%.3f GB/s\n", 	"XOR/Memory Write", 	xor_w * mem_band_factor);
	printf(FMT "%.3f GB/s\n", 	"Memory Read", 			xor_w * cfg->mr_mul);
	printf(FMT "%.3f GB/s\n",	"Memory Total",			xor_w * (cfg->mr_mul + mem_band_factor));

	printf(FMT "%.3f GB/s\n", 	"Host Bandwidth", 		xor_w * cfg->hw_mul);
	double iops = xor_w * cfg->hw_mul * 1024 * 1024 * 1024 / cfg->buf_size / 1000;
	printf(FMT "%.3f K/s\n", "Host IOPS", iops);
	printf(FMT "%.3f M/s\n", "4K Host IOPS", iops * cfg->buf_size / 4096 / 1000);
}

static void run_test_case(xor_test_cfg_t *cfg)
{
	u8 coef[RG_MAX_XOR_ELM];

	ci_loop(i, 140) 
		printf("#"); 
	printf("\n\n");

	int *history = (int *)malloc(sizeof(int) * cfg->pass);		// weird, double qsort() doesn't work
	ci_loop(i, RG_MAX_XOR_ELM)
		coef[i] = ci_rand_shr_i(0x01, 0xFF);

	ci_assert(history);
	printf("TEST { name:\"%s\", nr_buf:%d, buf_size:%d, loop:%d, pass:%d%s }\n\n", 
			cfg->name, cfg->nr_buf, cfg->buf_size, cfg->loop, cfg->pass,
#ifdef PAL_XOR_NON_TEMPORAL
			", NON_TEMPORAL"
#else
			""
#endif
		  );

	ci_loop(i, cfg->pass) {
		setup_xor_ptr(cfg);
		printf("PASS %04d: [ %s, %d, %d, %d/%d ]\n", i + 1, cfg->name, cfg->nr_buf, cfg->buf_size, i + 1, cfg->pass);
		const clock_t begin_time = clock();

		ci_loop(j, cfg->loop)
			if (cfg->flag & XOR_TEST_Q)
				cfg->fn_q_xor(xor_ptr, coef, cfg->nr_buf, cfg->buf_size);
			else if (cfg->flag & XOR_TEST_PQ_ROT)
				cfg->fn_pq_rot_xor(xor_ptr, coef, cfg->nr_buf, cfg->buf_size);
			else if (cfg->flag & XOR_TEST_PQ_RMW)
				cfg->fn_pq_rot_xor(xor_ptr, coef, cfg->nr_buf, cfg->buf_size);
			else
				cfg->fn_p_xor(xor_ptr, cfg->nr_buf, cfg->buf_size);

		double seconds = (float)(clock() - begin_time ) /  CLOCKS_PER_SEC;
		double xor_w = (double)cfg->buf_size * cfg->loop / 1024 / 1024 / 1024 / seconds;
		history[i] = (int)(xor_w * 1000000);
		
		status_dump(cfg, xor_w);
		printf(FMT "%.3f s\n", 		"Time Elapsed", 				seconds);
		printf("\n\n");
	}

	double average		= 0;
	int history_start	= cfg->pass / 10;
	int history_end		= cfg->pass - history_start;

	qsort(history, (size_t)cfg->pass, sizeof(history[0]), cmpfunc);
	ci_loop(i, history_start, history_end) {
		double xor_w = (double)history[i] / 1000000;
		average += xor_w;
	}
	free(history);

	int total_sampled = history_end - history_start;
	average /= total_sampled;

	printf("Total Sampled: %d of %d - [ %s ]\n", total_sampled, cfg->pass, cfg->name);
	printf("---------------------------------------------------------------------------------\n");
	status_dump(cfg, average);

	printf("\n"); ci_loop(i, 140) printf("#"); printf("\n");
	printf("\n\n\n\n\n");
}

static void perf_memcpy(void **buf, int nr, int size)
{
	memcpy(buf[1], buf[0], size);
}

static void p_data_integrity_test()
{
	int nr_buf, buf_size;
	int buf_offset[RG_MAX_XOR_ELM];
	void *ptr[RG_MAX_XOR_ELM];
	void *perf_ptr, *chk_ptr;

//	ci_unused(p_data_integrity_test);

	buf_size = ci_rand_shr_i(8, MAX_DATA_CHK_BUF_SIZE);
	buf_size = ci_align_upper(buf_size, 8);
	nr_buf = ci_rand_shr_i(3, RG_MAX_XOR_ELM);
	randomize_xor_buf(buf_size + 64);

	ci_loop(i, nr_buf)
		buf_offset[i] = ci_align_lower(ci_rand_shr_i(0, 64), 8);

#ifndef SUPPRESS_OUTPUT
	printf("     p, nr_buf=%02d, buf_size=%06d, [ ", nr_buf, buf_size);
	ci_loop(i, nr_buf)
		printf("%02d ", buf_offset[i]);
	printf("]\n");
#else
	static u64 counter;
	if (counter % 100 == 0)
		ci_printf("     p_data_integrity, process:%u, counter:%llu\n", ci_process_current(), counter);
	counter++;
	fflush(stdout);
#endif

	ci_loop(i, nr_buf)	/* assign buffer pointers */
		ptr[i] = (u8 *)xor_buf[i] + buf_offset[i];

	perf_ptr = ptr[0];
	pal_xor_p(ptr, nr_buf, buf_size);

	chk_ptr = ptr[0] = (u8 *)chk_buf0 + buf_offset[0];
	pal_xor_p_safe(ptr, nr_buf, buf_size);

//	*((u8 *)perf_ptr + 100) = 0xEF;

	if (!ci_memequal(perf_ptr, chk_ptr, buf_size)) 
		ci_panic("Data Integrity Failed!\n");
}

static void q_data_integrity_test()
{
	int nr_buf, buf_size;
	int buf_offset[RG_MAX_XOR_ELM];
	void *ptr[RG_MAX_XOR_ELM];
	void *perf_ptr, *chk_ptr;
	u8 coef[RG_MAX_XOR_ELM];

//	ci_unused(p_data_integrity_test);

	buf_size = ci_rand_shr_i(8, MAX_DATA_CHK_BUF_SIZE);
	buf_size = ci_align_upper(buf_size, 8);
	nr_buf = ci_rand_shr_i(3, RG_MAX_XOR_ELM);
	randomize_xor_buf(buf_size + 64);

	ci_loop(i, nr_buf)
		buf_offset[i] = ci_align_lower(ci_rand_shr_i(0, 64), 8);

	ci_memzero(coef, ci_sizeof(coef));
	ci_loop(i, 1, nr_buf)
		coef[i] = ci_rand_shr_i(1, 0xFF);

#ifndef SUPPRESS_OUTPUT
	printf("     q, nr_buf=%02d, buf_size=%06d, [ ", nr_buf, buf_size);
	ci_loop(i, nr_buf)
		printf("%02d ", buf_offset[i]);
	printf("]\n                                    [ ");
	ci_loop(i, nr_buf)
	printf("%02X ", coef[i]);
	printf("]\n");
#else
	static u64 counter;
	if (counter % 100 == 0)
		ci_printf("     q_data_integrity, process:%u, counter:%llu\n", ci_process_current(), counter);
	counter++;
	fflush(stdout);
#endif

	ci_loop(i, nr_buf) 		/* assign buffer pointers */
		ptr[i] = (u8 *)xor_buf[i] + buf_offset[i];		

	perf_ptr = ptr[0];
	pal_xor_q(ptr, coef, nr_buf, buf_size);

	chk_ptr = ptr[0] = (u8 *)chk_buf0 + buf_offset[0];
	pal_xor_q_safe(ptr, coef, nr_buf, buf_size);

//	*((u8 *)perf_ptr + 100) = 0xEF;
#if 0
	ci_memdump(coef, 16, "coef[]");
	ci_printf("\n\n");
	ci_memdump(ptr[1], 32, "buf[1]");
	ci_printf("\n\n");
	ci_memdump(ptr[2], 32, "buf[2]");
	ci_printf("\n\n");

	ci_memdump(perf_ptr, 32, "q_buf");
	ci_printf("\n\n");
	ci_memdump(chk_ptr, 32, "chk_buf0");
	exit(0);
#endif
	if (!ci_memequal(perf_ptr, chk_ptr, buf_size)) 
		ci_panic("Data Integrity Failed!\n");
}

static void pq_rot_data_integrity_test()
{
	int nr_buf, buf_size;
	int buf_offset[RG_MAX_XOR_ELM];
	void *ptr[RG_MAX_XOR_ELM];
	void *perf_ptr0, *perf_ptr1, *chk_ptr0, *chk_ptr1;
	u8 coef[RG_MAX_XOR_ELM];

	buf_size = ci_rand_shr_i(8, MAX_DATA_CHK_BUF_SIZE);
	buf_size = ci_align_upper(buf_size, 8);
	nr_buf = ci_rand_shr_i(4, RG_MAX_XOR_ELM);
	randomize_xor_buf(buf_size + 64);

	ci_loop(i, 1, nr_buf)
		buf_offset[i] = ci_align_lower(ci_rand_shr_i(0, 64), 8);
	buf_offset[0] = buf_offset[1];

	ci_memzero(coef, ci_sizeof(coef));
	ci_loop(i, 2, nr_buf)
		coef[i] = ci_rand_shr_i(1, 0xFF);

#ifndef SUPPRESS_OUTPUT
	printf("pq_rot, nr_buf=%02d, buf_size=%06d, [ ", nr_buf, buf_size);
	ci_loop(i, nr_buf)
		printf("%02d ", buf_offset[i]);
	printf("]\n                                    [ ");
	ci_loop(i, nr_buf)
	printf("%02X ", coef[i]);
	printf("]\n");
#else
	static u64 counter;
	if (counter % 100 == 0)
		ci_printf("pq_rot_data_integrity, process:%u, counter:%llu\n", ci_process_current(), counter);
	counter++;
	fflush(stdout);
#endif

	ci_loop(i, nr_buf) 		/* assign buffer pointers */
		ptr[i] = (u8 *)xor_buf[i] + buf_offset[i];		

	perf_ptr0 = ptr[0];
	perf_ptr1 = ptr[1];
	pal_xor_pq_rot(ptr, coef, nr_buf, buf_size);

	chk_ptr0 = ptr[0] = (u8 *)chk_buf0 + buf_offset[0];
	chk_ptr1 = ptr[1] = (u8 *)chk_buf1 + buf_offset[1];
	pal_xor_pq_rot_safe(ptr, coef, nr_buf, buf_size);

//	*((u8 *)perf_ptr1 + 100) = 0xEF;

	if (!ci_memequal(perf_ptr0, chk_ptr0, buf_size)) 
		ci_panic("P, Data Integrity Failed!\n");
	if (!ci_memequal(perf_ptr1, chk_ptr1, buf_size)) 
		ci_panic("Q, Data Integrity Failed!\n");
}

static void pq_rmw_data_integrity_test()
{
	int nr_buf, buf_size;
	int buf_offset[RG_MAX_XOR_ELM];
	void *ptr[RG_MAX_XOR_ELM];
	void *perf_ptr0, *perf_ptr1, *chk_ptr0, *chk_ptr1;
	u8 coef[RG_MAX_XOR_ELM];

	buf_size = ci_rand_shr_i(8, MAX_DATA_CHK_BUF_SIZE);
	buf_size = ci_align_upper(buf_size, 8);
	nr_buf = ci_rand_shr_i(6, RG_MAX_XOR_ELM);
	randomize_xor_buf(buf_size + 64);

	ci_loop(i, 1, nr_buf)
		buf_offset[i] = ci_align_lower(ci_rand_shr_i(0, 64), 8);
	buf_offset[0] = buf_offset[1];

	ci_memzero(coef, ci_sizeof(coef));
	ci_loop(i, 4, nr_buf)
		coef[i] = ci_rand_shr_i(1, 0xFF);
	coef[2] = coef[3] = 1;

#ifndef SUPPRESS_OUTPUT
	printf("pq_rmw, nr_buf=%02d, buf_size=%06d, [ ", nr_buf, buf_size);
	ci_loop(i, nr_buf)
		printf("%02d ", buf_offset[i]);
	printf("]\n                                    [ ");
	ci_loop(i, nr_buf)
	printf("%02X ", coef[i]);
	printf("]\n");
#else
	static u64 counter;
	if (counter % 100 == 0)
		ci_printf("pq_rmw_data_integrity, process:%u, counter:%llu\n", ci_process_current(), counter);
	counter++;
	fflush(stdout);
#endif

	ci_loop(i, nr_buf) 		/* assign buffer pointers */
		ptr[i] = (u8 *)xor_buf[i] + buf_offset[i];		

	perf_ptr0 = ptr[0];
	perf_ptr1 = ptr[1];
	pal_xor_pq_rmw(ptr, coef, nr_buf, buf_size);

	chk_ptr0 = ptr[0] = (u8 *)chk_buf0 + buf_offset[0];
	chk_ptr1 = ptr[1] = (u8 *)chk_buf1 + buf_offset[1];
	pal_xor_pq_rmw_safe(ptr, coef, nr_buf, buf_size);

//	*((u8 *)perf_ptr1 + 100) = 0xEF;

	if (!ci_memequal(perf_ptr0, chk_ptr0, buf_size)) 
		ci_panic("P, Data Integrity Failed!\n");
	if (!ci_memequal(perf_ptr1, chk_ptr1, buf_size)) 
		ci_panic("Q, Data Integrity Failed!\n");
}

static void qq_data_integrity_test()
{
	int nr_buf, buf_size;
	int buf_offset[RG_MAX_XOR_ELM];
	void *ptr[RG_MAX_XOR_ELM];
	void *perf_ptr0, *perf_ptr1, *chk_ptr0, *chk_ptr1;
	u8 coef0[RG_MAX_XOR_ELM], coef1[RG_MAX_XOR_ELM];

	buf_size = ci_rand_shr_i(8, MAX_DATA_CHK_BUF_SIZE);
	buf_size = ci_align_upper(buf_size, 8);
	nr_buf = ci_rand_shr_i(4, RG_MAX_XOR_ELM);
	randomize_xor_buf(buf_size + 64);

	ci_loop(i, 1, nr_buf)
		buf_offset[i] = ci_align_lower(ci_rand_shr_i(0, 64), 8);
	buf_offset[0] = buf_offset[1];

	ci_memzero(coef0, ci_sizeof(coef0));
	ci_memzero(coef1, ci_sizeof(coef1));
	ci_loop(i, 2, nr_buf) {
		coef0[i] = ci_rand_shr_i(1, 0xFF);
		coef1[i] = ci_rand_shr_i(1, 0xFF);
	}
	coef0[0] = coef0[1] = coef1[0] = coef1[1] = 0;

#ifndef SUPPRESS_OUTPUT
	printf("qq, nr_buf=%02d, buf_size=%06d,     [ ", nr_buf, buf_size);
	ci_loop(i, nr_buf)
		printf("%02d ", buf_offset[i]);
	printf("]\n                                    [ ");
	ci_loop(i, nr_buf)
	printf("%02X ", coef0[i]);
	printf("]\n                                    [ ");
	ci_loop(i, nr_buf)
	printf("%02X ", coef1[i]);
	printf("]\n");
#else
	static u64 counter;
	if (counter % 100 == 0)
		ci_printf("    qq_data_integrity, process:%u, counter:%llu\n", ci_process_current(), counter);
	counter++;
	fflush(stdout);
#endif

	ci_loop(i, nr_buf) 		/* assign buffer pointers */
		ptr[i] = (u8 *)xor_buf[i] + buf_offset[i];		

	perf_ptr0 = ptr[0];
	perf_ptr1 = ptr[1];
	pal_xor_qq(ptr, coef0, coef1, nr_buf, buf_size);

	chk_ptr0 = ptr[0] = (u8 *)chk_buf0 + buf_offset[0];
	chk_ptr1 = ptr[1] = (u8 *)chk_buf1 + buf_offset[1];
	pal_xor_qq_safe(ptr, coef0, coef1, nr_buf, buf_size);

//	*((u8 *)perf_ptr1 + 100) = 0xEF;

	if (!ci_memequal(perf_ptr0, chk_ptr0, buf_size)) 
		ci_panic("Q0, Data Integrity Failed!\n");

	if (!ci_memequal(perf_ptr1, chk_ptr1, buf_size)) 
		ci_panic("Q1, Data Integrity Failed!\n");
}


void test_xor()
{
	buf_alloc();

#ifdef PERFORMANCE_TEST
	printf("Binding current thread to core: %d\n", CORE_AFFINITY_ID);
	ci_thread_set_affinity(ci_thread_current(), CORE_AFFINITY_ID);
#endif

	printf("\n\n");


#ifdef PERFORMANCE_TEST
#ifndef PAL_XOR_SIMD
	ci_loop(3)
		ci_printf("!!! WARNING: PAL_XOR_SIMD DISABLED !!!\n");
#endif
//	run_test_case(&xor_p_aligned_sw);
//	run_test_case(&xor_p_aligned_rw);

#if 0
	run_test_case(&cfg_perf_memcpy);

	run_test_case(&xor_p_aligned_big_sw);
	run_test_case(&xor_p_unaligned_big_sw);
	run_test_case(&xor_p_aligned_big_rw);		
	run_test_case(&xor_p_unaligned_big_rw); 	

	run_test_case(&xor_q_aligned_big_sw);
	run_test_case(&xor_q_unaligned_big_sw);
	run_test_case(&xor_q_aligned_big_rw);
	run_test_case(&xor_q_unaligned_big_rw);
	
#endif

	run_test_case(&xor_pq_rot_aligned_big_sw);
	run_test_case(&xor_pq_rot_unaligned_big_sw);
	
	run_test_case(&xor_pq_rmw_aligned_big_rw);
	run_test_case(&xor_pq_rmw_aligned_big_rw);
#else
	for (;;) {
#ifndef PAL_XOR_SIMD
		ci_loop(3)
			ci_printf("!!! WARNING: PAL_XOR_SIMD DISABLED !!!\n");
#endif

		p_data_integrity_test();
		q_data_integrity_test();
		pq_rot_data_integrity_test();
		pq_rmw_data_integrity_test();
		qq_data_integrity_test();
	}
#endif
}

static void gf_xor_test()
{
	ci_here();
	return;
}




