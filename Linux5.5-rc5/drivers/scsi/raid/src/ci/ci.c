/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci.c							Common Infrastructure
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

int ci_init()
{
	/* do pre-init */
	ci_info_pre_init();
	ci_mod_pre_init();
	ci_node_pre_init();
	ci_sched_pre_init();

	/* setup printf */
	ci_printf_init();

#ifdef CI_WORKER_STA
	ci_warn_printfln("CI_WORKER_STA enabled, performance affected.");
#endif

#ifdef CI_PAVER_STA
	ci_warn_printfln("CI_PAVER_STA enabled, performance affected.");
#endif
	
	ci_imp_printf("ci init\n");

	/* pal init */
	pal_init();

	/* ci library init */
	ci_node_init();
	ci_paver_init();
	ci_mod_init();

	/* post init */
	ci_worker_post_init();
	
	return 0;
}

int ci_init_done()
{
	ci_imp_printf("ci init done\n");
	pal_init_done();
	return 0;
}

int ci_finz()
{
	ci_imp_printf("ci finz\n");
	ci_mod_finz();

	ci_pause();	

	return 0;
}

int ci_finz_done()
{
	pal_finz();
	ci_imp_printf("ci finz done\n");
	return 0;
}

