/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_node.h					CI Node
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#include "ci_balloc.h"
#include "ci_halloc.h"
#include "ci_macro.h"
#include "ci_printf.h"
#include "ci_sched.h"
#include "ci_type.h"
#include "ci_util.h"
#include "ci_worker.h"

struct __ci_node_t {
	int							 node_id;			/* logical node id, [0, N), no hole */
	int							 numa_id;			/* physical numa id, might have hole */
	int							 nr_worker;			/* equal to the number of CPU allocated for our use */
	
	ci_mem_range_t				 range;
	ci_halloc_t					 ha;				/* local node heap allocator */
	ci_balloc_t					 ba;				/* local node buddy system allocator */
	ci_balloc_t					 ba_paver;			/* local node buddy system allocator for pavers */

	ci_worker_map_t				 worker_com_map;	/* common worker map, default for all modules */
	ci_worker_t					 worker[CI_WORKER_NR];
};

typedef struct {
	int							 flag;
	int							 nr_node;
	ci_node_map_t				 node_com_map;		/* common node map for all modules */
	ci_node_t					*node[CI_NODE_NR];

	ci_mem_range_t				 range;
	ci_halloc_t					 ha_shr;			/* shared heap allocator */
	ci_balloc_t					 ba_shr;			/* shared buddy system allocator */
	ci_balloc_t					 ba_json;			/* shared buddy system allocator for json use */

	const char					*worker_data_name[CI_WORKER_DATA_NR];
	ci_sched_dpt_map_t			 sched_dpt_map;
	
	ci_slk_t					 lock;
} ci_node_info_t;


/*
 *	ci node family 
 */
#define ci_node_each(node_ptr, ...)		\
	ci_loop(__node_id__, ci_node_info->nr_node) {	\
		ci_node_t *node_ptr = ci_node_info->node[__node_id__];	\
		ci_assert(node_ptr);	\
		\
		__VA_ARGS__;	\
	}
#define ci_node_id_each(node_id)	\
	ci_loop(node_id, ci_node_info->nr_node)
#define ci_worker_each(node_ptr, worker_ptr, ...)	\
	ci_loop(__worker_id__, (node_ptr)->nr_worker) {		\
		ci_worker_t *worker_ptr = &(node_ptr)->worker[__worker_id__];	\
		__VA_ARGS__;		\
	}	
#define ci_node_worker_each(node_ptr, worker_ptr, ...)		\
	ci_node_each(node_ptr, {	\
		ci_worker_each(node_ptr, worker_ptr, {	\
			__VA_ARGS__;	\
		})	\
	})
#define ci_node_by_id(node_id)		\
	({	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);	\
		ci_node_info->node[node_id];	\
	})
#define ci_node_worker_id_by_ctx(ctx, __node_id, __worker_id)		\
	do {	\
		*(__node_id) = (ctx)->worker->node_id;		\
		*(__worker_id) = (ctx)->worker->worker_id;	\
	} while (0)	
	

/*
 *	ci_shr_halloc family 
 */
#define ci_shr_halloc(size, align, name)		ci_halloc(&ci_node_info->ha_shr, size, align, name)
#define ci_shr_halloc_dump()					ci_halloc_dump(&ci_node_info->ha_shr)


/*
 *	ci_node_halloc family
 */
#define ci_node_halloc(node_id, size, align, name)		\
	({	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);		\
		ci_halloc(&ci_node_info->node[node_id]->ha, size, align, name);	\
	})
#define ci_node_halloc_dump(node_id)		\
	({	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);		\
		ci_halloc_dump(&ci_node_info->node[node_id]->ha);		\
	})


/*
 *	ci_shr_balloc family
 */
#define ci_shr_balloc(size)						ci_exec_ptr_not_nil(ci_balloc(&ci_node_info->ba_shr, size))
#define ci_shr_bfree(ptr)						ci_bfree(&ci_node_info->ba_shr, ptr)
#define ci_shr_balloc_dump()					ci_balloc_dump(&ci_node_info->ba_shr)


/*
 *	ci_node_balloc family
 */
#define ci_node_balloc(node_id, size)	\
	({	\
		void *__node_balloc_ptr__;	\
		\
		ci_range_check(node_id, 0, ci_node_info->nr_node);	\
		__node_balloc_ptr__ = ci_balloc(&ci_node_info->node[node_id]->ba, size);	\
		ci_assert(__node_balloc_ptr__, "out of memory, please increase CI_BALLOC_NODE_SIZE.");	\
		\
		__node_balloc_ptr__;	\
	})
#define ci_node_bfree(node_id, ptr)		\
	do {	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);	\
		ci_bfree(&ci_node_info->node[node_id]->ba, ptr);	\
	} while (0)

/*
 *	chores
 */
#define ci_node_worker_id_check(node_id, worker_id)		\
	do {	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);	\
		ci_range_check(worker_id, 0, ci_node_info->node[node_id]->nr_worker);	\
	} while (0)
 

int ci_node_pre_init();
int ci_node_init();
int ci_node_info_dump();



