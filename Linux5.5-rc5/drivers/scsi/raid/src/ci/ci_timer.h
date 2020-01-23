/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_timer.h								Timer Service
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_list.h"
#include "ci_sched.h"


typedef struct {
	/* user set */
	int							 flag;
#define CI_TIMER_PERIODIC			0x0001
	
	u64							 msec;						/* in millisecond, 1000 = 1second */
	void			   			(*callback)(ci_sched_ctx_t *, void *);		/* callback with ctx & data */
	void						*data;

	/* internal use */
	u64							 jiffie;					/* internal use: when to time out */
	u64					 		 jiffie_expire;				/* informative, debug purpose only */
	ci_list_t					 link;						/* chained into wheel's head */

#ifdef CI_DEBUG	
	ci_sched_ctx_t				*ctx_add;					/* panic if the ctx_del != ctx_add */
	ci_access_tag;											/* caller monitor */
#endif
} ci_timer_t;

typedef struct {
	int							 id;
	int							 curr;						/* curr head to check */
	u64					 	 	 jiffie_adj;				/* when add a timer, + jiffie_adj */
	ci_list_t			 	 	 head[256];
} ci_timer_wheel_t;

typedef struct {
	u64							 jiffie;					/* each worker has its local jiffie */
	volatile int 				 jiffie_pending;			/* pending jiffies to process */
	ci_timer_wheel_t		 	 wheel[CI_TIMER_WHEEL_NR];

	ci_sched_ent_t				 sched_ent;					/* take care of timer service */
	ci_sched_grp_t				*sched_grp;					/* saved sched group */
	ci_sched_ctx_t				*sched_ctx;					/* saved schedule context */
	pal_timer_intr_handler_t	 intr_handler;				/* connect to pal_timer */

	ci_list_t					 ext_head;					/* external timer head */
	ci_slk_t					 lock;						/* for external timer */
} ci_timer_worker_data_t;


#define ci_timer_add(ctx, timer)		\
	do {	\
		ci_assert((ctx) && (timer));	\
		ci_assert((ctx) == ci_sched_ctx(), "please use the safe/slow version: ci_timer_ext_add()");	\
		ci_access_tag_set(timer);		\
		__ci_timer_add(ctx, timer);		\
	} while (0)

/*
 *	add from external without a worker context, pretty rare 
 *	there's no panic here, since a external context might happened be in the same context
 */
#define ci_timer_ext_add(ctx, timer)		\
	do {	\
		ci_assert((ctx) && (timer));	\
		ci_access_tag_set(timer);		\
		__ci_timer_ext_add(ctx, timer);	\
	} while (0)

/* must call this in the scheduler's context */
#define ci_timer_del(timer)		\
	do {	\
		ci_assert(timer);		\
		ci_assert((timer)->ctx_add == ci_sched_ctx(), "timer deletion not in scheduler's context");	\
		ci_list_del(&(timer)->link);	\
	} while (0)


/* internal use only */
void ci_timer_mod_init(ci_mod_t *mod);	
void __ci_timer_add(ci_sched_ctx_t *ctx, ci_timer_t *timer);
void __ci_timer_ext_add(ci_sched_ctx_t *ctx, ci_timer_t *timer);

	

