/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_numa.h				numa configurations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"

#include "pal_type.h"
#include "ci_bmp.h"
#include "ci_util.h"
#include "ci_type.h"
#include "pal_worker.h"


ci_bmp_def(pal_cpu_map, PAL_CPU_TOTAL);
#define pal_cpu_map_each_set(bmp, idx)		ci_bmp_each_set(bmp, PAL_CPU_TOTAL, idx)	

typedef struct {
	int							 flag;
#define PAL_NUMA_SMT						0x0001				/* will use smt1 */
#define PAL_NUMA_NODE_0_MEM_ALLOC_ONLY		0x0002				/* only alloc memory on node 0, bad axnpsim config! */
	
	int							 nr_numa;
	int							 nr_cpu;
	int							 page_size;						/* numa page size */
	u32							 node_map;						/* which numa node will goes to ci_numa */
	ci_mem_range_ex_t	 		 range[PAL_NUMA_NR];			/* [start, end) */
	pal_cpu_map_t			 	 sys_cpu_map[PAL_NUMA_NR];		/* system: which CPUs belong to this node */
	pal_cpu_map_t				 ci_cpu_map[PAL_NUMA_NR];		/* mine */

	sem_t						 sem_sync;						/* sync purpose */
	pal_worker_t				*worker[PAL_CPU_TOTAL];			/* in their own numa memory range */
	u8							*worker_stack[PAL_CPU_TOTAL];		/* stack */
} pal_numa_info_t;


typedef struct {
	int							 numa_id;						/* -1 means interleaved */
	u64							 size;							/* what'sthe size */
	u8							*start;							
	u8							*end;
} pal_malloc_item_t;

typedef struct {
	pal_malloc_item_t			 item[PAL_NUMA_NR + 1];			/* last one for interleave, end of table */	
} pal_malloc_info_t;


extern pal_numa_info_t			 pal_numa_info;
extern pal_malloc_info_t		 pal_malloc_info;


int pal_numa_init();
int pal_numa_finz();
int pal_numa_node_0_mem_alloc_only();





