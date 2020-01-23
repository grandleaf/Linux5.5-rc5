/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_mod.h				CI Module
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_list.h"
#include "ci_node.h"
#include "ci_json.h"
#include "ci_sched.h"


/*
 *	default start/stop order for a module
 *	a module can define its own start/stop order in its own module file
 *	or [preferred]: define all the things in pal_mod_order_start[] & pal_mod_order_start[]
 */
#define CI_MOD_ORDER_START				127
#define CI_MOD_ORDER_STOP				127
#define CI_MOD_ORDER_MIN				1
#define CI_MOD_ORDER_MAX				255


/* 
 *	vectors/functions for a module 
 *	remember to modify ci_modv_name[] in ci_mod.c if changed
 */
enum {
	CI_MODV_PROBE,						/* set memory requirement, do pre-init configuration */
	CI_MODV_INIT,						/* create module's data structure, memory allocation, etc ... */
	CI_MODV_START,						/* start the module by order_start */	
	CI_MODV_STOP,						/* stop the module by order_stop */
	CI_MODV_SHUTDOWN,					/* clean up */
	CI_MODV_BIST,						/* build in self test */
	CI_MODV_DUMP,						/* status dump */
	CI_MODV_CRASH_DUMP,					/* status dump when creash */
	CI_MODV_NR 							/* total */
};

/*
 *	module state machine 
 *	remember to modify ci_mods_name[] in ci_mod.c if changed
 */
enum {
	CI_MODS_NULL,						/* init value */
	CI_MODS_PROBE,						
	CI_MODS_PROBE_DONE,					
	CI_MODS_INIT,						
	CI_MODS_INIT_DONE,
	CI_MODS_START,
	CI_MODS_START_DONE,
	CI_MODS_STOP,
	CI_MODS_STOP_DONE,
	CI_MODS_SHUTDOWN,
	CI_MODS_SHUTDOWN_DONE
};


typedef void (*ci_mod_vect_t)(ci_mod_t *mod, ci_json_t *json);

typedef struct {
	/* alignment default == 0 == sizeof(void *) */
	u8							 align_shr;
	u8							 align_node;
	u8							 align_worker;
	
	/* input, ci allocates memory according to these values */
	u64							 size_shr;
	u64							 size_node;
	u64							 size_worker;

	/* output, where the memory allocated */
	ci_mem_range_t				 range_shr;
	ci_mem_range_t				 range_vect_sched_grp[CI_NODE_NR];
	ci_mem_range_t				 range_node[CI_NODE_NR];
	ci_mem_range_t				 range_worker[CI_NODE_NR][CI_WORKER_NR];
} ci_mod_mem_t;

typedef struct {
	const char					*name;						/* command name */
	int (*handler)(ci_mod_t *, ci_json_t *);				/* handler */
	const char					*desc;						/* a long description */
	const char					*usage;						/* option/arguments hint */
	int							 flag;					
#define CI_MOD_JCMD_DEBUG			0x0001					/* only show in debug build */
#define CI_MOD_JCMD_HIDE			0x0002					/* hide this command */
} ci_jcmd_t;

typedef struct {
	int			flag;
#define CI_MOD_VECT_PARALLEL		0x0001
	
	int			vect_id;		/* call which vector */
	int			st_exp;			/* info: expected state, -1 means doesn't mind */
	int			st_new;			/* info: new state, -1 means don't transfer to new state */
	int			st_mod_new;		/* mod: new state */

	int			order;			/* start/stop order */
	int 		(*get_order)(ci_mod_t *);	/* function to get order start/stop */
	void		(*all_done)();				/* don't set if get_order() is there */
} ci_mod_vect_param_t;

typedef struct {
	int							 flag;
#define CI_MOD_VECT_TASK_BUSY		0x0001
	
	ci_mod_t					*mod;
	ci_json_t					*json;
	ci_mod_vect_param_t			 param;
	ci_sched_ent_t				 sched_ent;
} ci_mod_vect_task_t;

struct __ci_mod_t{
	const char 					*name;						/* also server as a print prefix, <= CI_PR_MAX_PREFIX_LEN */
	const char					*desc;						/* a long description of name */
	int							 flag;
	int 						 state;						/* CI_MODS_.* */

	int							 order_start;				/* start all the modules in order */
	int							 order_stop;				/* stop all the modules in order */

	ci_mod_vect_t				 vect[CI_MODV_NR];			/* probe, start, stop, etc. */
	ci_jcmd_t					*jcmd;						/* command handler, e.g. cli */

	ci_node_map_t				 node_map;					/* this module runs on these nodes */
	ci_worker_map_t				 worker_map[CI_NODE_NR];	/* this module runs on these workers */
	ci_sched_id_t				*sched_id;					/* schedule identifiers */
	ci_mod_mem_t			 	 mem;						/* memory extension for the module */

	ci_sched_id_t				 vect_sched_id[2];			/* if sched_id is NULL, ci program this */
	ci_sched_grp_t				*vect_sched_grp[CI_NODE_NR][CI_WORKER_NR];		/* for vect call */
	ci_mod_vect_task_t			 vect_task;					/* static allocated for vect call */

	ci_list_t				 	 link;
};

typedef struct {
	int						 	 mod_name;					/* module's name */
	int						 	 mod_desc;					/* module's description */
	int						 	 sched_id_name;				/* schedule identifier's name */
	int							 sched_id_desc;				/* schedule identifier's description */
	int							 jcmd_name;					/* cli command */
	int							 jcmd_desc;					/* description of a jcmd */
	int							 jcmd_usage;				/* usage of a jcmd */
} ci_mod_max_len_t;

typedef struct {
	int							 flag;
#define CI_MODF_READY					0x0001				/* init/start all finished, never cleared */
#define CI_MODF_SHUT_DOWN				0x0002				/* system ask for a shut down */
		
	int							 state;						/* CI_MODS_.* */
	int							 countdown;					/* a counter for probe/init/start ... */
	int							 order;						/* current start/stop order */
	ci_mod_max_len_t			 max_len;					/* store max string len for cli output purpose */
	ci_mem_range_ex_t			 range_vect_sid_name;		/* store module's sid vect name (built-in) */

	int							 vect_node_id;				/* for round robin vect call purpose */
	int							 vect_worker_id;

	ci_list_t					 mod_head;					/* chain all modules */
	ci_slk_t					 lock;
} ci_mod_info_t;


/*
 *	utilities
 */
#define ci_mod_mem_shr(mod)							ci_exec_ptr_not_nil((mod)->mem.range_shr.start)
#define ci_mod_mem_node(mod, node_id)				ci_exec_ptr_not_nil((mod)->mem.range_node[node_id].start)
#define ci_mod_mem_worker(mod, node_id, worker_id)	ci_exec_ptr_not_nil((mod)->mem.range_worker[node_id][worker_id].start)
 
#define ci_mod_each(m, ...)		\
	do {	\
		ci_mod_t *m;		\
		\
		ci_list_each(&ci_mod_info->mod_head, m, link) {	\
			__VA_ARGS__;		\
		}	\
	} while (0)
#define ci_mod_node_each(mod, node, ...)	\
	ci_node_each(node, {	\
		if (ci_node_map_bit_is_set(&(mod)->node_map, node->node_id)) {	\
			__VA_ARGS__;	\
		}	\
	})
#define ci_mod_node_worker_each(mod, node, worker, ...)	\
	ci_node_worker_each(node, worker, {	\
		if (ci_node_map_bit_is_set(&(mod)->node_map, node->node_id) &&	\
			ci_worker_map_bit_is_set(&(mod)->worker_map[node->node_id], worker->worker_id)) 	\
		{	\
			__VA_ARGS__;	\
		}	\
	})
#define ci_mod_by_ctx(ctx)	\
	({	\
		ci_assert((ctx) && (ctx)->sched_grp && (ctx)->sched_grp->mod);	\
		(ctx)->sched_grp->mod;	\
	})


/*
 *	functions
 */
int  ci_mod_pre_init();
int  ci_mod_init();
int  ci_mod_finz();
ci_mod_t *ci_mod_get(const char *name);	
int  ci_mod_dump(ci_mod_t *mod);
void ci_mod_node_worker_iterator(ci_mod_t *mod, void (*iterator)(ci_mod_t *, int, int));	/* iterator with node_id & worker_id */

void ci_mod_probe_done(ci_mod_t *mod, ci_json_t *json);
void ci_mod_init_done(ci_mod_t *mod, ci_json_t *json);
void ci_mod_start_done(ci_mod_t *mod, ci_json_t *json);
void ci_mod_stop_done(ci_mod_t *mod, ci_json_t *json);
void ci_mod_shutdown_done(ci_mod_t *mod, ci_json_t *json);
void ci_mod_bist_done(ci_mod_t *mod, ci_json_t *json);



