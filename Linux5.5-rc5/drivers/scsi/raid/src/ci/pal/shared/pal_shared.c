/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_shared.c				PAL shared utilities
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"
#include "pal_numa.h"

#ifdef WIN_SIM
__declspec(allocate(".mod$a")) void *__mod_section_start;
__declspec(allocate(".mod$z")) void *__mod_section_end;
#endif

int ci_thread_set_affinity(pthread_t thread, int core)
{
	int rv;

   	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
   	CPU_SET(core, &cpuset);

   	rv = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);	
   	ci_assert(!rv);
   	return rv;
}

static int mod_name_comparator(ci_mod_t *a, ci_mod_t *b)
{
	ci_assert(ci_strcmp(a->name, b->name), "Duplicate module name \"%s\" for modules %p and %p.\n", a->name, a, b);
	return ci_strcmp(a->name, b->name);
}

static void pal_mod_add(ci_mod_t *mod)
{
	ci_assert(mod->name);
	pal_printf("pal_mod_add(%p, \"%s\")\n", mod, mod->name);	
	ci_list_add_sort(&ci_mod_info->mod_head, mod, link, mod_name_comparator);
}

static int pal_mod_init()
{
#ifdef WIN_SIM
	extern void *__mod_section_start, *__mod_section_end;

	for (void **p = &__mod_section_start; p < &__mod_section_end; p++) 
		if (*p) 
			pal_mod_add((ci_mod_t *)*p);
#else
	extern void *__start_ci_mod_section, *__stop_ci_mod_section;

	for (void **p = &__start_ci_mod_section; p < &__stop_ci_mod_section; p++) 
		pal_mod_add((ci_mod_t *)*p);
#endif	

	return 0;
}

int pal_init()
{
	pal_imp_printf("pal init, tid=%lld\n", pal_get_tid());
//	pal_timer_init();
	pal_numa_init();
	pal_rand_init();
	pal_xor_init();
	pal_mod_init();

	return 0;
}

int pal_finz()
{
	pal_imp_printf("pal finz, tid=%lld\n", pal_get_tid());
	pal_numa_finz();
	
	return 0;
}

