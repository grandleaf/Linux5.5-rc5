/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_worker.c				CI Workers
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

#ifdef CI_WORKER_STA
//#define CI_WORKER_HIST_DUMP_NAME				"1_SEC"

static ci_sta_cfg_hist_t worker_sta_cfg_hist[] = {
	{ "1_SEC",			1000,				60 	},		/* 1000 ms, 60 samples */
	{ "5_SEC",			5000,				60 	},		/* 5000 ms, 60 samples */
	{ "1_MIN",			60000,				60 	},		/* 1 minute, 60 samples */
	CI_EOT
};

#ifdef CI_WORKER_HIST_DUMP_NAME
static ci_sta_dump_dpt_t worker_dump_dpt[] = ci_sta_dump_dpt_maker(ci_worker_sta_acc_t, 
	busy_cycle,
);
#endif /* !CI_WORKER_HIST_DUMP_NAME */
#endif /* !CI_WORKER_STA */

#ifdef CI_SCHED_DEBUG
static void ci_worker_check(ci_sched_ctx_t *ctx, ci_sched_ent_t *ent)
{
	if (!ctx->sched_grp->name || !ci_strlen(ctx->sched_grp->name))
		ci_panic("sched_grp without name, grp=%p, mod=%s", ctx->sched_grp, ctx->sched_grp->mod->name);

	if (!ctx->sched_grp->exec_check) 
		ctx->sched_grp->exec_check = ent->exec;
	else
		ci_assert(ctx->sched_grp->exec_check == ent->exec, 
				  "different exec() detected, %p, %p", 
				  ctx->sched_grp->exec_check, ent->exec);
}
#endif	

static int ci_worker_do_work(ci_worker_t *worker)
{
#ifdef CI_WORKER_STA
	u64 cycle_start, cycle_end;
	cycle_start = pal_perf_counter();		/* pal_perf_counter_relax() goes faster, but not precise */
#endif

	int se_flag, rv = 0;
	ci_sched_tab_t *tab = &worker->sched_tab;
	ci_sched_ctx_t *ctx = &tab->ctx;
	ci_sched_ent_t *ent = ci_sched_get(tab);
	ci_paver_map_t *map = &tab->pn_map;

	if (ci_unlikely(!ci_paver_map_is_all_clear(map)))
		ci_paver_notify_map_check(ctx);

	if (ci_unlikely(!ent)) 
		goto __exit;

	/* check */
	rv = 1;
	ci_sched_dbg_exec(ci_worker_check(ctx, ent));
	ci_paver_check_reset(ctx);
	ci_assert(ent->exec);

#if 0
	/* inc table's busy? */
	se_flag = ent->flag;	
	ci_assert(!ci_flag_all_set(se_flag, CI_SCHED_ENTF_BUSY_INC | CI_SCHED_ENTF_BUSY_DEC));

	if (ci_unlikely(se_flag & CI_SCHED_ENTF_BUSY_INC)) {
		int new_lvl_busy;
		
		tab->nr_busy++;
		ci_assert(tab->nr_busy >= 1);

		if (ci_unlikely((new_lvl_busy = ci_sched_busy_inc_get_lvl(tab->lvl_busy, tab->nr_busy)) != tab->lvl_busy)) {
			tab->lvl_busy = new_lvl_busy;
			ci_printf("+ nr_busy=%d, new_lvl_busy=%d\n", tab->nr_busy, new_lvl_busy);
		}
	}
#endif	
	
	/*
	 *	do the work 
	 */
	ent->exec(ctx);	

	/* dec table's busy? */
	se_flag = ent->flag;	
	ci_assert(!ci_flag_all_set(se_flag, CI_SCHED_ENTF_BUSY_INC | CI_SCHED_ENTF_BUSY_DEC));
	if (ci_unlikely(se_flag & CI_SCHED_ENTF_BUSY_DEC)) {
		int new_lvl_busy, new_nr_busy;
	
		new_nr_busy = ci_atomic_dec_fetch(&tab->nr_busy);
		if (ci_unlikely((new_lvl_busy = ci_sched_busy_dec_get_lvl(tab->lvl_busy, new_nr_busy)) != tab->lvl_busy)) {
			tab->lvl_busy = new_lvl_busy;
			ci_printf("- nr_busy=%d, new_lvl_busy=%d\n", tab->nr_busy, new_lvl_busy);
		}
	}

	/* unblock printf */
	if (ci_unlikely(ctx->flag & CI_SCHED_CTXF_PRINTF)) {
		ctx->flag &= ~CI_SCHED_CTXF_PRINTF;
		__ci_printf_block_unlock();
	}

	/* clear context */
	ctx->sched_grp = NULL, ctx->sched_ent = NULL;

__exit:
	
#ifdef CI_WORKER_STA
	cycle_end = pal_perf_counter();		/* pal_perf_counter_relax() goes faster, but not precise */
	ci_worker_sta_acc_add(worker, busy_cycle, cycle_end - cycle_start + PAL_CYCLE_OVERHEAD);
#endif

	return rv;
}

int ci_worker_init(ci_worker_t *worker)
{
	worker->do_work = ci_worker_do_work;

	ci_sched_tab_init(&worker->sched_tab);
	worker->sched_tab.ctx.worker = worker;

	return 0;
}

int ci_worker_post_init()
{
#ifdef CI_WORKER_STA
	/* init statistics */
	ci_sta_cfg_t cfg = {
		.name				= "ci_worker",
		.hist				= worker_sta_cfg_hist,
		.acc_size			= ci_sizeof(ci_worker_sta_acc_t)
	};	

	ci_node_worker_each(node, worker, {
		cfg.node_id 	= node->node_id;
		cfg.worker_id 	= worker->worker_id;
		
		worker->sta = ci_sta_create(&cfg);
		
#ifdef CI_WORKER_HIST_DUMP_NAME	
		ci_sta_set_hist_dump(worker->sta, CI_WORKER_HIST_DUMP_NAME, worker_dump_dpt, 1, 0);				
#endif
	});
#endif	

	return 0;
}

int ci_worker_data_register(const char *name)
{
	const char **data_name;
	
	ci_assert(name);

	ci_slk_lock(&ci_node_info->lock);
	ci_loop(i, CI_WORKER_DATA_NR) {
		data_name = &ci_node_info->worker_data_name[i];
		if (!*data_name) {
			*data_name = name;
			ci_slk_unlock(&ci_node_info->lock);
			return i;
		}

		ci_assert(!ci_strequal(name, *data_name), "duplicate worker data name: \"%s\"", name);
	}

	ci_bug("CI_WORKER_DATA_NR too small!");
	ci_slk_unlock(&ci_node_info->lock);

	return -1;
}

#if 0
int ci_worker_data_id(const char *name)
{
	const char *data_name;
	
	ci_loop(i, CI_WORKER_DATA_NR) {
		data_name = ci_node_info->worker_data_name[i];
		if (ci_strequal(name, data_name))
			return i;
	}	

	ci_bug("Cannot find worker data name: \"%s\", do ci_worker_data_register() first.", name);
	return -1;
}
#endif

void ci_worker_data_set(ci_sched_ctx_t *ctx, int wdid, void *data)
{
	ci_assert(ctx && ctx->worker);
	ci_range_check(wdid, 1, CI_WORKER_DATA_NR);
	ci_assert(!ctx->worker->data[wdid], "wdid already set");
	ctx->worker->data[wdid] = data;
}




