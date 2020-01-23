#include "ci.h"
#include "ci_raid_ver.h"



extern void test_xor();
extern void test_list();
extern void test_numa();
extern void test_gf_xor_dev();
extern void test_bmp();
extern void test_trace();
extern void test_scratchpad();
extern void test_malloc();
extern void test_mod();
extern void test_json();
extern void test_sched();
extern void test_timer();
extern void perf_test_start();
extern void test_sta();


static int main_entrance();

#ifdef LIB_FAST_RAID 
lib_fast_raid_cfg_t *lib_fast_raid_cfg;

int fast_raid_init(lib_fast_raid_cfg_t *cfg)
{
	if (cfg) {
		lib_fast_raid_cfg = cfg;

		if (ci_sizeof(lib_fast_raid_cfg_t) != cfg->size_check) {
			pal_err_printf("size mismatch: \"lib_fast_raid_cfg_t\"\n");
			pal_printf("size_check=%d, my_size=%d\n", cfg->size_check, ci_sizeof(lib_fast_raid_cfg_t));
			pal_printf("fast raid init failed!\n");
			return -1;
		}
	}

	return main_entrance();
} 

int fast_raid_finz()
{
	int rv = ci_finz();

	return rv;
}

#else

int main(int argc, char *argv[])
{
	return main_entrance();
}

#endif

static int main_entrance()
{
	ci_info.build_nr = CI_RAID_BUILD_NR;
	ci_printf_pre_init();		/* do it before first printf */
	
	ci_imp_printf("%s, %s, %s Build %s\n", __DATE__, __TIME__, __RELEASE_DEBUG__, CI_RAID_BUILD_NR);
	ci_init();


#ifndef LIB_FAST_RAID	
//	for (;;) pal_nsleep_no_ctx(1000000000ull);

//	ci_imp_printf("PRESS ANY KEY TO EXIST\n");
	ci_pause();
	ci_finz();
#endif

	return 0; 
}

int pal_init_done()
{
	test_scratchpad();

//	test_sched();
//	test_malloc();	
//	test_scratchpad();
//	test_bmp();
//	test_trace();

//	test_list();
//	test_xor();
//	test_numa();
//	test_gf_xor_dev();

//	test_mod();
//	test_json();

//	test_timer();

//	perf_test_start();
//	test_sta();

	return 0;
}

