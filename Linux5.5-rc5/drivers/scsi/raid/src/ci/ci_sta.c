/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_sta.c					CI Statistics
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"


#define STA_INIT_PTN_U8			 0xFE		/* -1 is possible to happen if user dec incorrectly */
#define STA_INIT_PTN_U64		 0xFEFEFEFEFEFEFEFE


#define __ci_sta_u64_add(ptr, val)		\
	({	\
		u64 *__u64_ptr__ = (u64 *)(ptr);	\
		u64 __u64_val__ = (u64)(val);		\
		*__u64_ptr__ = *__u64_ptr__ != STA_INIT_PTN_U64 ? *__u64_ptr__ + __u64_val__ : __u64_val__;	\
	})
#define __ci_sta_u64_sub(ptr, val)		\
	({	\
		u64 *__u64_ptr__ = (u64 *)(ptr);	\
		u64 __u64_val__ = (u64)(val);		\
		*__u64_ptr__ = *__u64_ptr__ != STA_INIT_PTN_U64 ? *__u64_ptr__ - __u64_val__ : __u64_val__;	\
	})
#define __ci_sta_u64_set_max(ptr, val)		\
	({	\
		u64 *__u64_ptr__ = (u64 *)(ptr);	\
		u64 __u64_val__ = (u64)(val);		\
		*__u64_ptr__ = *__u64_ptr__ != STA_INIT_PTN_U64 ? ci_max(*__u64_ptr__, __u64_val__) : __u64_val__;	\
	})
#define __ci_sta_u64_set_min(ptr, val)		\
	({	\
		u64 *__u64_ptr__ = (u64 *)(ptr);	\
		u64 __u64_val__ = (u64)(val);		\
		*__u64_ptr__ = *__u64_ptr__ != STA_INIT_PTN_U64 ? ci_min(*__u64_ptr__, __u64_val__) : __u64_val__;	\
	})



static void ci_sta_single_dump(ci_sta_t *sta, void *buf, ci_sta_dump_dpt_t *dpt, ci_sta_dump_data_t *data);


static void ci_sta_cfg_check(ci_sta_cfg_t *cfg)
{
	ci_node_worker_id_check(cfg->node_id, cfg->worker_id);
	ci_assert(cfg->name && ci_strlen(cfg->name));
	ci_align_check(cfg->pending_size, ci_sizeof(u64), "pending_size must be multiple of sizeof(u64)");
	ci_align_check(cfg->acc_size, ci_sizeof(u64), "acc_size must be multiple of sizeof(u64)");

#ifdef CI_DEBUG
	ci_sta_cfg_hist_t *cfg_hist;

	for (cfg_hist = cfg->hist; cfg_hist->name; cfg_hist++) {
		ci_assert(ci_strlen(cfg_hist->name));
		ci_assert(cfg_hist->depth >= 2);
		ci_assert(cfg_hist->interval >= 1);
	}	
#endif	
}

static int ci_sta_eval_alloc_size(ci_sta_cfg_t *cfg, ci_sta_eval_t *eval)
{
#define ci_sta_alloc_size_inc(total, size)	\
	do {	\
		if (size) {	\
			(total) += (size);	\
			ci_align_cpu_asg(total);	\
		}	\
	} while (0)

	ci_sta_cfg_hist_t *cfg_hist;

	eval->acc_alloc_size = ci_align_cpu(ci_sizeof(ci_sta_link_t)) + cfg->acc_size;
	
	ci_sta_alloc_size_inc(eval->total, ci_sizeof(ci_sta_t));	
	ci_sta_alloc_size_inc(eval->total, cfg->pending_size);		/* might be zero */
	ci_sta_alloc_size_inc(eval->total, eval->acc_alloc_size);	/* for odometer, always there */

	eval->trip_nr = ci_get_argc(cfg->trip_name);
	ci_sta_alloc_size_inc(eval->total, ci_sizeof(ci_sta_acc_t) * eval->trip_nr);	/* the control data structure */
	ci_sta_alloc_size_inc(eval->total, eval->acc_alloc_size * eval->trip_nr);		/* each trip one acc */

	for (cfg_hist = cfg->hist; cfg_hist->name; cfg_hist++) {
		eval->hist_nr++;
		eval->hist_depth += cfg_hist->depth;
	}
	ci_sta_alloc_size_inc(eval->total, ci_sizeof(ci_sta_acc_t) * eval->hist_nr);			
	ci_sta_alloc_size_inc(eval->total, eval->acc_alloc_size * eval->hist_nr * eval->hist_depth);		

	return eval->total;
}

static void ci_sta_hist_timer_callback(ci_sched_ctx_t *ctx, void *data)
{
	ci_sta_link_t *old_link, *new_link;
	ci_sta_acc_t *hist_acc = data;
	ci_sta_t *sta = hist_acc->sta;

	/* move first to end */
	old_link = ci_container_of(ci_list_del_head(&hist_acc->head), ci_sta_link_t, link);
	ci_list_add_tail(&hist_acc->head, &old_link->link);

	/* init the first obj */
	new_link = ci_container_of(ci_list_head(&hist_acc->head), ci_sta_link_t, link);
	hist_acc->buf = (u8 *)new_link + sta->offset;
	ci_memset(hist_acc->buf, STA_INIT_PTN_U8, sta->acc_size);

	/* do periodic dump */
	if ((hist_acc->flag & CI_STA_ACCF_DUMP) && !(hist_acc->flag & CI_STA_ACCF_DUMP_PAUSE) && !hist_acc->dump_data.timer.msec)
		ci_sta_single_dump(sta, (u8 *)old_link + sta->offset, hist_acc->dump_dpt, &hist_acc->dump_data);
		
//	ci_printfln(CI_PRN_FMT_NODE_WORKER ", %s, %llu, obj=%p", CI_PRN_VAL_NODE_WORKER, hist_acc->name, hist_acc->timer.msec, link);
}

static void ci_sta_init(ci_sta_t *sta, ci_sta_cfg_t *cfg, ci_sta_eval_t *eval)
{
#define ci_sta_alloc_inc(ptr, size)	\
	({	\
		void *__old_ptr__ = ptr;	\
		if (size) {	\
			(ptr) = (u8 *)(ptr) + (size);	\
			ci_ptr_align_cpu_asg(ptr);	\
		}	\
		__old_ptr__;	\
	})

	ci_timer_t *tm;
	ci_sta_cfg_hist_t *cfg_hist;
	u8 *ptr, *trip_obj, *hist_obj;
	ci_sta_acc_t *trip_acc, *hist_acc;


	/* 1. init sta */
	ci_memzero(sta, eval->total);
	sta->name		= cfg->name;
	sta->node_id 	= cfg->node_id;
	sta->worker_id 	= cfg->worker_id;
	sta->acc_size 	= cfg->acc_size;
	sta->offset 	= eval->acc_alloc_size - sta->acc_size;
	ci_list_init(&sta->trip_head);
	ci_list_init(&sta->hist_head);

	/* 2. do memory allocation */
	ptr = (u8 *)sta;
	ci_sta_alloc_inc(ptr, ci_sizeof(ci_sta_t));
	sta->pending.buf_size	= cfg->pending_size;
	sta->pending.buf		= ci_sta_alloc_inc(ptr, sta->pending.buf_size);
	
	sta->odometer.buf_size	= sta->acc_size; 
	sta->odometer.buf		= ci_sta_alloc_inc(ptr, eval->acc_alloc_size) + sta->offset; 
	ci_memset(sta->odometer.buf, STA_INIT_PTN_U8, sta->odometer.buf_size);

	trip_acc 		= ci_sta_alloc_inc(ptr, ci_sizeof(ci_sta_acc_t) * eval->trip_nr);
	trip_obj 		= ci_sta_alloc_inc(ptr,  eval->acc_alloc_size * eval->trip_nr);

	hist_acc 		= ci_sta_alloc_inc(ptr, ci_sizeof(ci_sta_acc_t) * eval->hist_nr);
	hist_obj 		= ci_sta_alloc_inc(ptr, eval->acc_alloc_size * eval->hist_nr * eval->hist_depth);

	ci_assert((u8 *)ptr == (u8 *)sta + eval->total);

	/* 3. init trip */
	ci_argv_each(cfg->trip_name, name, {
		trip_acc->name 		= *name;
		trip_acc->sta 		= sta;
		trip_acc->buf 		= trip_obj + sta->offset;
		trip_acc->buf_size 	= sta->acc_size;
		ci_memset(trip_acc->buf, STA_INIT_PTN_U8, trip_acc->buf_size);
		ci_list_add_tail(&sta->trip_head, &trip_acc->link);
		
		(!sta->trip) && (sta->trip = trip_acc);		/* set current trip */
		trip_acc++;
		trip_obj += eval->acc_alloc_size;
	});			

	/* 4. init hist */
	for (cfg_hist = cfg->hist; cfg_hist->name; cfg_hist++) {
		ci_list_init(&hist_acc->head);
		hist_acc->name 		= cfg_hist->name;
		hist_acc->sta 		= sta;
		hist_acc->buf 		= hist_obj + sta->offset;
		hist_acc->buf_size 	= sta->acc_size;
		ci_memset(hist_acc->buf, STA_INIT_PTN_U8, hist_acc->buf_size);

		ci_loop(cfg_hist->depth) {
			ci_list_add_tail(&hist_acc->head, &((ci_sta_link_t *)hist_obj)->link);
			hist_obj += eval->acc_alloc_size;
		}

		tm = &hist_acc->timer;
		tm->flag = CI_TIMER_PERIODIC;
		tm->msec = cfg_hist->interval;
		tm->data = hist_acc;
		tm->callback = ci_sta_hist_timer_callback;
		ci_timer_ext_add(ci_sched_ctx_by_id(cfg->node_id, cfg->worker_id), tm);

		ci_list_add_tail(&sta->hist_head, &hist_acc->link);
		hist_acc++;
	}
}

ci_sta_t *ci_sta_create(ci_sta_cfg_t *cfg)
{
	ci_sta_t *sta;
	ci_sta_eval_t eval = {};
	
	ci_sta_cfg_check(cfg);
	ci_sta_eval_alloc_size(cfg, &eval);
	
	sta = ci_node_balloc(cfg->node_id, eval.total);
	ci_sta_init(sta, cfg, &eval);
	
	return sta;
}

void __ci_sta_acc_add(ci_sta_t *sta, int offset, u64 val)
{
	ci_sta_acc_t *acc;

	__ci_sta_u64_add(sta->odometer.buf + offset, val);

	if (sta->trip) {
		ci_assert(sta->trip->buf);
		__ci_sta_u64_add(sta->trip->buf + offset, val);
	}

	ci_list_each(&sta->hist_head, acc, link) {
		ci_assert(acc->buf);
		__ci_sta_u64_add(acc->buf + offset, val);
	}
}

void __ci_sta_acc_max(ci_sta_t *sta, int offset, u64 val)
{
	ci_sta_acc_t *acc;

	__ci_sta_u64_set_max(sta->odometer.buf + offset, val);

	if (sta->trip) {
		ci_assert(sta->trip->buf);
		__ci_sta_u64_set_max(sta->trip->buf + offset, val);
	}

	ci_list_each(&sta->hist_head, acc, link) {
		ci_assert(acc->buf);
		__ci_sta_u64_set_max(acc->buf + offset, val);
	}
}

void __ci_sta_acc_min(ci_sta_t *sta, int offset, u64 val)
{
	ci_sta_acc_t *acc;

	__ci_sta_u64_set_min(sta->odometer.buf + offset, val);

	if (sta->trip) {
		ci_assert(sta->trip->buf);
		__ci_sta_u64_set_min(sta->trip->buf + offset, val);
	}

	ci_list_each(&sta->hist_head, acc, link) {
		ci_assert(acc->buf);
		__ci_sta_u64_set_min(acc->buf + offset, val);
	}
}

static ci_sta_acc_t *ci_sta_hist_acc_by_name(ci_sta_t *sta, const char *name)
{
	ci_sta_acc_t *acc;

	ci_list_each(&sta->hist_head, acc, link) 
		if (ci_strequal(acc->name, name))
			return acc;

	return NULL;
}

int ci_sta_last_hist_val(ci_sta_t *sta, const char *name, int offset, u64 *val)
{
	ci_sta_acc_t *acc;
	ci_sta_link_t *link;

	ci_sta_offset_check(sta, offset);
	if (!(acc = ci_sta_hist_acc_by_name(sta, name)))
		return -CI_E_NOT_FOUND;

	link = ci_container_of(ci_list_tail(&acc->head), ci_sta_link_t, link);
	*val = *(u64 *)((u8 *)link + sta->offset);

	return *val == STA_INIT_PTN_U64 ? -CI_E_UNINITIALIZED : 0;
}

int ci_sta_set_hist_dump(ci_sta_t *sta, const char *hist_name, ci_sta_dump_dpt_t *dpt, int immediate, int dump_flag)
{
	ci_sta_acc_t *acc;

	if (!(acc = ci_sta_hist_acc_by_name(sta, hist_name)))
		return -CI_E_NOT_FOUND;

	acc->dump_dpt = dpt;
	acc->flag |= CI_STA_ACCF_DUMP;
	acc->dump_data.flag = dump_flag;

	if (!immediate)
		acc->flag |= CI_STA_ACCF_DUMP_PAUSE;

	for (ci_sta_dump_dpt_t *p = dpt; p->name; p++) {
		ci_assert(p->offset < sta->acc_size);
		ci_align_check(p->offset, ci_sizeof(u64));
		ci_max_set(acc->dump_data.dpt_name_max, ci_strlen(p->name));
	}

#if 0
	tm = &acc->timer;
	tm->flag = CI_TIMER_PERIODIC;
	tm->msec = cfg_hist->interval;	
#endif
	
	return 0;
}

static void ci_sta_single_dump(ci_sta_t *sta, void *buf, ci_sta_dump_dpt_t *dpt, ci_sta_dump_data_t *data)
{
	const char *fmt = ci_ssf("%%%ds : %%lli\n", data->dpt_name_max + 4);
	const char *fmt_init = ci_ssf("%%%ds : %%s\n", data->dpt_name_max + 4);

	ci_printfln("[ \"%s\", " CI_PRN_FMT_NODE_WORKER " ]", sta->name, CI_PRN_VAL_NODE_WORKER);
	
	for (ci_sta_dump_dpt_t *p = dpt; p->name; p++) {
		u64 val = *(u64 *)((u8 *)buf + p->offset);
		if (val != STA_INIT_PTN_U64) 
			ci_printf(fmt, p->name, *(u64 *)((u8 *)buf + p->offset));
		else
			ci_printf(fmt_init, p->name, CI_STR_UNDEFINED_VALUE);
	}
	ci_printfln();
}



