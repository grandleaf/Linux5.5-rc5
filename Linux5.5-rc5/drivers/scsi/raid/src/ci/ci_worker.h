/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_worker.h				CI Workers
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_sched.h"
#include "ci_sta.h"

#ifdef CI_WORKER_STA
typedef struct {
	u64							 busy_cycle;
} ci_worker_sta_acc_t;
#endif	/* ! CI_WORKER_STA */

typedef struct __ci_worker_t {
	int							 node_id;		
	int							 worker_id;		

	ci_sched_tab_t				 sched_tab;		/* schedule table */

	int (*do_work)(struct __ci_worker_t *);
	pal_worker_t				*pal_worker;	

	void			 			*data[CI_WORKER_DATA_NR];		/* per worker data */

#ifdef CI_WORKER_STA
	ci_sta_t					*sta;
#endif
} ci_worker_t;


/* frequently used */
#define ci_worker_by_id(node_id, worker_id)		\
	({	\
		ci_range_check(node_id, 0, ci_node_info->nr_node);	\
		ci_range_check(worker_id, 0, ci_node_info->node[node_id]->nr_worker);	\
		&ci_node_info->node[node_id]->worker[worker_id];	\
	})
#define ci_worker_data_by_ctx(ctx, wdid)	\
	({	\
		ci_assert((ctx) && (ctx)->worker);	\
		ci_range_check(wdid, 1, CI_WORKER_DATA_NR);		\
		ci_assert((ctx)->worker->data[wdid]);	\
		(ctx)->worker->data[wdid];		\
	})
#define ci_worker_data_by_ctx_not_nil(ctx, wdid)	\
	({	\
		void *__worker_data_by_ctx__ = ci_worker_data_by_ctx(ctx, wdid);		\
		ci_assert(__worker_data_by_ctx__);	\
		__worker_data_by_ctx__;	\
	})
#define ci_worker_data_by_id(node_id, worker_id, wdid)	\
	ci_worker_by_id(node_id, worker_id)->data[wdid]
#define ci_worker_data_by_id_not_nil(node_id, worker_id, wdid)	\
	({	\
		void *__worker_data_by_id__ = ci_worker_by_id(node_id, worker_id)->data[wdid];		\
		ci_assert(__worker_data_by_id__);	\
		__worker_data_by_id__;	\
	})


/* internal use */
#define ci_worker_soft_irq(worker)		\
	do {	\
		ci_assert((worker) && (worker)->pal_worker);	\
		pal_soft_irq_post(&(worker)->pal_worker->soft_irq);	\
	} while (0)


#ifdef CI_WORKER_STA
#define ci_worker_sta_acc_add(worker, name, val)		((worker)->sta && ci_sta_acc_add((worker)->sta, ci_offset_of(ci_worker_sta_acc_t, name), val))
#else
#define ci_worker_sta_acc_add(worker, name, val)		ci_nop()
#endif	


/* exported functions */
int  ci_worker_init(ci_worker_t *worker);
int  ci_worker_post_init();
int  ci_worker_data_register(const char *name);	
void ci_worker_data_set(ci_sched_ctx_t *ctx, int wdid, void *data);



