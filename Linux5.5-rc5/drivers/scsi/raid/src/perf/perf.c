/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * perf.c			Performance Evaluation Module
 *                                                          hua.ye@Hua Ye.com
 */
#include "perf.h"

//#define PERF_ALL_WORKER			/* stress all workers */
#define PERF_FAST_SCHED


static perf_cfg_t perf_cfg = {
	.test_time  		= 10 * 1000,
	.queue_depth 		= 2,
	.node_map 			= {{ 1 }},
	.worker_map 		= {{{ 1 }}}								// worker 0
//	.worker_map 		= {{{ 3 }}}								// worker 0, 1

//	.worker_map 		= {{{ 1 + (1 << 10) }}}					// WIN_SIM smt
//	.worker_map 		= {{{ 3 + (1 << 10) + (1 << 11) }}}		// WIN_SIM 2x smt

//	.worker_map 		= {{{ 1 + (1 << 18) }}}					// simulator smt
//	.worker_map 		= {{{ 3 + (1 << 18) + (1 << 19) }}}		// simulator 2x smt
};

static perf_data_t perf_data;


static void perf_task_callback(ci_sched_ctx_t *ctx)
{
	int pending;
	perf_task_t *task = perf_task_by_ctx(ctx);
	perf_data_t *pd = &perf_data;

	if (!pd->stop) {
		task->cnt++;

#ifdef PERF_FAST_SCHED		
		ci_sched_run_add(task->sched_grp, &task->sched_task.ent);
#else
		ci_sched_task(&task->sched_task);
#endif

		return;
	}

	pending = ci_atomic_fetch_dec(&pd->pending);
	if (pending == pd->total) {		/* first one */
		ci_list_each(&pd->head, task, link)
			pd->data.nr_io += task->cnt;
		
		ci_perf_eval_end(&perf_data.data, 1);
	}

	if (pending != 1) 	/* not the last one */
		return;

	/* last one, cleanup */
	ci_list_each_safe(&pd->head, task, link) {
		ci_list_dbg_poison_set(&task->link);
		ci_node_bfree(ci_node_id_by_sched_task(&task->sched_task), task);
	}
	ci_list_init(&pd->head);

	ci_printfln("ALL DONE!");
}

static void perf_timer_stop(ci_sched_ctx_t *ctx, void *data)
{
	perf_data.stop = 1;
}

static void perf_cfg_all_worker()
{
	ci_node_map_mask(&perf_cfg.node_map, 0, ci_node_info->nr_node);
	ci_node_map_each_set(&perf_cfg.node_map, node_id)
		ci_worker_map_mask(&perf_cfg.worker_map[node_id], 0, ci_node_by_id(node_id)->nr_worker);
}


void perf_test_start()
{
	perf_task_t *task;

#ifdef PERF_ALL_WORKER
	perf_cfg_all_worker();
#endif
	

	/* initialize the stop timer */
	static ci_timer_t tm = {
		.data 		= &tm,
		.callback 	= perf_timer_stop,
	};
	tm.msec 		= perf_cfg.test_time;

	/* initialize the perf_data */
	perf_data_t *pd = &perf_data;
	ci_obj_zero(pd);

	/* common init */
	ci_list_init(&pd->head);
	ci_node_map_each_set(&perf_cfg.node_map, node_id)
		ci_worker_map_each_set(&perf_cfg.worker_map[node_id], worker_id) 
			ci_loop(perf_cfg.queue_depth) {
				task = ci_node_balloc(node_id, ci_sizeof(perf_task_t));
				ci_sched_task_t *st = &task->sched_task;
		
				ci_obj_zero(task);
				st->dpt = ci_sched_dpt_by_name("perf.io", NULL, NULL);
				st->prio = CI_SCHED_PRIO_NORMAL;
				st->ctx = ci_sched_ctx_by_id(node_id, worker_id);
				st->ent.data = &st;
				st->ent.exec = perf_task_callback;

				task->sched_grp = ci_sched_grp_by_ctx(st->ctx, st->dpt, st->prio);

				pd->pending++, pd->total++;
				ci_list_add_tail(&pd->head, &task->link);
			}

	ci_printfln("+++ Starting Performance Test, duration: %d seconds ...", perf_cfg.test_time / 1000);
	ci_dump_preln("node_map: ", ci_node_map_dump(&perf_cfg.node_map));
	ci_node_map_each_set(&perf_cfg.node_map, node_id) {
		ci_printf("worker_map[%d]: ", node_id);
		ci_worker_map_dumpln(&perf_cfg.worker_map[node_id]);
	}
	ci_printf("queue_depth: %d\n", perf_cfg.queue_depth);

	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);
	ci_perf_eval_start(&pd->data, 0);
	
	ci_list_each(&pd->head, task, link)
#ifdef PERF_FAST_SCHED		
		ci_sched_ext_add(task->sched_grp, &task->sched_task.ent);
#else
		ci_sched_task_ext(&task->sched_task);
#endif
}

#if 0
static void perf_mod_start(ci_mod_t *mod, ci_json_t *json)
{
	static ci_timer_t tm = {
		.msec 		= 3000,
		.data 		= &tm,
		.callback 	= perf_timer_callback,
	};

	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);
	ci_mod_start_done(mod, json);
}
#endif

