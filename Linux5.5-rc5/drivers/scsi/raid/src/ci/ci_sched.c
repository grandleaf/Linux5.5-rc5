/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_sched.c					CI Scheduler
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

#define CI_SWDID_SCHED_LAD_TAB		"$sched.lad_tab"

static ci_sched_info_t		sched_info;
static int					ci_wdid_sched_lad_tab;


static void ci_sched_init_cfg()
{
	ci_sched_cfg_t *cfg = &ci_sched_info->cfg;
	ci_sched_sparam_t *sparam = &cfg->sparam;
	
	cfg->ext_chk_int 	= CI_SCHED_EXT_CHK_INT;
	
	sparam->burst		= CI_SCHED_BURST;
	sparam->quota		= CI_SCHED_QUOTA;
}

int ci_sched_pre_init()
{
	ci_sched_info = &sched_info;
	ci_sched_init_cfg();
	return 0;
}

static void ci_sched_grp_init(ci_sched_grp_t *grp)
{
	ci_obj_zero(grp);

	ci_list_init(&grp->ehead);
	ci_list_init(&grp->rhead);
	ci_slk_init(&grp->lock);

	grp->sparam = &ci_sched_info->cfg.sparam;
	grp->lparam.burst = grp->sparam->burst;
	grp->lparam.quota = grp->sparam->quota;

	ci_loop(idx, CI_SCHED_GRP_PAVER_NR)
		grp->res_link[idx].data = grp;
}

static int ci_sched_calc_lad_buf(int node_id, int worker_id)
{
	int size = 0;

	ci_mod_sched_id_each(mod, sid, {
		if (ci_node_map_bit_is_set(&sid->node_map, node_id) &&
			ci_worker_map_bit_is_set(&sid->worker_map[node_id], worker_id))
		{
			int nr_prio = ci_sched_prio_map_count_set(&sid->prio_map);
			ci_assert(nr_prio);
			size += ci_sizeof(ci_sched_lad_t) + ci_sizeof(ci_sched_grp_t) * nr_prio;
		}
	});

	return size;
}

static void ci_sched_lad_tab_buf(int node_id, int worker_id, ci_sched_lad_t **lad_tab, ci_mem_range_t *range)
{
	ci_sched_lad_t *lad;
	ci_sched_grp_t *grp;
	u8 *ptr = range->start;
	int top = 1, btm = CI_SCHED_DPT_NR - 1, sd;		/* 0 is unused */

#ifdef CI_DEBUG
	ci_mod_sched_id_each(mod0, sid0, {
		ci_mod_sched_id_each(mod1, sid1, {
			ci_assert(sid0->name && sid1->name);
			if ((sid0 != sid1) && ci_strequal(sid0->name, sid1->name)) 
				ci_panic("duplicate sid name detected: sid_name=\"%s\", mod0=\"%s\", mod1=\"%s\"", 
						 sid0->name, mod0->name, mod1->name);
		});
	});
#endif

	ci_mod_sched_id_each(mod, sid, {
		ci_assert(top < btm, "CI_SCHED_DPT_NR is to small");

		if (ci_node_map_bit_is_set(&sid->node_map, node_id) &&
			ci_worker_map_bit_is_set(&sid->worker_map[node_id], worker_id))
		{
			lad = (ci_sched_lad_t *)ptr;
			lad->name = sid->name;
			ci_sched_prio_map_copy(&lad->map, &sid->prio_map);

			grp = lad->grp_buf;
			ci_assert(!ci_sched_prio_map_empty(&lad->map));
			ci_sched_prio_map_each_set(&lad->map, prio) {
				/* init the group, will do more set up later */
				ci_sched_grp_init(grp);
				grp->name 	= sid->name;
				grp->mod 	= mod;
				grp->prio 	= prio;
				grp->tab 	= &ci_worker_by_id(node_id, worker_id)->sched_tab;

				lad->grp[prio] = grp++;		/* set reference */
			}
			
			ptr += ci_sizeof(ci_sched_lad_t) + ci_sizeof(ci_sched_grp_t) * ci_sched_prio_map_count_set(&sid->prio_map);
			ci_assert(ptr == (u8 *)grp);
			sd = sid->flag & CI_SIDF_SYSTEM ? btm : top;
			lad_tab[sd] = lad;
			ci_sched_dpt_map_set_bit(&ci_node_info->sched_dpt_map, sd);
		}		
		
		sid->flag & CI_SIDF_SYSTEM ? btm-- : top++;
	});

	ci_assert(ptr == range->end);
}

static void ci_sched_init_lad()
{
	ci_sched_dpt_map_zero(&ci_node_info->sched_dpt_map);
	ci_sched_dpt_map_set_bit(&ci_node_info->sched_dpt_map, 0);	/* 0 is reserved/unused */

	ci_node_worker_each(node, worker, {
		u8 *buf;
		ci_sched_lad_t **lad_tab;
		int size, node_id = node->node_id, worker_id = worker->worker_id;
		
		/* step 1, allocate the lad table *ptr[] */
		size = ci_sizeof(ci_sched_lad_t *) * CI_SCHED_DPT_NR;
		ci_sched_info->lad_tab[node_id][worker_id] = lad_tab = 
			ci_node_halloc(node->node_id, size, 0, ci_ssf("sched_lad_tab.%d.%02d", node_id, worker_id));;
		ci_memzero(lad_tab, size);
		ci_worker_data_set(ci_sched_ctx_by_id(node_id, worker_id), ci_wdid_sched_lad_tab, lad_tab);

		/* step 2, allocate the lad buffer */	
		if ((size = ci_sched_calc_lad_buf(node_id, worker_id))) {	/* not usual, idle worker? */
			ci_sched_info->lad_buf[node_id][worker_id].start = buf = 
				ci_node_halloc(node->node_id, size, 0, ci_ssf("sched_lad_buf.%d.%02d", node_id, worker_id));
			ci_sched_info->lad_buf[node_id][worker_id].end = buf + size;
			ci_memzero(buf, size);
		}

		/* step 3, init lad table & lad buffer */
		ci_sched_lad_tab_buf(node_id, worker_id, lad_tab, &ci_sched_info->lad_buf[node_id][worker_id]);
	});
}

int ci_sched_dpt_by_name(const char *name, ci_mod_t **mod, ci_sched_id_t **sid)
{
	ci_sched_lad_t *lad, **lad_tab;

	ci_node_worker_each(node, worker, {
		lad_tab = ci_sched_info->lad_tab[node->node_id][worker->worker_id];

		ci_loop(sd, 1, CI_SCHED_DPT_NR)
			if ((lad = lad_tab[sd]) && ci_strequal(lad->name, name)) {
				if (mod || sid) 
					ci_mod_sched_id_each(m, s, {
						if (ci_strequal(s->name, name)) {
							mod && ((*mod) = m);
							sid && ((*sid) = s);
							return sd;
						}
					});

				ci_assert(!mod && !sid);
				return sd;
			}
	});

	ci_bug();
	return -1;
}

int ci_sched_dpt_by_dpt(int sd, ci_mod_t **mod, ci_sched_id_t **sid)
{
	ci_sched_lad_t *lad, **lad_tab;

	ci_node_worker_each(node, worker, {
		lad_tab = ci_sched_info->lad_tab[node->node_id][worker->worker_id];

		if ((lad = lad_tab[sd])) {
			if (mod || sid) 
				ci_mod_sched_id_each(m, s, {
					if (ci_strequal(s->name, lad->name)) {
						mod && ((*mod) = m);
						sid && ((*sid) = s);
						return sd;
					}
				});

			ci_assert(!mod && !sid);
			return sd;
		}
	});

	return -1;	
}

ci_sched_grp_t *ci_sched_grp_by_ctx(ci_sched_ctx_t *ctx, int sd, int prio)
{
	ci_sched_lad_t *lad, **lad_tab;
	
	lad_tab = (ci_sched_lad_t **)ci_worker_data_by_ctx(ctx, ci_wdid_sched_lad_tab);
	ci_range_check(sd, 1, CI_SCHED_DPT_NR, "invalid sched_dpt=%d", sd);
	
	if (ci_unlikely(!(lad = lad_tab[sd])))		/* not available in this node/worker */
		return NULL;

	if (ci_unlikely(prio < 0))
		prio = ci_sched_prio_map_last_set(&lad->map);

	ci_range_check(prio, 0, CI_SCHED_PRIO_NR);
	return lad->grp[prio];
}

static void ci_sched_init_vector_sched_grp(ci_mod_t *mod)
{
	int size;
	ci_sched_grp_t *grp;
	ci_mem_range_t *range;
	ci_worker_map_t *worker_map;

	ci_node_map_each_set(&mod->node_map, node_id) {		/* node memory */
		if (ci_worker_map_empty(worker_map = &mod->worker_map[node_id]))
			continue;

		range = &mod->mem.range_vect_sched_grp[node_id];
		size = ci_sizeof(ci_sched_grp_t) * ci_worker_map_count_set(worker_map);
		range->start = ci_node_halloc(node_id, size , 0, ci_ssf("mod.vsg.%s.%d", mod->name, node_id));
		range->end =  range->start + size;	

		grp = (ci_sched_grp_t *)range->start;
		ci_worker_map_each_set(worker_map, worker_id) {
			ci_assert((u8 *)grp < range->end);
			mod->vect_sched_grp[node_id][worker_id] = grp;

			/* init */
			ci_sched_grp_init(grp);
			grp->name 	= "mod_vector_sched_grp";
			grp->mod 	= mod;
			grp->prio 	= CI_SCEHD_PRIO_MOD_VECT;
			grp->tab	= &ci_worker_by_id(node_id, worker_id)->sched_tab;			
			grp++;
		}

		ci_assert((u8 *)grp == range->end);
	}
}

int ci_sched_init()
{
	ci_imp_printfln("ci_sched_init()");
	ci_wdid_sched_lad_tab = ci_worker_data_register(CI_SWDID_SCHED_LAD_TAB);		/* register per worker data */
	ci_mod_each(m, ci_sched_init_vector_sched_grp(m));
	ci_sched_init_lad();
	
	return 0;
}

void ci_sched_tab_init(ci_sched_tab_t *tab)
{
	ci_sched_tab_ext_t *ext = &tab->ext;
	ci_sched_tab_res_t *res = &tab->res;
	ci_sched_tab_run_t *run = &tab->run;
	ci_sched_tab_rdy_t *rdy = &tab->rdy;

	ci_obj_zero(tab);
	tab->cfg = &ci_sched_info->cfg;
	tab->ext_chk_int = tab->cfg->ext_chk_int;

	/* init the external table */
	ci_sched_prio_map_zero(&ext->map);
	ci_list_ary_init(ext->head);
	ci_list_ary_init(ext->head_boost);
 	ci_slk_init(&ext->lock);

	/* init the res table */
	ci_paver_map_zero(&res->map);
	ci_list_ary_init(res->head);

	/* init the run table */
	ci_sched_prio_map_zero(&run->map);
	ci_list_ary_init(run->head);
	run->prio_burst = -1;

	/* init the rdy table */
	ci_sched_prio_map_zero(&rdy->map);
	ci_list_ary_init(rdy->head);

	/* init the paver notify map */
	ci_paver_map_zero(&tab->pn_map);
	ci_slk_init(&tab->pn_map_lock);
}

void ci_sched_bind_paver_mask()	/* paver should already be inited, let's set the paver mask (resource) */
{
	ci_sched_id_t *sid;
	ci_sched_grp_t *grp;
	const char **paver_name;
	int paver_id;

	ci_sched_dpt_map_each_set(&ci_node_info->sched_dpt_map, sd) {
		if ((ci_sched_dpt_by_dpt(sd, NULL, &sid) < 0) || !sid || !sid->paver)
			continue;

		ci_node_map_each_set(&sid->node_map, node_id)
			ci_worker_map_each_set(&sid->worker_map[node_id], worker_id)
				ci_sched_prio_map_each_set(&sid->prio_map, prio_id) {
					grp = ci_sched_grp_by_ctx(ci_sched_ctx_by_id(node_id, worker_id), sd, prio_id);
					ci_assert(grp);
					ci_paver_map_zero(&grp->paver_map);
					
					for (paver_name = sid->paver; *paver_name; paver_name++) {
						paver_id = ci_paver_id_by_name(*paver_name);
						ci_assert(paver_id >= 1, "cannot find paver id for paver pool name: \"%s\"", *paver_name);
						ci_paver_map_set_bit(&grp->paver_map, paver_id);
					}
				}
	}
}

static void ci_sched_tab_inc_busy(ci_sched_tab_t *tab)
{
	int new_lvl_busy, new_nr_busy;

	new_nr_busy = ci_atomic_inc_fetch(&tab->nr_busy);
	ci_assert(new_nr_busy >= 1);

	if (ci_unlikely((new_lvl_busy = ci_sched_busy_inc_get_lvl(tab->lvl_busy, new_nr_busy)) != tab->lvl_busy)) {
		tab->lvl_busy = new_lvl_busy;
		printf("+ nr_busy=%d, new_lvl_busy=%d\n", tab->nr_busy, new_lvl_busy);
	}
}

/* from outward, another context, add a schedule entity to a group */
void ci_sched_ext_add(ci_sched_grp_t *grp, ci_sched_ent_t *ent)
{
	int in_ext = 0;
	ci_sched_tab_t *tab = grp->tab;

	ci_range_check(grp->prio, 0, CI_SCHED_PRIO_NR);

	ci_assert(!ci_flag_all_set(ent->flag, CI_SCHED_ENTF_BUSY_INC | CI_SCHED_ENTF_BUSY_DEC));
	if (ci_unlikely(ent->flag & CI_SCHED_ENTF_BUSY_INC)) 
		ci_sched_tab_inc_busy(tab);

	ci_slk_protected(&grp->lock, {
		ci_list_add_tail(&grp->ehead, &ent->link);
		in_ext = grp->xflag & CI_SCHED_GRP_EXT;
	});

	if (ci_unlikely(!in_ext)) {
		ci_sched_tab_ext_t *ext = &tab->ext;

		ci_slk_lock(&ext->lock);	/* lock order is import for avoiding dead lock */
		ci_slk_lock(&grp->lock);

		if (!(grp->xflag & CI_SCHED_GRP_EXT) && !ci_list_empty(&grp->ehead)) {	/* other threads not changed it yet */
			grp->xflag |= CI_SCHED_GRP_EXT;
			if (grp->flag & CI_SCHED_GRP_RRR) {	/* grp is already in RRR */
				ci_list_add_tail(&ext->head[grp->prio], &grp->elink);
				ci_sched_prio_map_set_bit(&ext->map, grp->prio);
			} else { 	/* grp not in RRR, we like force it goes into R to avoid starvation */
				ci_assert(grp->prio > 0, "hightest sched priority is 1, 0 is reserved");
				ci_list_add_tail(&ext->head_boost[grp->prio], &grp->elink);
				ci_sched_prio_map_set_bit(&ext->map, grp->prio - 1);
			}
		}

		ci_slk_unlock(&grp->lock);
		ci_slk_unlock(&ext->lock);
	}

	ci_worker_soft_irq(tab->ctx.worker);
}

/* from internal, the same context, add a schedule entity to a group */
void ci_sched_run_add(ci_sched_grp_t *grp, ci_sched_ent_t *ent)
{
	ci_sched_tab_t *tab = grp->tab;
	ci_sched_tab_run_t *run = &tab->run;

	ci_range_check(grp->prio, 0, CI_SCHED_PRIO_NR);
	ci_list_add_tail(&grp->rhead, &ent->link);

	/* already active or waiting for resource */
	if (grp->flag & CI_SCHED_GRP_RRR) 
		return;

	/* activate */
	grp->flag |= CI_SCHED_GRP_RRR;
	ci_list_add_tail(&run->head[grp->prio], &grp->rlink);
	ci_sched_prio_map_set_bit(&run->map, grp->prio);	
	ci_worker_soft_irq(tab->ctx.worker);
}

static inline void __ci_sched_delay_head_add_grp(ci_list_t *delay_head, ci_sched_grp_t *grp)
{
	ci_slk_lock(&grp->lock);
	ci_assert(!ci_list_empty(&grp->ehead));
	ci_list_merge_tail(&grp->rhead, &grp->ehead);	/* move entities from ext to run/res/rdy */
	ci_list_del(&grp->elink);	/* del grp from ext */
	grp->xflag &= ~CI_SCHED_GRP_EXT;		
	ci_slk_unlock(&grp->lock);

	ci_list_add_tail(delay_head, &grp->tlink);
}

/* move sched_grp from ext to run */
static int ci_sched_ext_to_run(ci_sched_tab_run_t *run, ci_sched_tab_ext_t *ext)
{
	ci_sched_grp_t *grp;
	ci_list_def(delay_head);
	
	ci_slk_lock(&ext->lock);

	ci_sched_prio_map_each_set(&ext->map, prio) {
		ci_list_each_safe(&ext->head[prio], grp, elink) 
			__ci_sched_delay_head_add_grp(&delay_head, grp);

		if (prio < CI_SCHED_PRIO_NR)
			ci_list_each_safe(&ext->head_boost[prio + 1], grp, elink) 
				__ci_sched_delay_head_add_grp(&delay_head, grp);
	}

	ci_sched_prio_map_zero(&ext->map);
	ci_slk_unlock(&ext->lock);

	ci_list_each(&delay_head, grp, tlink) 
		if (ci_unlikely(!(grp->flag & CI_SCHED_GRP_RRR))) {		/* not in the run/rdy/res table */
			ci_list_add_tail(&run->head[grp->prio], &grp->rlink);	/* add to run table */
			grp->flag |= CI_SCHED_GRP_RRR;
			ci_sched_prio_map_set_bit(&run->map, grp->prio);
		}

	ci_list_dbg_del_all(&delay_head);			/* so it not next panic */
	return ci_sched_prio_map_first_set(&run->map);
}

static ci_sched_grp_t *ci_sched_grp_by_task(ci_sched_task_t *task)
{
	ci_sched_grp_t *grp = ci_sched_grp_by_ctx(task->ctx, task->dpt, task->prio);
	ci_assert(grp, "cannot find sched_grp for node_id:%d, worker_id:%d, sched_dpt:%d, prio:%d\n",
					task->ctx->worker->node_id, task->ctx->worker->worker_id,
					task->dpt, task->prio);
	return grp;
}

void ci_sched_task(ci_sched_task_t *task)
{
	ci_sched_grp_t *grp = ci_sched_grp_by_task(task);
	
	ci_assert(task->ctx == ci_sched_ctx(), "sched in a different context, please use safe version ci_sched_task_ext()");
	ci_sched_run_add(grp, &task->ent);
}

void ci_sched_task_ext(ci_sched_task_t *task)
{
	ci_sched_grp_t *grp = ci_sched_grp_by_task(task);
	
//	ci_assert(task->ctx != ci_sched_ctx(), "sched in the same context, please use fast version ci_sched_task()");
	ci_sched_ext_add(grp, &task->ent);
}

/* move from rdy to run, return the highest priority in run */
static int ci_sched_rdy_to_run(ci_sched_tab_run_t *run, ci_sched_tab_rdy_t *rdy)
{
	int prio_rdy, prio_run;

	if ((prio_rdy = ci_sched_prio_map_first_set(&rdy->map)) < 0)	/* nothing in the rdy tab */
		return ci_sched_prio_map_first_set(&run->map);	/* return run's highest priority */

	if (ci_unlikely((prio_run = ci_sched_prio_map_first_set(&run->map)) < 0) ||		/* run is empty for the priority */
		ci_unlikely((prio_rdy < prio_run))) 	/* or run not empty, but rdy has a higher priority */
	{	
		/* move from rdy to run */
		ci_list_merge_tail(&run->head[prio_rdy], &rdy->head[prio_rdy]);
		ci_sched_prio_map_clear_bit(&rdy->map, prio_rdy);
		ci_sched_prio_map_set_bit(&run->map, prio_rdy);
		return prio_rdy;	/* now the highest priority of run is prio_rdy */
	}

	return prio_run; 	/* run still has the highest priority */
}

static inline void ci_sched_grp_rrr_del(ci_list_t *head, ci_sched_grp_t *grp, ci_sched_tab_run_t *run, int prio)
{
	ci_list_del(&grp->rlink);

	if (ci_unlikely(ci_list_empty(head)))	/* unset the bitmap */
		ci_sched_prio_map_clear_bit(&run->map, prio);
}

static int ci_sched_apply_policy(ci_list_t *head, ci_sched_grp_t *grp, ci_sched_tab_run_t *run, ci_sched_tab_rdy_t *rdy, int prio)
{
	int burst_prio;
	ci_sched_lparam_t *lparam;

	if (ci_unlikely((ci_list_empty(&grp->rhead)))) {	/* no entities in the group */
		ci_sched_grp_rrr_del(head, grp, run, prio);
		grp->flag &= ~CI_SCHED_GRP_RRR;		/* not in run/rdy/res */
		return -1;
	}

	burst_prio = -1;
	lparam = &grp->lparam;
	ci_assert((lparam->quota >= 1) && (lparam->burst >= 1));
	
	if (--lparam->quota) {		/* still have quota */
		if (--grp->lparam.burst)	/* still can burst */
			burst_prio = prio;
		else {
			grp->lparam.burst = grp->sparam->burst;
			ci_list_del(&grp->rlink);	/* move to run head's tail */	
			ci_list_add_tail(head, &grp->rlink);
		}
	} else {	/* run out of quota */
		lparam->quota = grp->sparam->quota;
		lparam->burst = grp->sparam->burst;
		
		ci_list_del(&grp->rlink);	/* move to rdy head's tail */	
		ci_list_add_tail(&rdy->head[prio], &grp->rlink);

		ci_sched_prio_map_set_bit(&rdy->map, prio);
		if (ci_unlikely(ci_list_empty(head)))	/* unset the bitmap */
			ci_sched_prio_map_clear_bit(&run->map, prio);
	}

	return burst_prio;
}

static void ci_sched_grp_no_res(ci_sched_tab_t *tab, ci_sched_grp_t *grp, ci_paver_map_t *res_map)
{
	int idx = 0;
	ci_sched_tab_run_t *run = &tab->run;
	ci_sched_tab_res_t *res = &tab->res;

//ci_printf("4444444444444444 no resource\n");	
	ci_paver_map_copy(&grp->paver_wait_map, res_map);
	ci_sched_grp_rrr_del(&run->head[grp->prio], grp, run, grp->prio);
	run->prio_burst = -1;		/* break the burst */

	ci_paver_map_each_set(res_map, p_idx) {
		ci_list_add_tail(&res->head[p_idx], &grp->res_link[idx++].link);
		ci_assert(idx <= CI_SCHED_GRP_PAVER_NR, "out of memory, please increase CI_SCHED_GRP_PAVER_NR");
	}
}

/* worker call this to grab a schedule entity */
ci_sched_ent_t *ci_sched_get(ci_sched_tab_t *tab)
{
	ci_list_t *head;
	ci_sched_grp_t *grp;
	ci_sched_ent_t *ent;
	ci_paver_map_t res_map;
	ci_sched_ctx_t *ctx = &tab->ctx;
	ci_sched_tab_ext_t *ext = &tab->ext; 
	ci_sched_tab_run_t *run = &tab->run;
	ci_sched_tab_rdy_t *rdy = &tab->rdy;
	int prio, prio_ext, ext_checked = 0;


	/* step 1. burst ? */
	if (run->prio_burst >= 0) {
		prio = run->prio_burst;
		tab->ext_chk_int--;
		goto __prio_sched;
	}


__again:
	/* step 2. move from rdy to run if need, then try move ext to run if need */
	if (ci_unlikely((prio = ci_sched_rdy_to_run(run, rdy)) < 0)) {	/* empty */
		if (ci_unlikely((prio = ci_sched_ext_to_run(run, ext)) < 0))	/* slow, return if ext also empty */
			return NULL;
		ext_checked = 1;
	}
	ci_range_check(prio, 0, CI_SCHED_PRIO_NR);


	/* step 3. to see if we need check ext and move ext to run */
	if (ci_unlikely(!ext_checked && ci_unlikely(--tab->ext_chk_int <= 0))) {
		tab->ext_chk_int = tab->cfg->ext_chk_int;
		if (((prio_ext = ci_sched_prio_map_first_set(&ext->map)) >= 0) && (prio_ext < prio)) 
			prio = ci_sched_ext_to_run(run, ext);
	}
	ci_range_check(prio, 0, CI_SCHED_PRIO_NR);
	

	/* step 4. get candidate from the run table */
__prio_sched:	
	head = &run->head[prio];
	grp = ci_list_head_obj_not_nil(head, ci_sched_grp_t, rlink);


	/* step 5. ci_todo resource check move to res */
	ci_paver_map_sub(&res_map, &grp->paver_map, &tab->res.map);
	if (ci_unlikely(!ci_paver_map_is_all_clear(&res_map))) {
		ci_sched_grp_no_res(tab, grp, &res_map);
		goto __again;
	}


	/* step 6. yes, we have resource */
	ent = ci_list_head_obj_not_nil(&grp->rhead, ci_sched_ent_t, link);
	ci_list_del(&ent->link);
	

	/* step 7. apply quota/burst policy */
	run->prio_burst = ci_sched_apply_policy(head, grp, run, rdy, prio);
	
		
	/* step 8. all done */
	ctx->sched_ent = ent;
	ctx->sched_grp = grp;
	return ent;
}

void ci_sched_paver_ready(ci_sched_tab_t *tab, int paver_id)
{
	ci_sched_grp_t *grp;
	ci_data_list_t *data_link;
	ci_sched_prio_map_t grp_map;
	ci_list_t grp_head[CI_SCHED_PRIO_NR];
	
	ci_sched_tab_run_t *run = &tab->run;
	ci_sched_tab_res_t *res = &tab->res;
	ci_list_t *res_head = &res->head[paver_id];

//ci_printfln("+++ sched paver ready, paver_id=%d", paver_id);
	/* init grp_map for grp_head */
	ci_range_check(paver_id, 1, CI_PAVER_NR);
	ci_sched_prio_map_zero(&grp_map);

	/* set the paver ready bit */
	ci_assert(ci_paver_map_bit_is_clear(&res->map, paver_id));
	ci_paver_map_set_bit(&res->map, paver_id);

	/* empty the res_head, queue grp into grp_head if waiting for nothing */
	ci_list_each(res_head, data_link, link) {
		grp = data_link->data;
		ci_paver_map_clear_bit(&grp->paver_wait_map, paver_id);
		
		if (ci_paver_map_is_all_clear(&grp->paver_wait_map)) {
//ci_printf("xxxxxxxxxx all clear\n");
			ci_list_t *head = &grp_head[grp->prio];
			if (ci_unlikely(ci_sched_prio_map_bit_is_clear(&grp_map, grp->prio))) {
				ci_sched_prio_map_set_bit(&grp_map, grp->prio);
				ci_list_init(head);
			}
			ci_list_add_tail(head, &grp->rlink);
		}
//else ci_printf("nononononononononononono\n"), ci_paver_map_dump(&grp->paver_wait_map), ci_printfln();		
	}

	/* clear the res_head so it will not panic in debug mode */
	ci_list_dbg_del_all(res_head);

	/* set empty */
	ci_list_init(res_head);

	/* no grp dequeued */
	if (ci_unlikely(ci_sched_prio_map_is_all_clear(&grp_map)))
		return;

	/* insert all ready grps chains into run->head[prio] */
	ci_sched_prio_map_each_set(&grp_map, prio) 
		ci_list_merge_head(&run->head[prio], &grp_head[prio]);

	/* don't forget to set the "available" map */
	if (ci_unlikely(ci_sched_prio_map_is_all_clear(&run->map)))
		ci_sched_prio_map_or_asg(&run->map, &grp_map);

	/* break the burst */
	run->prio_burst = -1;		
}

void ci_sched_paver_unready(ci_sched_tab_t *tab, int paver_id)
{
//ci_printfln("--- sched paver unready, paver_id=%d", paver_id);
	ci_paver_map_clear_bit(&tab->res.map, paver_id);
}



