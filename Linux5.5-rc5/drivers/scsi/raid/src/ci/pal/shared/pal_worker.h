/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_worker.h				PAL Workers
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_type.h"
#include "pal_soft_irq.h"
#include "pal_rand.h"


/* avoid using this in the io path(performance impact) */
#define ci_sched_ctx()			((ci_sched_ctx_t *)pthread_getspecific(pal_tls_key))
#define ci_worker_mod()				\
	({	\
		ci_sched_ctx_t *__ctx__ = ci_sched_ctx();	\
		__ctx__ && __ctx__->sched_grp ? __ctx__->sched_grp->mod : NULL;		\
	})


typedef struct {
	int							 flag;
#define PAL_WORKER_QUIT					0x0001					/* tell the worker to quit */

	int							 numa_id;						/* pal */
	int							 cpu_id;						/* pal */
	u64							 thread_id;						/* pal */
	
	int							 node_id;						/* ci */
	int							 worker_id;						/* ci */

	volatile int				*count_down;					/* synchronization */

	u8							*stack;							/* lower address, size is PAL_WORKER_STACK_SIZE */
	u8							*stack_curr;					/* current stack */
	
	pthread_attr_t				 attr;	
	pal_soft_irq_t				 soft_irq;
	pthread_t					 thread;

	pal_rand_ctx_t				 rand_ctx;
	struct __ci_worker_t		*ci_worker;
} pal_worker_t;


extern pthread_key_t 			 pal_tls_key;


int pal_worker_create(pal_worker_t *worker);
int pal_worker_destroy(pal_worker_t *worker);
u8 *pal_worker_current_stack(pal_worker_t *worker);
int pal_worker_stack_usage(pal_worker_t *worker);

const char *ci_worker_mod_name();	


