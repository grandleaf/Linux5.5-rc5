/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * perf.h			Performance Evaluation Module
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"


#define perf_info(mod)					((perf_info_t *)((mod)->mem.range_shr.start))
#define perf_task_by_ctx(ctx)			ci_container_of((ctx)->sched_ent, perf_task_t, sched_task.ent)	

typedef struct {
	int						 test_time;		/* in msec */
	int						 queue_depth;	/* for each worker */
	ci_node_map_t			 node_map;
	ci_worker_map_t			 worker_map[CI_NODE_NR];
} perf_cfg_t;

typedef struct {
	int						 stop;
	int						 queue_depth;	/* for each worker */
	int						 total;	
	volatile int			 pending;

	ci_list_t				 head;
	ci_perf_data_t			 data;
} perf_data_t;

typedef struct {
	u64						 cnt;
	ci_sched_task_t			 sched_task;
	ci_sched_grp_t			*sched_grp;
	ci_list_t				 link;
} perf_task_t;

#if 0
typedef struct {
	int					 sd;		/* schedule descriptor for "perf.io" */
} perf_info_t;
#endif

void perf_test_start();

