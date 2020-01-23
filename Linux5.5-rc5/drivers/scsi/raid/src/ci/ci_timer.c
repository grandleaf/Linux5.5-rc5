/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_timer.c								Timer Service
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#define CI_SID_TIMER				"$tmr.sid"			/* scheduler identifier */
#define CI_SWDID_TIMER				"$tmr"				/* string of worker data identifier */

#define ci_timer_worker_data(mod, node_id, worker_id)		\
	((ci_timer_worker_data_t *)((mod)->mem.range_worker[node_id][worker_id].start))

static int CI_WDID_TIMER;	/* timer's work data id */


static void ci_timer_init_wheel(ci_timer_wheel_t *wheel, int id)
{
	ci_obj_zero(wheel);
	wheel->id = id;
	ci_list_ary_init(wheel->head);
}

static void ci_timer_internal_add(ci_timer_worker_data_t *data, ci_timer_t *timer)
{	
	u64 jiffie_adjusted;
	int wheel_id, head_id;
	ci_timer_wheel_t *wheel;

	ci_dbg_exec(timer->ctx_add = data->sched_ctx);

	timer->jiffie = timer->msec / PAL_MSEC_PER_JIFFIE;
	timer->jiffie_expire = pal_jiffie + timer->jiffie;
	if (ci_unlikely(!timer->jiffie))
		timer->jiffie = 1;

	wheel_id = u64_last_set(timer->jiffie) >> 3;
	ci_assert(wheel_id < CI_TIMER_WHEEL_NR, "timer's jiffie is too big: %#llX\n", timer->jiffie);

	wheel = &data->wheel[wheel_id];
	jiffie_adjusted = timer->jiffie + wheel->jiffie_adj;

	if (ci_unlikely((u64_last_set(jiffie_adjusted) >> 3) > wheel_id)) {
		wheel_id++;
		ci_assert(wheel_id < CI_TIMER_WHEEL_NR, "timer's jiffie is too big: %#llX\n", timer->jiffie);
		wheel = &data->wheel[wheel_id];
		jiffie_adjusted = timer->jiffie + wheel->jiffie_adj;
	}
	
	timer->jiffie = jiffie_adjusted;
	head_id = jiffie_adjusted >> (wheel_id << 3);
	ci_range_check_i(head_id, 0, 0xFF);
	head_id = (head_id + wheel->curr) & 0xFF;
	ci_list_add_tail(&wheel->head[head_id], &timer->link);
}

static void ci_timer_wheel_dispatch(ci_timer_worker_data_t *data, ci_timer_wheel_t *wheel)
{
	int head_id;
	ci_timer_t *timer;
	ci_list_t *head = &wheel->head[wheel->curr];
	ci_assert(!ci_list_empty(head));

	/* timer expire callback */
	if (wheel->id == 0) {
		ci_list_each_safe(head, timer, link) {
			ci_list_dbg_poison_set(&timer->link);
			ci_assert(timer->callback);
			ci_assert(data->sched_ctx);

			if (ci_unlikely(timer->flag & CI_TIMER_PERIODIC))
				ci_timer_internal_add(data, timer);
			
			timer->callback(data->sched_ctx, timer->data);
		}

		ci_list_init(head);
		return;
	}

	/* dispatch to lower wheel */
	wheel = &data->wheel[wheel->id - 1];
	ci_list_each_safe(head, timer, link) {
		head_id = ((timer->jiffie >> (wheel->id << 3)) + wheel->curr) & 0xFF;
		ci_list_dbg_poison_set(&timer->link);
		ci_list_add_tail(&wheel->head[head_id], &timer->link);
	}
	ci_list_init(head);
}

static void ci_timer_wheel_spin(ci_timer_worker_data_t *data, ci_timer_wheel_t *wheel)
{
	u64 jiffie_adj;
	ci_range_check(wheel->curr, 0, 0x100);

	/* always do inc first */
	wheel->curr = (wheel->curr + 1) & 0xFF;

	/* update the jiffie adj for curr != 0 */
	if (wheel->curr) {	
		jiffie_adj = 1ull << (wheel->id << 3);
		ci_loop(id, wheel->id + 1, CI_TIMER_WHEEL_NR)
			data->wheel[id].jiffie_adj += jiffie_adj;
	} else {
		/* update the jiffie adj for curr == 0 */
		jiffie_adj = 0xFFull << (wheel->id << 3);
		ci_loop(id, wheel->id + 1, CI_TIMER_WHEEL_NR)
			data->wheel[id].jiffie_adj -= jiffie_adj;
		
		/* spin the upper wheel */
		if (wheel->id != CI_TIMER_WHEEL_NR - 1)		
			ci_timer_wheel_spin(data, &data->wheel[wheel->id + 1]);
	}

	/* dispatch curr if not empty */
	if (ci_unlikely(!ci_list_empty(&wheel->head[wheel->curr])))
		ci_timer_wheel_dispatch(data, wheel);
}

static void __ci_timer_wheel_dump(ci_timer_worker_data_t *data)
{
	ci_loop(i, CI_TIMER_WHEEL_NR) {
		ci_timer_wheel_t *wheel = &data->wheel[i];
		ci_printf("    %d:%d:%llu    ", wheel->id, wheel->curr, wheel->jiffie_adj);
	}
	ci_printfln();
}

static void ci_timer_intr(int jiffie_inc, void *__data)
{
	ci_timer_worker_data_t *data = (ci_timer_worker_data_t *)__data;

	data->jiffie += jiffie_inc;
	if (!ci_atomic_fetch_add(&data->jiffie_pending, jiffie_inc))
		ci_sched_ext_add(data->sched_grp, &data->sched_ent);
}

static void ci_timer_sched(ci_sched_ctx_t *ctx)
{
	ci_list_t ext_head;
	ci_timer_t *timer;
	ci_timer_worker_data_t *data;

	data = (ci_timer_worker_data_t *)ctx->sched_ent->data;
	data->sched_ctx = ctx;

	if (ci_unlikely(!ci_list_empty(&data->ext_head))) { 	/* don't acquire spinlock */
		ci_slk_protected(&data->lock, ci_list_move(&ext_head, &data->ext_head));

		ci_list_each_safe(&ext_head, timer, link) {
			ci_list_dbg_poison_set(&timer->link);
			ci_timer_internal_add(data, timer);
		}
	}
	
	ci_timer_wheel_spin(data, &data->wheel[0]);

//data->jiffie_pending = 2;	pal_jiffie++;
	if (ci_atomic_dec_fetch(&data->jiffie_pending)) {
		ci_sched_run_add(ctx->sched_grp, ctx->sched_ent);
		

#ifndef WIN_SIM		/* Window's print is so so slow */
		ci_dbg_exec(
			if (data->jiffie_pending >= 100)
				ci_exec_upto(3, ci_warn_printfln("Warning: jiffie_pending=%d, timer interrupt handler overloaded!", 
												  data->jiffie_pending));
		);
#endif
	}
}

void __ci_timer_ext_add(ci_sched_ctx_t *ctx, ci_timer_t *timer)
{
	ci_timer_worker_data_t *data = (ci_timer_worker_data_t *)ci_worker_data_by_ctx(ctx, CI_WDID_TIMER);
	ci_slk_protected(&data->lock, ci_list_add_tail(&data->ext_head, &timer->link));		/* will be checked in next jiffie */
}

void __ci_timer_add(ci_sched_ctx_t *ctx, ci_timer_t *timer)
{
	ci_timer_worker_data_t *data = (ci_timer_worker_data_t *)ci_worker_data_by_ctx(ctx, CI_WDID_TIMER);
	ci_timer_internal_add(data, timer);
}



/*
 *	module related 
 */
static void ci_timer_mod_init_each(ci_mod_t *mod, int node_id, int worker_id)
{
	int sd = ci_sched_dpt_by_name(CI_SID_TIMER, NULL, NULL);
	ci_timer_worker_data_t *data = ci_timer_worker_data(mod, node_id, worker_id);

	/* initialize data */
	ci_obj_zero(data);
	ci_list_init(&data->ext_head);
	ci_slk_init(&data->lock);

	/* initialize each wheel */
	ci_ary_each_with_index(data->wheel, wheel, index, ci_timer_init_wheel(wheel, index));

	/* save sched_grp for fast access */
	data->sched_grp = ci_sched_grp_by_ctx(ci_sched_ctx_by_id(node_id, worker_id), sd, CI_SCHED_PRIO_TIMER);
	ci_assert(data->sched_grp);

	/* setup the sched_ent */
	data->sched_ent.exec = ci_timer_sched;
	data->sched_ent.data = data;

	/* register to pal */
	data->intr_handler.callback = ci_timer_intr;
	data->intr_handler.data = data;
	data->jiffie = pal_jiffie;
	pal_timer_register(&data->intr_handler);

	/* set per worker data */
	ci_worker_data_set(ci_sched_ctx_by_id(node_id, worker_id), CI_WDID_TIMER, data);
}

void ci_timer_mod_init(ci_mod_t *mod)
{
	CI_WDID_TIMER = ci_worker_data_register(CI_SWDID_TIMER);		/* register per worker data */
	ci_mod_node_worker_iterator(mod, ci_timer_mod_init_each);
	pal_timer_init();
//	ci_mod_init_done(mod, json);
}

static void ci_timer_mod_shutdown(ci_mod_t *mod, ci_json_t *json)
{
	pal_timer_finz();
	ci_mod_shutdown_done(mod, json);
}

ci_sched_id_def(ci_timer_sid, 
	{	.name 		= CI_SID_TIMER,
		.desc		= "ci timer sched",
		.prio_map 	= CI_SCHED_PRIO_BIT(CI_SCHED_PRIO_TIMER)
	},
	
	CI_EOT
);

ci_mod_def(mod_timer, {
	.name = "$tmr",
	.desc = "ci built-in timer module",

	.node_map 	= CI_NODE_ALL_MAP,			// {{1}}, CI_NODE_ALL_MAP
	.worker_map = CI_NODE_WORKER_ALL_MAP,	// {{{1}}}, CI_NODE_WORKER_ALL_MAP
	.sched_id 	= ci_timer_sid,

	.mem = {
		.size_worker		= ci_sizeof(ci_timer_worker_data_t)
	},	

	.vect = {
//		[CI_MODV_INIT] 		= ci_timer_mod_init,
		[CI_MODV_SHUTDOWN]	= ci_timer_mod_shutdown
	},		
});





 
