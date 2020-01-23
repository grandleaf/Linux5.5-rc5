/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_sta.h					CI Statistics
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#include "ci_type.h"
#include "ci_list.h"
#include "ci_timer.h"


typedef struct __ci_sta_t ci_sta_t;

typedef struct {
	ci_list_t					 link;			
	u64							 start;			/*time stamp for start & end */
	u64							 end;
} ci_sta_link_t;

typedef struct {
	const char					*name;
	int							 interval;		/* in ms */
	int							 depth;			/* how many */
} ci_sta_cfg_hist_t;

typedef struct {
	const char 					*name;
	int 					 	 offset;
} ci_sta_dump_dpt_t;

typedef struct {
	int							 flag;			/* dump flags */
	int							 dpt_name_max;	/* max len of dpt names */
	ci_timer_t					 timer;			/* dump timer if needed */
} ci_sta_dump_data_t;

typedef struct {
	int							 node_id;
	int							 worker_id;
	
	const char					*name;
	const char 				   **trip_name;
	ci_sta_cfg_hist_t			*hist;

	int							 pending_size;	/* for pending data structure */
	int							 acc_size;		/* accumulator, for odometer, trip, hist */
} ci_sta_cfg_t;

typedef struct {
	const char					*name;			/* trip name */

	int flag;
#define CI_STA_ACCF_DUMP			0x0001		/* enable periodic dump */
#define CI_STA_ACCF_DUMP_PAUSE		0x0002		/* pause the dump */

	ci_sta_dump_dpt_t			*dump_dpt;		/* dump descriptor */
	ci_sta_dump_data_t			 dump_data;		/* data meta data */
		
	u8							*buf;			/* user object start */
	int							 buf_size;		/* accumulator or pending */
	ci_timer_t 					 timer;			/* used for hist only */

	ci_sta_t					*sta;			/* pointer to parent */
	ci_list_t					 head;			/* head for acc */
	ci_list_t					 link;			/* chain to the ci_sta_t */
} ci_sta_acc_t;

typedef struct {
	int						 	 total;
	int						 	 acc_alloc_size;			/* ci_sta_link_t + padding + user object size */

	int						 	 trip_nr;					/* trip[N] */
	int						 	 trip_size;					/* sizeof(trip[0]) * N */
	int						 	 trip_acc_size;				/* acc[N] */

	int							 hist_nr;					/* hist[M] */
	int						 	 hist_size;					/* sizeof(hist[0]) * M */
	int						 	 hist_depth;				/* sum(hist[i]->head) */
	int						 	 hist_acc_size;				/* acc[M] * depth */
} ci_sta_eval_t;

struct __ci_sta_t {
	const char					*name;
	int							 node_id;
	int							 worker_id;
	int							 offset;		/* link | gap | accu_data, offset = link + gap */
	int							 acc_size;		/* user acc object size */

	ci_sta_acc_t				 pending;
	ci_sta_acc_t				 odometer;

	ci_sta_acc_t				*trip;			/* current trip */
	ci_list_t					 trip_head;
	ci_list_t					 hist_head;
};


#define ci_sta_acc_inc(sta, offset)				ci_sta_acc_add(sta, offset, 1)
#define ci_sta_pending_inc(sta, offset)			ci_sta_pending_add(sta, offset, 1)
#define ci_sta_pending_dec(sta, offset)			ci_sta_pending_sub(sta, offset, 1)


#define ci_sta_offset_check(sta, offset)	\
	do {	\
		ci_range_check(offset, 0, (sta)->acc_size);		\
		ci_align_check(offset, ci_sizeof(u64));		\
	} while (0)

#define ci_sta_acc_add_no_ctx(sta, offset, val)		/* might call this in init stage without context */	\
	({	\
		ci_sta_offset_check(sta, offset);	\
		__ci_sta_acc_add(sta, offset, val);		\
		sta;	\
	})
#define ci_sta_acc_add(sta, offset, val)		\
	({	\
		ci_sched_ctx_check((sta)->node_id, (sta)->worker_id);	\
		ci_sta_acc_add_no_ctx(sta, offset, val);	\
	})


#define ci_sta_acc_max_no_ctx(sta, offset, val)		\
	({	\
		ci_sta_offset_check(sta, offset);	\
		__ci_sta_acc_max(sta, offset, val);		\
		sta;	\
	})
#define ci_sta_acc_max(sta, offset, val)		\
	({	\
		ci_sched_ctx_check((sta)->node_id, (sta)->worker_id);	\
		ci_sta_acc_max_no_ctx(sta, offset, val);	\
	})
	
#define ci_sta_acc_min_no_ctx(sta, offset, val)		\
	({	\
		ci_sta_offset_check(sta, offset);	\
		__ci_sta_acc_min(sta, offset, val);		\
		sta;	\
	})
#define ci_sta_acc_min(sta, offset, val)		\
	({	\
		ci_sched_ctx_check((sta)->node_id, (sta)->worker_id);	\
		ci_sta_acc_min_no_ctx(sta, offset, val);	\
	})
	
#define ci_sta_pending_add(sta, offset, val)		\
	({	\
		int *__sta_pending_ptr__ = (int *)((u8 *)(sta)->pending + (offset));	\
		\
		ci_sta_offset_check(sta, offset);	\
		ci_sched_ctx_check((sta)->node_id, (sta)->worker_id);	\
		ci_assert((sta)->pending);	\
		ci_assert(*__sta_pending_ptr__ >= 0);	\
		ci_assert((val) > 0);	\
		*__sta_pending_ptr__ += (val);	\
		sta;	\
	})
#define ci_sta_pending_sub(sta, offset, val)		\
	({	\
		int *__sta_pending_ptr__ = (int *)((u8 *)(sta)->pending + (offset));	\
		\
		ci_sta_offset_check(sta, offset);	\
		ci_sched_ctx_check((sta)->node_id, (sta)->worker_id);	\
		ci_assert((sta)->pending);	\
		ci_assert(*__sta_pending_ptr__ > 0);	\
		ci_assert((val) > 0);	\
		*__sta_pending_ptr__ -= (val);	\
		ci_assert(*__sta_pending_ptr__ >= 0);	\
		sta;	\
	})

/* for statistics dumping */
#define __ci_sta_dump_dpt_maker_helper(type, name)		{ #name, ci_offset_of(type, name) },
#define ci_sta_dump_dpt_maker(type, ...)	\
	{	\
		ci_m_each_m1(__ci_sta_dump_dpt_maker_helper, type, __VA_ARGS__)	\
		CI_EOT	\
	}	



ci_sta_t *ci_sta_create(ci_sta_cfg_t *cfg);
int  ci_sta_last_hist_val(ci_sta_t *sta, const char *name, int offset, u64 *val);
void __ci_sta_acc_add(ci_sta_t *sta, int offset, u64 val);
void __ci_sta_acc_max(ci_sta_t *sta, int offset, u64 val);
void __ci_sta_acc_min(ci_sta_t *sta, int offset, u64 val);
int  ci_sta_set_hist_dump(ci_sta_t *sta, const char *hist_name, ci_sta_dump_dpt_t *dpt, int immediate, int dump_flag);


