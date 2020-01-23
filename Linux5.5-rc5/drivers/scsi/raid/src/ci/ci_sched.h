/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_sched.h					CI Scheduler
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#include "ci_type.h"
#include "ci_list.h"
#include "ci_bmp.h"


/* frequently used */
#define ci_sched_ctx_by_id(node_id, worker_id)		\
	({	\
		ci_worker_t *__ctx_worker__ = ci_worker_by_id(node_id, worker_id);		\
		&__ctx_worker__->sched_tab.ctx;		\
	})
#define ci_sched_task_by_ctx(ctx)		\
	ci_container_of((ctx)->sched_ent, ci_sched_task_t, ent)	
#define ci_node_id_by_sched_task(st)				((st)->ctx->worker->node_id)
#define ci_worker_id_by_sched_task(st)				((st)->ctx->worker->worker_id)
#define ci_sched_id_def(sched_id_name, ...)		\
	static ci_sched_id_t sched_id_name[] = 	\
	{	\
		{	/* room for vector sid, init by ci_sched */	\
		},	\
		\
		__VA_ARGS__		/* for consistency, user must specif a EOT */	\
	}	
#define ci_sched_ctx_check(__node_id, __worker_id)		\
	ci_assert(ci_sched_ctx() == ci_sched_ctx_by_id(__node_id, __worker_id), 	\
			  "wrong sched ctx, expected:%d/%02d, current:%d/%02d", __node_id, __worker_id,		\
			  ci_sched_ctx() ? ci_sched_ctx()->worker->node_id : -1,	\
			  ci_sched_ctx() ? ci_sched_ctx()->worker->worker_id : -1)


/* maps definition */
ci_bmp_def(ci_node_map, CI_NODE_NR);
#define CI_NODE_ALL_MAP								{{ CI_BMP_INITIALIZER(CI_NODE_NR) }}
#define ci_node_map_each_set(bmp, idx)				ci_bmp_each_set(bmp, CI_NODE_NR, idx)

ci_bmp_def(ci_worker_map, CI_WORKER_NR);
#define CI_WORKER_ALL_MAP							{{ CI_BMP_INITIALIZER(CI_WORKER_NR) }}
#define ci_worker_map_each_set(bmp, idx)			ci_bmp_each_set(bmp, CI_WORKER_NR, idx)

ci_bmp_def(ci_sched_prio_map, CI_SCHED_PRIO_NR);
#define CI_SCHED_PRIO_ALL_MAP						{{ CI_BMP_INITIALIZER(CI_SCHED_PRIO_NR) }}
#define ci_sched_prio_map_each_set(bmp, idx)		ci_bmp_each_set(bmp, CI_SCHED_PRIO_NR, idx)

ci_bmp_def(ci_sched_dpt_map, CI_SCHED_DPT_NR);
#define ci_sched_dpt_map_each_set(bmp, idx)			ci_bmp_each_set(bmp, CI_SCHED_DPT_NR, idx)

ci_bmp_def(ci_paver_map, CI_PAVER_NR);
#define ci_paver_map_each_set(bmp, idx)				ci_bmp_each_set(bmp, CI_PAVER_NR, idx)


/* other node/worker map relats */
#define __all_node_worker_helper(n, ...)			{{ __CI_BMP_INITIALIZER(CI_WORKER_NR) }},
#define CI_NODE_WORKER_ALL_MAP						{ ci_m_call_up_to_lv2_8(1, CI_NODE_NR, __all_node_worker_helper) }

#define CI_SCHED_PRIO_MAP(prio)						{{ prio }}							
#define CI_SCHED_PRIO_BIT(prio)						CI_SCHED_PRIO_MAP(1 << (prio))

#ifdef CI_SCHED_DEBUG
#define ci_sched_dbg_exec(...)						ci_dbg_exec(__VA_ARGS__)
#else
#define ci_sched_dbg_exec(...)				
#endif


typedef struct __ci_sched_ctx_t	ci_sched_ctx_t;
typedef struct __ci_sched_tab_t ci_sched_tab_t;


/* schedule entity */
typedef struct {
	int							 flag;				
#define CI_SCHED_ENTF_BUSY_INC					0x0001		/* indicate this ent will +1 nr_busy in sched tab */
#define CI_SCHED_ENTF_BUSY_DEC					0x0002
	
	void	(*exec)(ci_sched_ctx_t *);						/* working function */
	void						*data;						/* for user */
	ci_list_t					 link;						/* chain to grp's head */
} ci_sched_ent_t;

/* schedule task */
typedef struct {
	int							 dpt;						/* schedule descriptor */
	int							 prio;						/* priority */
	ci_sched_ent_t				 ent;						/* schedule entity */
	ci_sched_ctx_t				*ctx;						/* shcedule context */
} ci_sched_task_t;

/* local schedule parameters */
typedef struct {
	int							 burst;						/* burst remain */
	int							 quota;						/* quota remain */
} ci_sched_lparam_t;

/* shared schedule parameters */
typedef struct {
	int							 burst;						/* burst init val */
	int							 quota;						/* quota init val */
} ci_sched_sparam_t;

/* global schedule parameters */
typedef struct {
	int							 dummy;
} ci_sched_gparam_t;

/* schedule group */
typedef struct {
	const char					*name;						/* most time inherit from sid */

	int							 flag;
#define CI_SCHED_GRP_RRR			0x0001					/* the grp in the run/rdy/res table */

	int							 xflag;						/* exclusive flags, access with protection */
#define CI_SCHED_GRP_EXT			0x0001					/* the grp in the external table */

	int							 prio;						/* priority */
	ci_list_t					 ehead;						/* ext head for schedule entities */
	ci_list_t					 rhead;						/* run/rdy head for schedule entities */

	ci_sched_lparam_t			 lparam;					/* local parameters */
	ci_sched_sparam_t			*sparam;					/* pointer to shared parameters */
	ci_sched_gparam_t			*gparam;					/* pointer to global parameters */

	ci_paver_map_t				 paver_map;					/* resource requirements */
	ci_paver_map_t				 paver_wait_map;			/* wait for these resources */

	ci_sched_tab_t				*tab;						/* pointer to schedule table */
	ci_mod_t					*mod;						/* pointer to the module */

#ifdef CI_SCHED_DEBUG
	void	(*exec_check)(ci_sched_ctx_t *);				/* check to see if sched_ent use a fixed exec() */
#endif	
	
	ci_slk_t					 lock;						/* for protection */
	ci_list_t					 elink;						/* chain to external table's head */
	ci_list_t					 rlink;						/* chain to run/rdy table's head */
	ci_list_t					 tlink;						/* a temporary link */

	ci_data_list_t				 res_link[CI_SCHED_GRP_PAVER_NR];	/* link to res table's head[paver_id] */
} ci_sched_grp_t;

/* for async op, we need both submission and returning */
typedef struct {
	ci_sched_task_t			 st_sbm;	/* sched_task for submission */
	ci_sched_task_t			 st_ret;	/* sched_task for returning */
} ci_sched_task_dpt_t;

/* similar to ci_sched_task_dpt_t, a little bit faster */
typedef struct {
	ci_sched_ent_t			 se_sbm;	/* sched_ent for submission */
	ci_sched_grp_t			*sg_sbm;	/* sched_grp for submission */
	
	ci_sched_ent_t			 se_ret;	/* sched_ent for returning */
	ci_sched_grp_t			*sg_ret;	/* sched_grp for returning */
} ci_sched_fast_dpt_t;

/* schedule context */
struct __ci_sched_ctx_t {
	int							 flag;
#define CI_SCHED_CTXF_PRINTF		0x0001					/* print detected in the context */	

	ci_sched_grp_t				*sched_grp;					/* pointer to the sched group */
	ci_sched_ent_t				*sched_ent;					/* pointer to the sched entity */
	struct __ci_worker_t		*worker;					/* pointer to ci_worker */
};		

/* schedule identifier */
typedef struct {
	const char					*name;						/* mandatory, a unique name */
	const char					*desc;						/* mandatory, a description */
	const char				   **paver;						/* paver pool name */
	ci_sched_prio_map_t			 prio_map;					/* optional, set CI_SCHED_PRIO_MAP if empty */
	ci_node_map_t				 node_map;					/* optional, copy from mod_t if empty */
	ci_worker_map_t				 worker_map[CI_NODE_NR];	/* optional, copy from mod_t if empty */

	int							 flag;						/* system use only */
#define CI_SIDF_SYSTEM				0x0001	
} ci_sched_id_t;

/* schedule table for "external" */
typedef struct {
	ci_sched_prio_map_t			 map;							/* head[i] not empty */
	ci_list_t					 head[CI_SCHED_PRIO_NR];		/* chain schedule group by priorities */
	ci_list_t					 head_boost[CI_SCHED_PRIO_NR];	/* if a group doesn't have RRR flag set, boost it priority */
	ci_slk_t					 lock;							/* for protection */
} ci_sched_tab_ext_t;

/* schedule table for "no resource" */
typedef struct {
	ci_paver_map_t				 map;						/* paver resource id map, 0 means out of resource */
	ci_list_t					 head[CI_PAVER_NR];			/* chain schedule group by resource id */
} ci_sched_tab_res_t;

/* schedule table for "run" */
typedef struct {
	int 						 prio_burst;				/* burst this priority */

	ci_sched_prio_map_t			 map;						/* head[i] not empty */
	ci_list_t					 head[CI_SCHED_PRIO_NR];	/* chain schedule group by priorities */
} ci_sched_tab_run_t;

/* schedule table for "ready" */
typedef struct {
	ci_sched_prio_map_t			 map;						/* head[i] not empty */
	ci_list_t					 head[CI_SCHED_PRIO_NR];	/* chain schedule group by priorities */
} ci_sched_tab_rdy_t;

/* configuration data */
typedef struct {
	int							 ext_chk_int;				/* external table check interval */
	ci_sched_sparam_t			 sparam;					/* local param init */
} ci_sched_cfg_t;

/* for each worker: a container for all tables */
struct __ci_sched_tab_t {
	ci_sched_tab_ext_t			 ext;						/* external table */
	ci_sched_tab_res_t			 res;						/* no resource table */
	ci_sched_tab_run_t			 run;						/* run table */
	ci_sched_tab_rdy_t			 rdy;						/* ready table */

	int							 nr_busy;					/* busy counter affected by CI_SCHED_ENTF_BUSY_XXX */
	int							 lvl_busy;					/* level of busy, derived from nr_busy, bigger means busier */
	int							 ext_chk_int;

	ci_sched_cfg_t				*cfg;						/* pointer to the configuration data */
	ci_sched_ctx_t				 ctx;						/* schedule context */
	
	ci_paver_map_t				 pn_map;					/* paver notify map, indicates a paver got buckets from pool */
	ci_slk_t					 pn_map_lock;				/* for pn_map sync purpose */
};

/* schedule look-aside-data */
typedef struct {
	const char					*name;						/* schedule id's name */
	ci_sched_prio_map_t			 map;						/* priority map */
	ci_sched_grp_t				*grp[CI_SCHED_PRIO_NR];		/* pointer to grp_buf[i] if bit i is set in map */
	ci_sched_grp_t				 grp_buf[];					/* schedule groups */
} ci_sched_lad_t;

/* schedule info */
typedef struct {
	ci_sched_cfg_t 			 	 cfg;					/* for schedule parameters */

	int							 nr_lad;					/* how many lads */
	ci_sched_lad_t			   **lad_tab[CI_NODE_NR][CI_WORKER_NR];	/* pointer to the lad table */
	ci_mem_range_t				 lad_buf[CI_NODE_NR][CI_WORKER_NR];	/* lad's buffer allocation */
} ci_sched_info_t;


/* other macros */
#define ci_sched_id_each(mod, sid)	\
	for (ci_sched_id_t *sid = (mod)->sched_id; sid && sid->name; sid++)
#define ci_mod_sched_id_each(mod, sid, ...)	\
	ci_mod_each(mod, {	\
		ci_sched_id_each(mod, sid) {	\
			__VA_ARGS__;	\
		};	\
	})


/* outside might need these */
int  ci_sched_dpt_by_name(const char *name, ci_mod_t **mod, ci_sched_id_t **sid);		/* get descriptor, slow, caller should save it */
int  ci_sched_dpt_by_dpt(int sd, ci_mod_t **mod, ci_sched_id_t **sid);		/* similar to ci_sched_dpt_by_name(), use sd instead of name */
ci_sched_grp_t *ci_sched_grp_by_ctx(ci_sched_ctx_t *ctx, int sd, int prio);

void ci_sched_task(ci_sched_task_t *task);
void ci_sched_task_ext(ci_sched_task_t *task);


/* functions mainly for internal use */
int  ci_sched_pre_init();
int  ci_sched_init();

void ci_sched_tab_init(ci_sched_tab_t *tab);
void ci_sched_ext_add(ci_sched_grp_t *grp, ci_sched_ent_t *ent);
void ci_sched_run_add(ci_sched_grp_t *grp, ci_sched_ent_t *ent);
ci_sched_ent_t *ci_sched_get(ci_sched_tab_t *tab);
void ci_sched_bind_paver_mask();	/* paver should already be inited, let's set the paver mask (resource) */
void ci_sched_paver_ready(ci_sched_tab_t *tab, int paver_id);
void ci_sched_paver_unready(ci_sched_tab_t *tab, int paver_id);




