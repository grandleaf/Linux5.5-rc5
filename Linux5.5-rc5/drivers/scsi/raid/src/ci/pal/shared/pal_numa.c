/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_numa.c				numa configurations
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"
#include "pal_numa.h"

u64 PAL_CYCLE_PER_SEC = 2301000000;
u64 PAL_CYCLE_OVERHEAD;

static s16 pal_cpu_alloc_axnpsim[PAL_NUMA_NR][PAL_CPU_TOTAL + 1] = {
//	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, -1 },
//	{ -1 }		/* disable node 1 */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, -1 },
	{ 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, -1 }
};

static s16 pal_cpu_alloc_ubuntu[PAL_NUMA_NR][PAL_CPU_TOTAL + 1] = {
#if 0
	{ 0, -1 },
#else	
	{ 0, 3, 6, 9, 12, 15, -1 },
	{ 1, 4, 7, 10, 13, -1 },
	{ 2, 5, 8, 11, 14, -1 }
#endif
};

static s16 pal_cpu_alloc_winsim[PAL_NUMA_NR][PAL_CPU_TOTAL + 1] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 },
	{ 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 }
};

#if 0
#ifdef WIN_SIM_NUMA_CFG
static s16 pal_cpu_alloc_win_sim[PAL_NUMA_NR][PAL_CPU_TOTAL + 1] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1 }
};
#else
static s16 pal_cpu_alloc_ubuntu[PAL_NUMA_NR][PAL_CPU_TOTAL + 1] = {
	{ 0, 3, 6, 9, -1 },
	{ 1, 4, 7, 10, -1 },
	{ 2, 5, 8, 11, -1 }
};
#endif
#endif

static int pal_cpu_grab_all;
static s16 (*pal_cpu_alloc)[PAL_CPU_TOTAL + 1];

pal_numa_info_t		pal_numa_info;
pal_malloc_info_t	pal_malloc_info;
pthread_key_t 		pal_tls_key;

static ci_int_to_name_t pal_numa_flag_to_name[] = {
	ci_int_name(PAL_NUMA_SMT),
	ci_int_name(PAL_NUMA_NODE_0_MEM_ALLOC_ONLY),
	CI_EOT	
};


int pal_numa_id_by_ptr(void *ptr)
{
#ifdef WIN_SIM
	return numa_node_from_pointer(ptr);
#else
	int numa_node = -1;
	get_mempolicy(&numa_node, NULL, 0, (void*)ptr, MPOL_F_NODE | MPOL_F_ADDR);
	return numa_node;
#endif
}

int pal_numa_id_by_stack()
{
	volatile int dummy;
	return pal_numa_id_by_ptr((void *)&dummy);
}

int pal_numa_node_0_mem_alloc_only()
{
	return pal_numa_info.flag & PAL_NUMA_NODE_0_MEM_ALLOC_ONLY;
}

static int pal_numa_dump()
{
	int rv = 0;

	rv += pal_imp_printfln("pal_numa_info=%p, flag=%s", &pal_numa_info, ci_flag_str(pal_numa_flag_to_name, pal_numa_info.flag));
	rv += pal_printfln("nr_numa=%d, nr_cpu=%d, page_size=%d", 
					 pal_numa_info.nr_numa, pal_numa_info.nr_cpu, pal_numa_info.page_size);

	ci_loop(numa_id, pal_numa_info.nr_numa) {
		pal_cpu_map_t *map = &pal_numa_info.sys_cpu_map[numa_id];
		int nr_cpu = pal_cpu_map_count_set(map);

		pal_printf("%d cpus @ numa %d: [ ", nr_cpu, numa_id);
		pal_cpu_map_dump_ex(map, CI_BMP_DUMP_SHOW_EACH_ONLY | CI_BMP_DUMP_SHOW_EACH_COMMA);
		pal_printf(" ]\n");
	}

	ci_loop(numa_id, pal_numa_info.nr_numa) {
		pal_cpu_map_t *map = &pal_numa_info.ci_cpu_map[numa_id];
		int nr_cpu = pal_cpu_map_count_set(map);
		if (!nr_cpu)
			continue;

		pal_printf("ci takes %d cpus @ numa %d: [ ", nr_cpu, numa_id);
		pal_cpu_map_dump_ex(map, CI_BMP_DUMP_SHOW_EACH_ONLY | CI_BMP_DUMP_SHOW_EACH_COMMA);
		pal_printf(" ]\n");
	}	

	ci_loop_i(numa_id, pal_numa_info.nr_numa) {
		ci_mem_range_ex_t *range = &pal_numa_info.range[numa_id];
		if (range->start >= range->end)
			continue;

		int bind_id = pal_numa_id_by_ptr(range->start);
		char ntc = (numa_id == pal_numa_info.nr_numa) || (numa_id == bind_id) ? ' ' : '!';
		
		numa_id != pal_numa_info.nr_numa ? pal_ntc_printf(ntc, "numa_mem %d", numa_id) : pal_ntc_printf(ntc, "shared_mem");


		pal_printf(", " CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(range));
		numa_id != pal_numa_info.nr_numa ? pal_printf(", binding:%d\n", bind_id) : pal_printf(", shared\n");
	}

	return rv;
}

static void pal_numa_detect()
{
#define MASK_BUF_NR						((int)ci_div_ceil(PAL_CPU_TOTAL, sizeof(unsigned long) * 8))
	u64 mask_buf[MASK_BUF_NR];

	pal_numa_info.nr_numa = numa_max_node() + 1;
	pal_numa_info.nr_cpu = numa_num_configured_cpus();
	pal_numa_info.page_size = numa_pagesize();

	ci_assert(pal_numa_info.nr_numa <= PAL_NUMA_NR);
	ci_assert(pal_numa_info.nr_cpu <= PAL_CPU_TOTAL);

#ifdef PAL_RDTSC_OVERHEAD
	PAL_CYCLE_OVERHEAD = PAL_RDTSC_OVERHEAD;
	pal_imp_printf("PAL_CYCLE_OVERHEAD    = %llu (Predefined)\n", PAL_CYCLE_OVERHEAD);
#else
	/* calculate the rdtsc()'s overhead */
	pal_perf_prep();
	u64 start = pal_perf_counter();
	u64 end = pal_perf_counter();
	PAL_CYCLE_OVERHEAD = (end - start) / 2;
	pal_imp_printf("PAL_CYCLE_OVERHEAD    = %llu (Calculated)\n", PAL_CYCLE_OVERHEAD);
#endif

	/* hard code here */
	switch (pal_numa_info.nr_cpu) {
		case 72:	/* our simulator */
			pal_cpu_alloc = pal_cpu_alloc_axnpsim;
			pal_numa_info.flag |= PAL_NUMA_NODE_0_MEM_ALLOC_ONLY;
			PAL_CYCLE_PER_SEC = 2300000000;
			pal_warn_printf("PAL_NUMA_NODE_0_MEM_ALLOC_ONLY, performance compromised\n");
			break;
		case 16:	/* my linux vm */
			pal_cpu_alloc = pal_cpu_alloc_ubuntu;
			break;
		case 40:	/* my windows */
			pal_cpu_alloc = pal_cpu_alloc_winsim;
			break;
		default:
			pal_imp_printfln("no CPU allocation map found, grab all");
			pal_cpu_grab_all = 1;
	}

	/* get the system configuration */
	pal_cpu_map_t *map = &pal_numa_info.sys_cpu_map[0];
	pal_cpu_map_zero(map);

	ci_loop(numa_id, pal_numa_info.nr_numa) {
		pal_cpu_map_t *map = &pal_numa_info.sys_cpu_map[numa_id];
		pal_cpu_map_zero(map);
		ci_memzero(mask_buf, sizeof(mask_buf));

#ifdef WIN_SIM
//		u64_set_range(mask_buf[0], 0, 12);
		struct bitmask mask = { PAL_CPU_TOTAL, (unsigned long long *)mask_buf };
#else
		struct bitmask mask = { PAL_CPU_TOTAL, (unsigned long *)mask_buf };
#endif

		numa_node_to_cpus(numa_id, &mask);
		ci_loop(i, MASK_BUF_NR) 
			u64_each_set(mask_buf[i], bit) 
				pal_cpu_map_set_bit(map, bit + 64 * i);
	}

	/* take my cpus */
	ci_loop(numa_id, pal_numa_info.nr_numa) {
		int last_cpu_id = -1, is_smt0 = 1, cpu_id_step = -1;	/* for smt1 detection, formula from my observation, might not work for dual core smt */
		
		pal_cpu_map_t *sys_map = &pal_numa_info.sys_cpu_map[numa_id];
		pal_cpu_map_t *ci_map = &pal_numa_info.ci_cpu_map[numa_id];
		pal_cpu_map_zero(ci_map);

		pal_cpu_map_each_set(sys_map, cpu_id) {
			int should_add = 0;

			if (last_cpu_id >= 0) {
				if (cpu_id_step < 0)
					cpu_id_step = cpu_id - last_cpu_id;
				if (is_smt0 && (last_cpu_id + cpu_id_step != cpu_id))
					is_smt0 = 0;
			}

			if (pal_cpu_grab_all)
				should_add = 1;
			else
				ci_loop(i, PAL_CPU_TOTAL) 	/* allocation table lookup */
					if (pal_cpu_alloc[numa_id][i] < 0) 
						goto __next_cpu_id;
					else if (pal_cpu_alloc[numa_id][i] == cpu_id) {
						should_add = 1;
						break;
					}

#ifndef CI_SCHED_ENABLE_SMT
			if (!is_smt0)
				should_add = 0;
#endif

			if (should_add) {
				if (!is_smt0)
					pal_numa_info.flag |= PAL_NUMA_SMT;
				
				pal_cpu_map_set_bit(ci_map, cpu_id);
			}
			
__next_cpu_id:
			last_cpu_id = cpu_id;
		}
	}
}

static void pal_numa_init_malloc_info()
{
	pal_malloc_item_t *item;

	pal_printfln("PAL_MEMORY_PER_CPU    = " 	CI_PR_BNP_EX_FMT(3), ci_pr_bnp_val(PAL_MEMORY_PER_CPU));
	pal_printfln("PAL_MEMORY_SHARED     = " 	CI_PR_BNP_EX_FMT(3), ci_pr_bnp_val(PAL_MEMORY_SHARED));
	pal_printfln("PAL_WORKER_STACK_SIZE = " 	CI_PR_BNP_EX_FMT(3), ci_pr_bnp_val(PAL_WORKER_STACK_SIZE));
	pal_printfln("PAL_JIFFIE_PER_SEC    = %i", 	PAL_JIFFIE_PER_SEC);
	pal_printfln("PAL_CYCLE_PER_SEC     = %lli (rdtsc overhead=%lli)", 	PAL_CYCLE_PER_SEC, PAL_CYCLE_OVERHEAD);
	
	ci_loop(numa_id, pal_numa_info.nr_numa) {
		pal_cpu_map_t *map = &pal_numa_info.ci_cpu_map[numa_id];
		int nr_cpu = pal_cpu_map_count_set(map);
		if (!nr_cpu)
			continue;

		item = &pal_malloc_info.item[numa_id];
		item->numa_id = numa_id;
		item->size = (u64)nr_cpu * PAL_MEMORY_PER_CPU;
	}

	item = &pal_malloc_info.item[pal_numa_info.nr_numa];
	item->numa_id = -1;
	item->size = PAL_MEMORY_SHARED;
}

/* might put this function into fp in the future (by passing pal_malloc_info to fp) */
static void pal_numa_allocate_memory()
{
	u8 *ptr;
	ci_mem_range_ex_t *range;

	pal_imp_printf();

	ci_loop_i(numa_id, pal_numa_info.nr_numa) {
		pal_malloc_item_t *item = &pal_malloc_info.item[numa_id];
		
		if (!item->size)
			continue;

		range = &pal_numa_info.range[numa_id];
		if ((item->numa_id < 0) || (pal_numa_info.flag & PAL_NUMA_NODE_0_MEM_ALLOC_ONLY))
			ptr = pal_aligned_malloc(item->size, numa_pagesize());
		else
			ptr = pal_numa_alloc(item->numa_id, item->size);

#ifndef CI_DEBUG	/* if debug, memset happened in pal_malloc family */	
		ci_memset(ptr, 0xEF, item->size);		/* make sure the page is allocated */
#endif
		pal_printf("numa malloc, numa_id=%02d, ptr=%p, " CI_PR_BNP_FMT "%s\n", 
					item->numa_id, ptr, ci_pr_bnp_val(item->size), 
					item->numa_id < 0 ? " [ shared ]" : "");		
		
		range->start = range->curr = ptr;
		range->end = ptr + item->size;
	}
}

static void pal_numa_free_memory()
{
	pal_imp_printf();
	ci_loop_i(numa_id, pal_numa_info.nr_numa) {
		pal_malloc_item_t *item = &pal_malloc_info.item[numa_id];
		ci_mem_range_ex_t *range = &pal_numa_info.range[numa_id];
		u64 mem_size = (u64)(range->end - range->start);

		if (mem_size) {
			pal_printf("numa free, node=%2d, ptr=%p, " CI_PR_BNP_FMT "%s\n", 
						numa_id, range->start, ci_pr_bnp_val(mem_size), 
						item->numa_id < 0 ? " [ shared ]" : "");		
			pal_numa_free(range->start, mem_size);
			range->start = range->end = range->curr = NULL;
		}
	}
}

static u8 *pal_numa_alloc_data(int numa_id, int size, int align)
{
	u8 *rv, *ptr = pal_numa_info.range[numa_id].curr;

	ci_range_check(numa_id, 0, PAL_NUMA_NR);
	ci_assert(ci_is_power_of_two(align));
	ci_assert(ptr);
	
	if (align)
		ptr = ci_ptr_align_upper(ptr, align);

	rv = ptr;
	pal_numa_info.range[numa_id].curr = ptr + size;
	ci_assert(pal_numa_info.range[numa_id].curr <= pal_numa_info.range[numa_id].end);

	return rv;
}

static void pal_numa_init_ci_numa()
{
#ifdef WIN_SIM		/* windows has a different smt1 allocation */
	int cpu_id_adj = 0;
#endif

	pal_imp_printfln("starting pal_worker[] ...");

	/* set up ci_node_info's mem_range */
	ci_mem_range_ex_t *range = &pal_numa_info.range[pal_numa_info.nr_numa];
	ci_assert(range->start && (range->end > range->start));
	ci_node_info->range.start = range->start;
	ci_node_info->range.end = range->end;
	ci_node_map_zero(&ci_node_info->node_com_map);

	ci_loop(numa_id, pal_numa_info.nr_numa) {
		ci_node_t *node;
		pal_cpu_map_t *map = &pal_numa_info.ci_cpu_map[numa_id];
		int nr_cpu = pal_cpu_map_count_set(map);
		if (!nr_cpu)
			continue;

		/* grab all */
		ci_node_map_set_bit(&ci_node_info->node_com_map, ci_node_info->nr_node);	

		/* allocate ci_numa */
		node = ci_node_info->node[ci_node_info->nr_node] 
			 = (ci_node_t *)pal_numa_alloc_data(numa_id, sizeof(ci_node_t), PAL_CPU_CACHE_LINE_SIZE);

		/* init node */
		ci_obj_zero(node);
		node->range.start = ci_ptr_align_upper(pal_numa_info.range[numa_id].curr, PAL_CPU_CACHE_LINE_SIZE);
		node->range.end = pal_numa_info.range[numa_id].end;
		node->node_id = ci_node_info->nr_node++;
		node->numa_id = numa_id;
		ci_worker_map_zero(&node->worker_com_map);

		pal_numa_info.range[numa_id].curr = NULL;		/* no more pal_numa_alloc_data() from this point */

		/* assign worker */
		pal_cpu_map_each_set(map, cpu_id) {
			ci_worker_t *worker = &node->worker[node->nr_worker];
#ifdef WIN_SIM	
			pal_worker_t *pal_worker;

			ci_assert(!(nr_cpu & 0x01));		/* must be even number */
			int pal_cpu_id = node->nr_worker * 2;
			if (pal_cpu_id >= nr_cpu)
				pal_cpu_id -= nr_cpu - 1;
			pal_cpu_id += cpu_id_adj;
			pal_worker = pal_numa_info.worker[pal_cpu_id];
#else
			pal_worker_t *pal_worker = pal_numa_info.worker[cpu_id];
#endif

			worker->node_id = node->node_id;
			worker->worker_id = node->nr_worker;
			worker->pal_worker = pal_worker;
			pal_worker->ci_worker = worker;
			ci_worker_init(worker);

			pal_soft_irq_post_raw(&pal_worker->soft_irq);		/* worker set, call pal_worker to do some init */
			sem_wait(&pal_numa_info.sem_sync);					/* waiting for the worker to enter the "ready" state */

			ci_worker_map_set_bit(&node->worker_com_map, node->nr_worker);	/* take all */
			node->nr_worker++;
		}

#ifdef WIN_SIM
		cpu_id_adj += nr_cpu;
#endif
	}	

	ci_worker_data_register("unused");		/* let's take index 0 for the per-worker data */
}

static void pal_numa_create_worker()
{
	int node_id = 0;
	pthread_key_create(&pal_tls_key, NULL);
	
	ci_loop(numa_id, pal_numa_info.nr_numa) {
		pal_cpu_map_t *map = &pal_numa_info.ci_cpu_map[numa_id];
		int nr_cpu = pal_cpu_map_count_set(map);
		if (!nr_cpu)
			continue;

		pal_cpu_map_each_set(map, cpu_id) {
			/* allocate pal_worker */
			pal_worker_t *worker = pal_numa_info.worker[cpu_id] 		
								 = (pal_worker_t *)pal_numa_alloc_data(numa_id, ci_sizeof(pal_worker_t), PAL_CPU_CACHE_LINE_SIZE);

			ci_obj_zero(worker);
			worker->numa_id = numa_id;
			worker->cpu_id = cpu_id;
			worker->node_id = worker->worker_id = -1;	/* which will be determined later */
			worker->stack = pal_numa_alloc_data(numa_id, PAL_WORKER_STACK_SIZE, PAL_CPU_CACHE_LINE_SIZE);	/* allocate stack */
	
			pal_worker_create(worker);
			sem_wait(&pal_numa_info.sem_sync);
		}

		node_id++;
	}		
}

static void pal_numa_destroy_worker()
{
	ci_node_each(node, {
		pal_printf("node=%d, ptr=%p, nr_worker=%d\n", node->node_id, node, node->nr_worker);
		ci_worker_each(node, worker, {
			pal_worker_t *pw = worker->pal_worker;
			pal_printf("	finz pal_worker < %p, node/cpu=%d/%02d > => worker < %02d, %p >\n", 
						pw, pw->numa_id, pw->cpu_id, worker->worker_id, worker);	

			pal_worker_destroy(pw);
		})
	})


#if 0
	ci_node_each(node, {
		pal_printf("node=%d, ptr=%p, nr_cpu=%d\n", node->node_id, node, node->nr_cpu);
		ci_worker_each(node, worker, {
			pal_worker_t *pw = worker->pal_worker;
			pal_printf("    finz worker < %02d, %p >, pal_worker < %p, node/cpu=%d/%02d >\n", 
						worker->worker_id, worker, pw, pw->numa_id, pw->cpu_id);	

/*			
			pw->flag |= PAL_WORKER_QUIT;
			pal_soft_irq_post(&pw->soft_irq);
 */			

			pthread_cancel(pw->thread);
			pthread_join(pw->thread, NULL);
		})
	})
#endif
}

/* change the com map */
static void pal_numa_config_com()
{
#if 0
	ci_node_map_clear_bit(&ci_node_info->node_com_map, 0);	/* remove node 0 */
	ci_node_each(node, {
		ci_worker_map_clear_bit(&node->worker_com_map, 0);	/* remove worker 0 */
	});
#endif	
}

int pal_numa_init()
{
	sem_init(&pal_numa_info.sem_sync, 0, 0);

	pal_numa_detect();
	pal_numa_init_malloc_info();
	pal_numa_allocate_memory();
	pal_numa_dump();
	
	pal_numa_create_worker();
	pal_numa_init_ci_numa();
	pal_numa_config_com();

	return 0;
}

int pal_numa_finz()
{
	pal_numa_destroy_worker();
	pal_numa_free_memory();

	return 0;
}




