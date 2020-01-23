#include "ci.h"

#define TRACE_LOOP_CNT				10000ULL
//#define TRACE_LOOP_CNT				1000000000ULL


static void write_trace_chunk(FILE *file, ci_trace_chunk_t *tc)
{
	if (!tc->meta->ts_walk) 
		;
//		ci_printf("Skip chunk %05d ...\n", tc->meta->id);
//		ci_printf("Skip chunk %p ...\n", tc->meta);
	else {
//		ci_printf("Writing chunk %05d ...\n", tc->meta->id);
		ci_printf("Writing chunk %p ...\n", tc->meta);
		fwrite(tc->meta, CI_TRACE_CHUNK_SIZE, 1, file); 
	}
}

static void write_trace_file()
{
	FILE *write_file;
	ci_list_t *head;
	ci_trace_chunk_t *tc;	

	ci_trace_chunk_t *curr = ci_container_of((trace_mgr.curr)->link.prev, ci_trace_chunk_t, link);
	ci_memdump(curr->meta, 0x200, "trace_test");

	ci_printf("### WRITING TRACE FILE ###\n");
#ifdef WIN_SIM
	fopen_s(&write_file, "trace.bin","wb");  
#elif defined(__GNUC__)
	write_file = fopen("trace.bin","wb"); 
#endif

	head = &trace_mgr.curr->link;
	ci_list_each(head, tc, link) 
		write_trace_chunk(write_file, tc);
	write_trace_chunk(write_file, trace_mgr.curr);

	fclose(write_file);
}

static void do_trace_test()
{
	u8 u8_val 	= 0x88;
	u16 u16_val = 0x1616;
	u32 u32_val = 0x32323232;
	u64 u64_val = 0x6464646464646464;


#define STR 		"OKOKOKOK"
#define do_trace(...)				ci_trace_ex(&trace_mgr, tc, 0xAABB, __VA_ARGS__)

//	do_trace("gaga_value=%d", STR, u8_val, u16_val, u32_val, u64_val);		// compiling error
	ci_trace_chunk_t *tc = ci_trace_mgr_get_chunk(&trace_mgr, NULL);

	char pr_buf[255] = "value=%d\n";
	char value_buf[255] = "gaga";

	do_trace("good morning");
	do_trace("FILE:%s, LINE:%d, %s, %d, %d", __FILE__, __LINE__, STR, u8_val, u16_val);		// __func__
	do_trace("u8=%#X, u16=%#X, u32=%#X, u64=%#llX\n", u8_val, u16_val, u32_val, u64_val);
	do_trace(pr_buf, 3);
	pr_buf[7] = 's';
	do_trace(pr_buf, value_buf);
	do_trace("my id is %d", 3399);
	do_trace("my name is %s\n", "big rabbit");
	ci_loop(i, 20)
		do_trace("today is %d\n", i);

	do_trace("value=%028llX\n", (u64)0x33445566778899AA);
	do_trace("value=%-28llx, OK\n", 0x33445566778899AAULL);
	do_trace("value=%d\n", -1);
	do_trace("value=%u\n", 0xFFFFFFFF);
	do_trace("value=%X\n", 0xFFFFFFFF);

	do_trace("nArgc, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %s\n", 
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, value_buf);
	do_trace("empty str=%s", "");
	value_buf[0] = 0;
	do_trace("another empty str=%s", value_buf);

	ci_loop(i, 10) do_trace("i=%d", i);		// 100000

	write_trace_file();
}


static void trace_performance()
{
	ci_trace_chunk_t *tc = ci_trace_mgr_get_chunk(&trace_mgr, NULL);

	ci_perf_eval(TRACE_LOOP_CNT, 1, {
		ci_trace_ex(&trace_mgr, tc, 0xAABB, "i=%llX", (u64)__loop_cnt__);	
	});
}

static int perf_setup_work(pal_perf_work_t *work) 
{
	work->arg[0] = &trace_mgr;
	work->arg[1] = ci_trace_mgr_get_chunk(&trace_mgr, NULL);
	return 0;
}

static int perf_do_work(pal_perf_work_t *work)
{
	ci_trace_mgr_t *mgr = (ci_trace_mgr_t *)work->arg[0];
	ci_trace_chunk_t *tc = (ci_trace_chunk_t *)work->arg[1];

	ci_loop(i, TRACE_LOOP_CNT)
		ci_trace_ex(mgr, tc, 0xABCD, "i=%llX", (u64)i);

	return 0;
}

static int perf_finish_work(pal_perf_work_t *work, int nr, u64 us)
{
	ci_big_nr_parse_t tp = CI_TIME_US_PARSE;
	tp.big_nr = us;
	ci_big_nr_parse(&tp);	
	ci_printf("--> " CI_PRN_FMT_TIME_SEC " Seconds (%lli usec)\n", CI_PRN_VAL_TIME_SEC(tp), tp.big_nr);		

	u64 mops = (u64)TRACE_LOOP_CNT * 1000000 * nr / us;
	ci_big_nr_parse_t mops_p = { .flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_UNIT_VERBOSE, .big_nr = mops, .dec_len = 3, .unit = NULL };
	ci_big_nr_parse(&mops_p);	
	ci_printf("--> %d.%03d %s (%lli) operations per second\n", mops_p.int_part, mops_p.dec_part, mops_p.unit, mops_p.big_nr);

//	write_trace_file();
	return 0;
}

static void threaded_trace_performance()
{
	ci_perf_mt(18, perf_setup_work, perf_do_work, perf_finish_work);
}

void test_trace()
{
	ci_here();

	ci_printf("CI_TRACE_BUF_SIZE 	= %d MiB\n", ci_to_mib(CI_TRACE_BUF_SIZE));
	ci_printf("CI_TRACE_CHUNK_SIZE 	= %d KiB\n", ci_to_kib(CI_TRACE_CHUNK_SIZE));
	ci_printf("CI_NR_TRACE_CHUNK	= %i\n",	 CI_NR_TRACE_CHUNK);
		
	ci_trace_mgr_init(&trace_mgr);
	
//	do_trace_test();
	trace_performance();
//	threaded_trace_performance();
}





