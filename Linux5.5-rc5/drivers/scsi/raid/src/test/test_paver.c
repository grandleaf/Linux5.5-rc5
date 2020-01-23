#include "ci.h"
#include "hwsim/hwsim.h"

#define PVT_TEST

//#define PVT_ALL_NODE_WORKER			/* ignore the config's node/worker setting, stress all nodes/workers */

#ifndef PVT_ALL_NODE_WORKER
#define PVT_NODE_MAP					{{ 1 }}
#define PVT_WORKER_MAP					{{{ 1 }}}	
//	{{{ 3 }}, {{3}} }								// worker 0, 1
//	{{{ 1 + (1 << 10) }}}						// WIN_SIM smt
//	{{{ 3 + (1 << 10) + (1 << 11) }}}			// WIN_SIM 2x smt
//	{{{ 1 + (1 << 18) }}}						// simulator smt
//	{{{ 3 + (1 << 18) + (1 << 19) }}}			// simulator 2x smt
#else
#define PVT_NODE_MAP 					CI_NODE_ALL_MAP
#define PVT_WORKER_MAP					CI_NODE_WORKER_ALL_MAP
#endif


//#define PVT_TEST_DURATION				(60 * 60 * 60 * 1000)		/* 60 hours */
#define PVT_TEST_DURATION				(10 * 1000)		/* 10 seconds */
#define PVT_QUEUE_DEPTH					128
#define PVT_DATA_CHECK					/* check the if paver does wrong thing */
#define PVT_TIMER_DUMP
#define PVT_LOOPBACK					/* direct callback without use hwsim threads */
#define PVT_FAST_SCHED
#define PVT_NO_ALLOC_FREE_RES			/* just test submit/return path, no resource alloc/free */

#ifdef WIN_SIM
#define PVT_TASK_PRE_DELAY_NS			500000
#else
#define PVT_TASK_PRE_DELAY_NS			1000		
#endif

/* wave of contiguous resource allocation */
#define PVT_WAVE_MIN					5000	
#define PVT_WAVE_MAX					10000


/* max alloc size */
#if 1
#define PVT_RES_A_ALLOC_MAX				54
#define PVT_RES_B_ALLOC_MAX				54
#define PVT_RES_C_ALLOC_MAX				128
#else
#define PVT_RES_A_ALLOC_MAX				3
#define PVT_RES_B_ALLOC_MAX				2
#define PVT_RES_C_ALLOC_MAX				2
#endif


/* resource size in bytes, + size of link */
#define PVT_RES_A_SIZE					23
#define PVT_RES_B_SIZE					64
#define PVT_RES_C_SIZE					88


typedef struct {
	int						 queue_depth;	/* for each worker */
	u64						 pre_delay_ns;
	u64						 post_delay_ns;
//	ci_node_map_t			 node_map;
//	ci_worker_map_t			 worker_map[CI_NODE_NR];
} pvt_cfg_t;

static pvt_cfg_t pvt_cfg = {
	.queue_depth 		= PVT_QUEUE_DEPTH,			/* per worker */
	.pre_delay_ns		= 500000,

//	.node_map 			= PVT_NODE_MAP,
//	.worker_map 		= PVT_WORKER_MAP
};


#define pvt_info(mod)									((pvt_info_t *)ci_mod_mem_shr(mod))
#define pvt_node_info(mod, node_id)						((pvt_node_info_t *)ci_mod_mem_node(mod, node_id))
#define pvt_worker_info(mod, node_id, worker_id)		((pvt_worker_info_t *)ci_mod_mem_worker(mod, node_id, worker_id))
#define pvt_worker_info_by_ctx(ctx)						pvt_worker_info(ci_mod_by_ctx(ctx), (ctx)->worker->node_id, (ctx)->worker->worker_id)

#ifdef PVT_FAST_SCHED
#define pvt_ctx_by_task(task)							(&(task)->sf_dpt.sg_sbm->tab->ctx)		/* submit & return has the same context */
#define pvt_task_by_submit_ctx(ctx)						ci_container_of((ctx)->sched_ent, pvt_task_t, sf_dpt.se_sbm)	
#define pvt_task_by_return_ctx(ctx)						ci_container_of((ctx)->sched_ent, pvt_task_t, sf_dpt.se_ret)	
#else
#define pvt_ctx_by_task(task)							((task)->st_dpt.st_sbm.ctx)		/* submit & return has the same context */
#define pvt_task_by_submit_ctx(ctx)						ci_container_of((ctx)->sched_ent, pvt_task_t, st_dpt.st_sbm.ent)	
#define pvt_task_by_return_ctx(ctx)						ci_container_of((ctx)->sched_ent, pvt_task_t, st_dpt.st_ret.ent)	
#endif



typedef struct {
	u8						 random;
	u8					 	 buf[PVT_RES_A_SIZE];
	u8						 random_check;
	ci_list_t				 link;
} pvt_res_a_t;

typedef struct {
	u8						 random_check;
	u8					 	 buf[PVT_RES_B_SIZE];
	ci_list_t				 link;
	u8						 random;
} pvt_res_b_t;

typedef struct {
	u8					 	 buf[PVT_RES_C_SIZE];
	u8						 random;
	ci_list_t				 link;
	u8						 random_check;
} pvt_res_c_t;

typedef struct {
	int 					 flag;
#define PVTF_STOP				0x0001
	
	int						 pid_res_a;
	int						 pid_res_b;
	int						 pid_res_c;

	ci_perf_data_t 			 perf_data;
} pvt_info_t;

typedef struct {
	u64						 dummy;
} pvt_node_info_t;

typedef struct {
	int						 wave_a;	/* alloc/free wave simulation */
	int						 wave_b;
	int						 wave_c;
	int						 dice_a;
	int						 dice_b;
	int						 dice_c;

	u64				 		 total;
	ci_list_t				 head;
} pvt_worker_info_t;

typedef struct {
	int						 task_id;		

	int						 node_id;
	int						 worker_id;

	u64						 cnt;

#ifdef PVT_FAST_SCHED
	ci_sched_fast_dpt_t		sf_dpt;
#else
	ci_sched_task_dpt_t		st_dpt;
#endif

	pvt_info_t				*pvt_info;
	pvt_node_info_t			*pvt_node_info;
	pvt_worker_info_t		*pvt_worker_info;

	ci_list_t				 head_res_a;
	ci_list_t				 head_res_b;
	ci_list_t				 head_res_c;

	hwsim_task_t			 hwsim_task;
	ci_list_t				 link;
} pvt_task_t;

ci_paver_cfg_t pool_cfg_res_a = {
	.name				= "pvt_res_a_t",
	.alloc_max			= PVT_RES_A_ALLOC_MAX,
	.size				= ci_sizeof(pvt_res_a_t),
};

ci_paver_cfg_t pool_cfg_res_b = {
	.name				= "pvt_res_b_t",
	.alloc_max			= PVT_RES_B_ALLOC_MAX,
	.size				= ci_sizeof(pvt_res_b_t),
};

ci_paver_cfg_t pool_cfg_res_c = {
	.name				= "pvt_res_c_t",
	.alloc_max			= PVT_RES_C_ALLOC_MAX,
	.size				= ci_sizeof(pvt_res_c_t),
};

static void pvt_info_init(pvt_info_t *info)
{
	ci_obj_zero(info);
}

static void pvt_node_info_init(pvt_node_info_t *info)
{
	ci_obj_zero(info);
}

static void pvt_worker_info_init(pvt_worker_info_t *info)
{
	ci_obj_zero(info);
	ci_list_init(&info->head);
}

/* incase in the future we implement the pvt_task_t as a paver, then we need reassign node_id/workeer_id */
static void pvt_task_set_node_worker(pvt_task_t *task, int node_id, int worker_id)
{
#ifdef PVT_FAST_SCHED	
	ci_sched_ctx_t *ctx;

	ctx = ci_sched_ctx_by_id(node_id, worker_id);
	task->sf_dpt.sg_sbm = ci_sched_grp_by_ctx(ctx, ci_sched_dpt_by_name("pvt.io.smb", NULL, NULL), CI_SCHED_PRIO_NORMAL);
	task->sf_dpt.sg_ret = ci_sched_grp_by_ctx(ctx, ci_sched_dpt_by_name("pvt.io.ret", NULL, NULL), CI_SCHED_PRIO_NORMAL);
#else
	ci_sched_task_t *st;

	st = &task->st_dpt.st_sbm;
	st->ctx = ci_sched_ctx_by_id(node_id, worker_id);

	st = &task->st_dpt.st_ret;
	st->ctx = ci_sched_ctx_by_id(node_id, worker_id);
#endif
}

static void pvt_task_alloc_res(ci_sched_ctx_t *ctx, pvt_task_t *pvt_task)
{
#define pvt_alloc_nr_by_dice(ctx, dice, buckete_size)	\
	({	\
		int __alloc_nr__;		\
		switch (dice) {	\
			case 0:	\
				__alloc_nr__ = ci_rand_i(ctx, ci_min(3, buckete_size));	\
				break;	\
			case 1:	\
				__alloc_nr__ = ci_rand_i(ctx, ci_max(0, buckete_size - 3), buckete_size);	\
				break;	\
			case 2:	\
				__alloc_nr__ = ci_rand_i(ctx, buckete_size);	\
				break;	\
			default:	\
				ci_bug();	\
				__alloc_nr__ = 0;	\
				break;	\
		}	\
		\
		__alloc_nr__;	\
	})
		

	int alloc_nr;
	pvt_worker_info_t *w_info = pvt_task->pvt_worker_info;
	
	ci_assert(pvt_task->pvt_worker_info == pvt_worker_info_by_ctx(ctx));
	if (w_info->wave_a <= 0) {
		w_info->wave_a = ci_rand_i(ctx, PVT_WAVE_MIN, PVT_WAVE_MAX);
		w_info->dice_a = ci_rand(ctx, 2);
	}
	
	if (w_info->wave_b <= 0) {
		w_info->wave_b = ci_rand_i(ctx, PVT_WAVE_MIN, PVT_WAVE_MAX);
		w_info->dice_b = ci_rand(ctx, 3);
	}
	
	if (w_info->wave_c <= 0) {
		w_info->wave_c = ci_rand_i(ctx, PVT_WAVE_MIN, PVT_WAVE_MAX);
		w_info->dice_c = ci_rand(ctx, 3);
	}
	

	alloc_nr = pvt_alloc_nr_by_dice(ctx, w_info->dice_a, PVT_RES_A_ALLOC_MAX);
	ci_loop(alloc_nr) {
		pvt_res_a_t *a = ci_palloc(ctx, pvt_task->pvt_info->pid_res_a);
#ifdef PVT_DATA_CHECK		
		a->random = ci_rand_i(ctx, 0xFF);
		a->random_check = (u8)(~a->random + pvt_task->task_id);
		ci_memset(a->buf, a->random, ci_sizeof(a->buf));
#endif		
		ci_list_add_tail(&pvt_task->head_res_a, &a->link);
	}

	alloc_nr = pvt_alloc_nr_by_dice(ctx, w_info->dice_b, PVT_RES_B_ALLOC_MAX);
	ci_loop(alloc_nr) {
		pvt_res_b_t *b = ci_palloc(ctx, pvt_task->pvt_info->pid_res_b);
#ifdef PVT_DATA_CHECK		
		b->random = ci_rand_i(ctx, 0xFF);
		b->random_check = (u8)(~b->random + pvt_task->task_id);
		ci_memset(b->buf, b->random, ci_sizeof(b->buf));
#endif		
		ci_list_add_tail(&pvt_task->head_res_b, &b->link);
	}

	alloc_nr = pvt_alloc_nr_by_dice(ctx, w_info->dice_c, PVT_RES_C_ALLOC_MAX);
	ci_loop(alloc_nr) {
		pvt_res_c_t *c = ci_palloc(ctx, pvt_task->pvt_info->pid_res_c);
#ifdef PVT_DATA_CHECK		
		c->random = ci_rand_i(ctx, 0xFF);
		c->random_check = (u8)(~c->random + pvt_task->task_id);
		ci_memset(c->buf, c->random, ci_sizeof(c->buf));
#endif		
		ci_list_add_tail(&pvt_task->head_res_c, &c->link);
	}
}

static void pvt_task_free_res(pvt_task_t *pvt_task)
{
	pvt_res_a_t *a;
	pvt_res_b_t *b;
	pvt_res_c_t *c;
	pvt_worker_info_t *w_info = pvt_task->pvt_worker_info;
	ci_sched_ctx_t *ctx = pvt_ctx_by_task(pvt_task);

	w_info->wave_a--;
	w_info->wave_b--;
	w_info->wave_c--;
	
	ci_list_each_safe(&pvt_task->head_res_a, a, link) {
#ifdef PVT_DATA_CHECK
		if (a->random_check != (u8)(~a->random + pvt_task->task_id))
			ci_panic("random_check error");
		for (int i = 0; i < ci_sizeof(a->buf); i++)
			if (a->random != a->buf[i])
				ci_panic("buffer error");
#endif
		ci_list_dbg_poison_set(&a->link);	/* free without del, so we set this in debug mode to avoid panics */
		ci_pfree(ctx, pvt_task->pvt_info->pid_res_a, a);
	}

	ci_list_each_safe(&pvt_task->head_res_b, b, link) {
#ifdef PVT_DATA_CHECK
		if (b->random_check != (u8)(~b->random + pvt_task->task_id))
			ci_panic("random_check error");
		for (int i = 0; i < ci_sizeof(b->buf); i++)
			if (b->random != b->buf[i])
				ci_panic("buffer error");
#endif
		ci_list_dbg_poison_set(&b->link);	/* free without del, so we set this in debug mode to avoid panics */
		ci_pfree(ctx, pvt_task->pvt_info->pid_res_b, b);
	}

	ci_list_each_safe(&pvt_task->head_res_c, c, link) {
#ifdef PVT_DATA_CHECK
		if (c->random_check != (u8)(~c->random + pvt_task->task_id))
			ci_panic("random_check error");
		for (int i = 0; i < ci_sizeof(c->buf); i++)
			if (c->random != c->buf[i])
				ci_panic("buffer error");
#endif
		ci_list_dbg_poison_set(&c->link);	/* free without del, so we set this in debug mode to avoid panics */
		ci_pfree(ctx, pvt_task->pvt_info->pid_res_c, c);
	}

	ci_list_init(&pvt_task->head_res_a);
	ci_list_init(&pvt_task->head_res_b);
	ci_list_init(&pvt_task->head_res_c);
}

static void pvt_task_done(ci_sched_ctx_t *ctx)
{
	pvt_task_t *pvt_task = pvt_task_by_return_ctx(ctx);

	pvt_task->pvt_worker_info->total++;

#ifndef PVT_NO_ALLOC_FREE_RES	
	pvt_task_free_res(pvt_task);
#endif

	if (pvt_task->pvt_info->flag & PVTF_STOP)
		return;

#ifdef PVT_FAST_SCHED		
	ci_sched_ext_add(pvt_task->sf_dpt.sg_sbm, &pvt_task->sf_dpt.se_sbm);
#else
	ci_sched_task_ext(&pvt_task->st_dpt.st_sbm);
#endif		
}

static void pvt_hw_task_callback(hwsim_task_t *hw_task, void *data)
{
	pvt_task_t *pvt_task = ci_container_of(hw_task, pvt_task_t, hwsim_task);

#ifdef PVT_FAST_SCHED		
	ci_sched_ext_add(pvt_task->sf_dpt.sg_ret, &pvt_task->sf_dpt.se_ret);
#else
	ci_sched_task_ext(&pvt_task->st_dpt.st_ret);
#endif		
}

static void pvt_task_exec(ci_sched_ctx_t *ctx)
{
	pvt_task_t *pvt_task = pvt_task_by_submit_ctx(ctx);
	hwsim_task_t *hwsim_task = &pvt_task->hwsim_task;

#ifndef PVT_NO_ALLOC_FREE_RES
	pvt_task_alloc_res(ctx, pvt_task);
#endif

#ifdef PVT_LOOPBACK
	pvt_hw_task_callback(hwsim_task, hwsim_task->data);
#else
	if (pvt_cfg.pre_delay_ns)
		hwsim_task->pre_delay_ns = ci_rand(ctx, 0, pvt_cfg.pre_delay_ns);
	if (pvt_cfg.post_delay_ns)
		hwsim_task->post_delay_ns = ci_rand(ctx, 0, pvt_cfg.post_delay_ns);
	
	hwsim_task_submit(hwsim_task);
#endif	
}

static void pvt_task_init(pvt_task_t *task, ci_mod_t *mod, int node_id, int worker_id)
{
	ci_obj_zero(task);
	task->node_id			= node_id;
	task->worker_id			= worker_id;
	task->pvt_info			= pvt_info(mod);
	task->pvt_node_info		= pvt_node_info(mod, node_id);
	task->pvt_worker_info	= pvt_worker_info(mod, node_id, worker_id);

	ci_list_init(&task->head_res_a);
	ci_list_init(&task->head_res_b);
	ci_list_init(&task->head_res_c);

#ifdef PVT_FAST_SCHED
	ci_sched_fast_dpt_t *sf = &task->sf_dpt;
	sf->se_sbm.exec	= pvt_task_exec;
	sf->se_sbm.flag = CI_SCHED_ENTF_BUSY_INC;
	sf->se_ret.exec	= pvt_task_done;
	sf->se_ret.flag	= CI_SCHED_ENTF_BUSY_DEC;
#else
	ci_sched_task_t *st;

	st = &task->st_dpt.st_sbm;
	st->dpt 		= ci_sched_dpt_by_name("pvt.io.smb", NULL, NULL);
	st->prio 		= CI_SCHED_PRIO_NORMAL;
	st->ent.flag	= CI_SCHED_ENTF_BUSY_INC;
	st->ent.exec 	= pvt_task_exec;

	st = &task->st_dpt.st_ret;
	st->dpt 		= ci_sched_dpt_by_name("pvt.io.ret", NULL, NULL);
	st->prio 		= CI_SCHED_PRIO_NORMAL;
	st->ent.flag	= CI_SCHED_ENTF_BUSY_DEC;
	st->ent.exec 	= pvt_task_done;
#endif	

	task->hwsim_task.callback = pvt_hw_task_callback;
	pvt_task_set_node_worker(task, node_id, worker_id);
}

/*
static void paver_test_cfg_all_node_worker()
{
	ci_node_map_mask(&pvt_cfg.node_map, 0, ci_node_info->nr_node);
	ci_node_map_each_set(&pvt_cfg.node_map, node_id)
		ci_worker_map_mask(&pvt_cfg.worker_map[node_id], 0, ci_node_by_id(node_id)->nr_worker);
}
*/

static void paver_test_init(ci_mod_t *mod, ci_json_t *json)
{
	int task_id = 0;
	pvt_info_t *info = pvt_info(mod);

	pvt_info_init(info);

#ifdef PVT_ALL_NODE_WORKER
//	paver_test_cfg_all_node_worker();
#endif

	/* init worker info and create tasks */
	ci_node_map_each_set(&mod->node_map, node_id) {
		pvt_node_info_init(pvt_node_info(mod, node_id));
	
		ci_worker_map_each_set(&mod->worker_map[node_id], worker_id) {
			ci_printf("paver_test_init node_id=%d, worker_id=%d\n", node_id, worker_id);

			pvt_worker_info_t *w_info = pvt_worker_info(mod, node_id, worker_id);
			pvt_worker_info_init(w_info);

			pvt_task_t *pvt_task;
			int alloc_size = ci_sizeof(pvt_task_t) * pvt_cfg.queue_depth;
			u8 *ptr = ci_node_halloc(node_id, alloc_size, 0, ci_ssf("pvt_task_t.%d.%02d", node_id, worker_id));

			ci_mem_range_each(pvt_task, &((ci_mem_range_t){ ptr, ptr + alloc_size })) {
				pvt_task_init(pvt_task, mod, node_id, worker_id);
				pvt_task->task_id = task_id++;
				ci_list_add_tail(&w_info->head, &pvt_task->link);
			}
		}
	}


	/* create pavers a, b & c */
	ci_node_map_copy(&pool_cfg_res_a.node_map, &mod->node_map);
	ci_memcpy(pool_cfg_res_a.worker_map, mod->worker_map, ci_sizeof(mod->worker_map));
	info->pid_res_a = ci_paver_pool_create(&pool_cfg_res_a);

	ci_node_map_copy(&pool_cfg_res_b.node_map, &mod->node_map);
	ci_memcpy(pool_cfg_res_b.worker_map, mod->worker_map, ci_sizeof(mod->worker_map));
	info->pid_res_b = ci_paver_pool_create(&pool_cfg_res_b);
	
	ci_node_map_copy(&pool_cfg_res_c.node_map, &mod->node_map);
	ci_memcpy(pool_cfg_res_c.worker_map, mod->worker_map, ci_sizeof(mod->worker_map));
	info->pid_res_c = ci_paver_pool_create(&pool_cfg_res_c);

	/* init done */
	ci_mod_init_done(mod, json);
}

static void paver_test_node_worker(ci_mod_t *mod, int node_id, int worker_id)
{
	pvt_task_t *task;
	pvt_worker_info_t *info = pvt_worker_info(mod, node_id, worker_id);
	
	ci_printfln("paver test: node_id=%d, worker_id=%d", node_id, worker_id);

	ci_list_each(&info->head, task, link)
#ifdef PVT_FAST_SCHED		
		ci_sched_ext_add(task->sf_dpt.sg_sbm, &task->sf_dpt.se_sbm);
#else
		ci_sched_task_ext(&task->st_dpt.st_sbm);
#endif	
}

static void paver_test_perf(ci_mod_t *mod)
{
	pvt_info_t *info = pvt_info(mod);
	ci_perf_data_t *perf_data = &info->perf_data;

	ci_node_map_each_set(&mod->node_map, node_id) 
		ci_worker_map_each_set(&mod->worker_map[node_id], worker_id) {
			pvt_worker_info_t *w_info = pvt_worker_info(mod, node_id, worker_id);
			perf_data->nr_io += w_info->total;
		}

	ci_perf_eval_end(perf_data, 1);	
}

static void paver_test_stop_timer_callback(ci_sched_ctx_t *ctx, void *data)
{
	ci_mod_t *mod = (ci_mod_t *)data;
	pvt_info_t *info = pvt_info(mod);
	
	ci_imp_printfln("paver test: STOP timer triggered");
	info->flag |= PVTF_STOP;

	ci_paver_sta_dump_stop();
	paver_test_perf(mod);
}

#ifdef PVT_TIMER_DUMP
static void paver_test_timer_callback(ci_sched_ctx_t *ctx, void *data)
{
#ifdef CI_WORKER_STA
	int cnt = 0;
	u64 busy_cycle;

	ci_mod_t *mod = (ci_mod_t *)data;
	pvt_info_t *info = pvt_info(mod);
	if (info->flag & PVTF_STOP)
		return;

	ci_node_worker_each(node, worker, {
		ci_printf(" %d/%02d", node->node_id, worker->worker_id);
		cnt++;
	});
	ci_printfln();
	ci_print_hlineln(cnt * 5);	

	ci_node_worker_each(node, worker, {
		if (ci_sta_last_hist_val(worker->sta, "1_SEC", 0, &busy_cycle) < 0) {
			ci_printf("  n/a");
			continue;
		}

		ci_printf("  %3lld", (busy_cycle * 1000 / PAL_CYCLE_PER_SEC) / 10);
//		ci_printf(CI_PR_PCT_FMT "  ", ci_pr_pct_val(busy_cycle, PAL_CYCLE_PER_SEC));
	});

	ci_printf("\n\n");
#endif	
	
#if 0
	pvt_info_t *info = pvt_info((ci_mod_t *)data);
	if (info->flag & PVTF_STOP)
		return;

	ci_paver_info_dump_rti(1);

	ci_print_hlineln(120);
	ci_printfln();
#endif	
}
#endif

static void paver_test_start(ci_mod_t *mod, ci_json_t *json)
{
//	pvt_cfg_t *cfg = &pvt_cfg;
	pvt_info_t *info = pvt_info(mod);

	ci_node_map_each_set(&mod->node_map, node_id)
		ci_worker_map_each_set(&mod->worker_map[node_id], worker_id)
			paver_test_node_worker(mod, node_id, worker_id);

#ifdef PVT_TIMER_DUMP
	static ci_timer_t tm = {
		.flag		= CI_TIMER_PERIODIC,
		.msec 		= 1000,
		.callback 	= paver_test_timer_callback,
	};
	tm.data = mod;
	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);	
#endif	

	static ci_timer_t stop_tm = {
		.msec		= PVT_TEST_DURATION,
		.callback	= paver_test_stop_timer_callback,
	};
	stop_tm.data = mod;
	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &stop_tm);	
	
	ci_printfln("+++ Will start PVT test, duration: %d seconds", PVT_TEST_DURATION / 1000);
	ci_perf_eval_start(&info->perf_data, 1);

	ci_mod_start_done(mod, json);
}


#ifdef PVT_TEST

ci_sched_id_def(pvt_sid, 
	{	.name 		= "pvt.io.smb",
		.desc		= "paver test io",
		.paver 		= ci_str_ary("pvt_res_a_t", "pvt_res_b_t", "pvt_res_c_t")
	},

	{	.name 		= "pvt.io.ret",
		.desc		= "paver test io return path"
	},

	CI_EOT
);


ci_mod_def(mod_paver_test, {
	.name 			= "pvt",
	.desc 			= "paver test",
	.order_start 	= 222,

	.sched_id 		= pvt_sid,
	.node_map		= PVT_NODE_MAP,
	.worker_map		= PVT_WORKER_MAP,

	.vect = {
		[CI_MODV_INIT] 		= paver_test_init,
		[CI_MODV_START] 	= paver_test_start,
	},

	.mem = {
		.size_shr			= ci_sizeof(pvt_info_t),
		.size_node			= ci_sizeof(pvt_node_info_t),
		.size_worker		= ci_sizeof(pvt_worker_info_t)
	}
});

#endif

