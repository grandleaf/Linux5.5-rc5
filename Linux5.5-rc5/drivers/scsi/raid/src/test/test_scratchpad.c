#include "ci.h"

#if 0

// #define LOG2(n)		((n) & 0xFF00 ? 8 + LOG2_8((n) >> 8) : LOG2_8(n))


#include <math.h>  

double Log2( double n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2 );  
}

void test_scratchpad()
{
	ci_here();

	ci_loop(i, 1, 300) {
//		if (ci_log2(i) != (int)Log2(i))
			printf("%2d, log2=%d, log2_ceil=%d, math.log2=%f\n", i, ci_log2(i), ci_log2_ceil(i), Log2(i));
		ci_assert(ci_log2(i) == (int)Log2(i));
	}

	ci_printf("passed\n");
}

#endif


#if 0
	ci_ssw_switch(cmd, {
		ci_ssw_case("history"):
			cli_intl_cmd_history(info);
			ci_ssw_break;
			
		ci_ssw_case("history -c"):
			cli_intl_cmd_history_clear(info);		// ci_todo:
			ci_ssw_break;

		ci_ssw_case("clear"):
			cli_intl_cmd_clear(info)
			ci_ssw_break;
			
		ci_ssw_case("quit"):
			cli_intl_cmd_quit(info)
			ci_ssw_break;

		ci_ssw_default:
#if 0			
			pal_sock_send_str(&info->sock_info, "  ");
			pal_sock_send_str(&info->sock_info, cmd);
			pal_sock_send_str(&info->sock_info, " <--\n");
#endif			
			ci_ssw_break;
	});
#endif

#define BITS			127
ci_bmp_def(ci_test_map, BITS);





void test_scratchpad()
{
#if 0
	ci_test_map_t map = { CI_BMP_ELM_FILL_INITIALIZER(BITS) };
	ci_test_map_dumpln(&map);


//	ci_printf("mask=%#llX\n", CI_BMP_MAX & (CI_BMP_MAX >> (CI_BITS_PER_BMP - (9 & CI_BMP_MASK))));
	ci_dead_loop();
#endif

#if 0
	ci_printfln("last_set=%d", u64_last_set(0));
	ci_loop(i, 64) {
		u64 mask = 1ULL << i;
		ci_assert(i == u64_last_set(mask));
		ci_printfln("i=%d, last_set=%d, wheel_id=%d", i, u64_last_set(mask), i >> 3);
	}
#endif	

#if 0
	for (;;) {
		ci_printf("wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");
		pal_msleep(1000);
	}
#endif

//	ci_printf("wwwwwwwwwwwwwwwww ci_list_t size=%d\n", ci_sizeof(ci_list_t));

#if 0
	ci_paver_map_t a, b, c;
	ci_paver_map_zero(&a);
	ci_paver_map_zero(&b);
//	ci_paver_map_zero(&c);

	ci_paver_map_set_bit(&b, 1);
	ci_paver_map_set_bit(&b, 2);
	ci_paver_map_set_bit(&b, 3);

	ci_paver_map_sub(&c, &b, &a);
#endif


#if 0
	pal_perf_prep();
	u64 start = pal_perf_counter();
	u64 end = pal_perf_counter();
	printf("overhead=%llu\n", end - start);
#endif

#if 0
	pal_perf_prep();
ci_printf("#### start ###\n");
	u64 start = pal_perf_counter();
	sleep(3);
	u64 stop = pal_perf_counter();
	u64 cycle = stop - start;
	printf("############## start=%llu, stop=%llu, cycle=%llu, seconds=%.2f\n", start, stop, cycle, (double)cycle / PAL_CYCLE_PER_SEC);
#endif
}




