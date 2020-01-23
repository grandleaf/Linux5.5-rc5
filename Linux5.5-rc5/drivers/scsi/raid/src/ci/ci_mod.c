/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_mod.c				CI Module
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

static ci_mod_info_t mod_info;

static void mod_probe_all();
static void mod_probe_all_done();
static void mod_init_all();
static void mod_init_all_done();
static void mod_start_all();
static void mod_start_all_continue();
static void mod_start_all_done();
static void mod_stop_all();
static void mod_stop_all_continue();
static void mod_stop_all_done();
static void mod_shutdown_all();
static void mod_shutdown_all_done();


static ci_int_to_name_t ci_mods_name[] = {
	ci_int_name(CI_MODS_NULL),
	ci_int_name(CI_MODS_PROBE),
	ci_int_name(CI_MODS_PROBE_DONE),
	ci_int_name(CI_MODS_INIT),
	ci_int_name(CI_MODS_INIT_DONE),
	ci_int_name(CI_MODS_START),
	ci_int_name(CI_MODS_START_DONE),
	ci_int_name(CI_MODS_STOP),
	ci_int_name(CI_MODS_STOP_DONE),
	ci_int_name(CI_MODS_SHUTDOWN),
	ci_int_name(CI_MODS_SHUTDOWN_DONE),
	CI_EOT	
};

static ci_int_to_name_t ci_modv_name[] = {
	ci_int_name(CI_MODV_PROBE),
	ci_int_name(CI_MODV_INIT),
	ci_int_name(CI_MODV_START),
	ci_int_name(CI_MODV_STOP),
	ci_int_name(CI_MODV_SHUTDOWN),
	ci_int_name(CI_MODV_BIST),
	ci_int_name(CI_MODV_DUMP),
	ci_int_name(CI_MODV_CRASH_DUMP),
	CI_EOT
};

static void mod_check(ci_mod_t *mod)
{
	ci_assert(mod->name, "module name cannot be NULL");
	ci_assert(mod->desc, "module description cannot be NULL");
	ci_assert(ci_strlen(mod->name) <= CI_PR_MAX_PREFIX_LEN, 
			  "module name \"%s\" is longer than %d", mod->name, CI_PR_MAX_PREFIX_LEN);
	ci_assert(!ci_node_map_empty(&mod->node_map));
	ci_sched_id_each(mod, sid) 
		ci_assert(sid->desc, "sched_id description for \"%s\" cannot be NULL", sid->name);
}

static void mod_set_dft(ci_mod_t *mod)
{
	pal_mod_cfg_t *cfg;
	ci_sched_id_t *sid;
	ci_mem_range_ex_t *mem;

	/* set order_start & order_stop */
	if ((cfg = pal_mod_cfg(mod->name))) {
		if (cfg->order_start) {
			if (mod->order_start) 
				ci_warn_printf("mod \"%s\" order_start overrided: %d -> %d\n", mod->name, mod->order_start, cfg->order_start);
			mod->order_start = cfg->order_start;
		}

		if (cfg->order_stop) {
			if (mod->order_stop) 
				ci_warn_printf("mod \"%s\" order_stop overrided: %d -> %d\n", mod->name, mod->order_stop, cfg->order_stop);
			mod->order_stop = cfg->order_stop;
		}
	}
	mod->order_start || (mod->order_start = CI_MOD_ORDER_START);
	mod->order_stop  || (mod->order_stop  = CI_MOD_ORDER_STOP);
	ci_range_check_i(mod->order_start, CI_MOD_ORDER_MIN, CI_MOD_ORDER_MAX);
	ci_range_check_i(mod->order_stop,  CI_MOD_ORDER_MIN, CI_MOD_ORDER_MAX);


	/* set default node map */
	if (ci_node_map_empty(&mod->node_map))	
		ci_node_map_copy(&mod->node_map, &ci_node_info->node_com_map);
	else if (ci_node_map_full(&mod->node_map))
		ci_node_map_mask(&mod->node_map, 0, ci_node_info->nr_node);
	ci_assert(ci_node_map_last_set(&mod->node_map) < ci_node_info->nr_node);


	/* set default worker map in a node */
	ci_node_each(node, {
		ci_worker_map_t *worker_map = &mod->worker_map[node->node_id];

		ci_assert(node->nr_worker);
		if (ci_worker_map_empty(worker_map)) 
			ci_worker_map_copy(worker_map, &node->worker_com_map);
		else if (ci_worker_map_full(worker_map))
			ci_worker_map_mask(worker_map, 0, node->nr_worker);
		ci_assert(ci_worker_map_last_set(worker_map) < node->nr_worker);
	});


	/* config built-in sid */
	if (!mod->sched_id) 
		mod->sched_id = mod->vect_sched_id;
	mem = &ci_mod_info->range_vect_sid_name;
	sid = mod->sched_id;
	sid->name = (const char *)mem->curr;
	sid->desc = "mod vect built-in";
	sid->flag |= CI_SIDF_SYSTEM;
	ci_sched_prio_map_set_bit(&sid->prio_map, CI_SCEHD_PRIO_MOD_VECT);
	ci_mem_printf(mem, "%s.mod_vect_sid*", mod->name);
	mem->curr++;

	/* config all sids */
	ci_sched_id_each(mod, sid) {
		if (ci_sched_prio_map_empty(&sid->prio_map))	/* set default priority */
			ci_sched_prio_map_set_bit(&sid->prio_map, CI_SCHED_PRIO_NORMAL);
		ci_assert(ci_node_map_last_set(&sid->prio_map) < CI_SCHED_PRIO_NR);
		
		if (ci_node_map_empty(&sid->node_map)) 			/* copy from mod->node_map if empty */
			ci_node_map_copy(&sid->node_map, &mod->node_map);
		else if (ci_node_map_full(&sid->node_map)) 		/* run on all the nodes */
			ci_node_map_mask(&sid->node_map, 0, ci_node_info->nr_node);

#ifdef CI_DEBUG
		ci_node_map_t node_check_map;
		ci_node_map_sub(&node_check_map, &sid->node_map, &mod->node_map);
		if (!ci_node_map_empty(&node_check_map)) {
			ci_err_printfln("sched_id's node_map not included in module's node_map");
			ci_printf("module=\"%s\", sid=\"%s\"", mod->name, sid->name);
			ci_dump_pre(", sid->node_map=", ci_node_map_dump(&sid->node_map));
			ci_dump_preln(", mod->node_map=", ci_node_map_dump(&mod->node_map));
			ci_bug();
		}
#endif

		ci_node_map_each_set(&sid->node_map, node_id) {
			ci_worker_map_t *worker_map = &sid->worker_map[node_id];
			if (ci_worker_map_empty(worker_map))
				ci_worker_map_copy(worker_map, &mod->worker_map[node_id]);
						
#ifdef CI_DEBUG
			ci_worker_map_t worker_check_map;
			ci_worker_map_sub(&worker_check_map, worker_map, &mod->worker_map[node_id]);
			if (!ci_worker_map_empty(&worker_check_map)) {
				ci_err_printfln("sched_id's worker_map[%d] not included in module's worker_map[%d]", node_id, node_id);
				ci_printf("module=\"%s\", sid=\"%s\"", mod->name, sid->name);
				ci_printf(", sid->worker_map[%d]=", node_id);
				ci_worker_map_dump(&sid->worker_map[node_id]);
				ci_printf(", mode->worker_map[%d]=", node_id);
				ci_worker_map_dumpln(&mod->worker_map[node_id]);
				ci_bug();
			}
#endif
		}
	}
}

static void mod_malloc(ci_mod_t *mod)
{
	ci_mod_mem_t *mem = &mod->mem;

	if (mem->size_shr) {	/* shared memory */
		mem->range_shr.start = ci_shr_halloc(mem->size_shr, mem->align_shr, ci_ssf("mod.%s.shr", mod->name));
		mem->range_shr.end = mem->range_shr.start + mem->size_shr;
	}

	ci_node_map_each_set(&mod->node_map, node_id) {		/* node memory */
		ci_worker_map_t *worker_map = &mod->worker_map[node_id];

		if (mem->size_node) {
			mem->range_node[node_id].start = ci_node_halloc(node_id, mem->size_node, mem->align_node, 
															ci_ssf("mod.%s.%d", mod->name, node_id));
			mem->range_node[node_id].end = mem->range_node[node_id].start + mem->size_node;
		}

		ci_worker_map_each_set(worker_map, worker_id) {		/* per cpu memory */
			ci_assert(worker_id < ci_node_by_id(node_id)->nr_worker);
			if (mem->size_worker) {
				mem->range_worker[node_id][worker_id].start = ci_node_halloc(node_id, mem->size_worker, mem->align_worker, 
																	   	     ci_ssf("mod.%s.%d.%02d", mod->name, node_id, worker_id));
				mem->range_worker[node_id][worker_id].end = mem->range_worker[node_id][worker_id].start + mem->size_worker;
			}
		}
	}
}

#if 0
static void ba_init()
{
	u8 *ptr;

	/* shared ba */
	ptr = ci_shr_halloc(CI_BALLOC_SHR_SIZE, 0, "ba_shr");
	ci_balloc_init(&ci_node_info->ba_shr, "ba_shr", ptr, ci_node_info->ha_shr.range.end);
	ci_node_info->ba_shr.flag |= CI_BALLOC_LAZY_CONQUER;

	/* ba for json */
	ptr = ci_shr_halloc(CI_BALLOC_JSON_SIZE, 0, "ba_json"); 
	ci_balloc_init(&ci_node_info->ba_json, "ba_json", ptr, ptr + CI_BALLOC_JSON_SIZE);

	/* ba for node */
	ci_node_each(node, {
		ci_balloc_t *ba = &node->ba;
		
		ptr = ci_node_halloc(node->node_id, CI_BALLOC_NODE_SIZE, PAL_CPU_CACHE_LINE_SIZE, ci_ssf("ba.%d", node->node_id));	
		ci_balloc_init(ba, "ba_node", ptr, ptr + CI_BALLOC_NODE_SIZE);
		ba->node_id = node->node_id;
	});
}
#endif

int ci_mod_pre_init()
{
	ci_info.mod_info = &mod_info;
	ci_list_init(&ci_mod_info->mod_head);

	return 0;
}

static void ci_mod_ma_dump()
{
	ci_shr_halloc_dump();
	ci_node_each(node, {
		ci_halloc_dump(&node->ha);
	});

	ci_balloc_dump_brief(&ci_node_info->ba_shr);
	ci_balloc_dump_brief(&ci_node_info->ba_json);
	ci_node_each(node, {
		ci_balloc_dump_brief(&node->ba);
	});
	ci_node_each(node, {
		ci_balloc_dump_brief(&node->ba_paver);
	});
}

static void ci_mod_calc_max_len()
{
	ci_mod_max_len_t *max_len = &ci_mod_info->max_len;
	ci_jcmd_t *jcmd;
	
	ci_mod_each(m, {
		ci_max_set(max_len->mod_name, ci_strlen(m->name));
		ci_max_set(max_len->mod_desc, ci_strlen(m->desc));
		ci_sched_id_each(m, sid) {
			ci_max_set(max_len->sched_id_name, ci_strlen(sid->name));
			ci_max_set(max_len->sched_id_desc, ci_strlen(sid->desc));
		}

		for (jcmd = m->jcmd; jcmd && jcmd->name; jcmd++) {
			ci_max_set(max_len->jcmd_name, ci_strlen(jcmd->name));
			jcmd->desc && ci_max_set(max_len->jcmd_desc, ci_strlen(jcmd->desc));
			jcmd->usage && ci_max_set(max_len->jcmd_usage, ci_strlen(jcmd->usage)); 
		}
	});
}

int ci_mod_init()
{
	ci_mem_range_ex_t *mem;

	ci_slk_init(&ci_mod_info->lock);
//	ba_init();	/* create shr, json, and node ba */

	/* allocate range_vect_sid_name */
	mem = &ci_mod_info->range_vect_sid_name;
	mem->start = mem->curr = ci_shr_halloc(CI_MOD_SID_VECT_NAME_SIZE, 0, "range_vect_sid_name");
	mem->end = mem->start + CI_MOD_SID_VECT_NAME_SIZE;

	/* system memory allocation done, dump it */
	ci_mod_ma_dump();

	/* probing all modules, start the state machine */
	ci_imp_printfln("ci init modules ...");
	mod_probe_all();		

	ci_mod_calc_max_len();

	return 0;
}

int ci_mod_finz()
{
	ci_imp_printfln("ci finz modules ...");
	
	ci_mod_info->flag |= CI_MODF_SHUT_DOWN;		/* so we'll call shutdown after stop */
	mod_stop_all();

	return 0;
}

ci_mod_t *ci_mod_get(const char *name)
{
	ci_assert(name);
	ci_mod_each(m, {
		if (ci_strequal(m->name, name))
			return m;
	});
	
	return NULL;
}

int ci_mod_dump(ci_mod_t *mod)
{
#define __FMT		"%26s : "

	int count, rv = 0;
	ci_sched_id_t *sched_id;

	rv += ci_printfln(__FMT "%p", 		"ptr", 		mod);
	rv += ci_printfln(__FMT "\"%s\"", 	"name", 	mod->name);
	rv += ci_printfln(__FMT "\"%s\"", 	"desc", 	mod->desc);
	rv += ci_printfln(__FMT "%#X", 		"flag", 	mod->flag);
	rv += ci_printfln(__FMT "%s", 		"state", 	ci_int_to_name(ci_mods_name, mod->state));

	count = 0;
	rv += ci_printf(__FMT, 				"vect[]");
	ci_loop(i, CI_MODV_NR) 
		if (mod->vect[i]) {
			count++ && (rv += ci_printf(__FMT, ""));
			rv += ci_printfln(".%-18s = %p", ci_int_to_name(ci_modv_name, i), 	mod->vect[i]);
		}
	count || (rv += ci_printfln("NULL"));
		
#if 0
	count = 0;
	rv += ci_printf(__FMT, 				"jcmd[]");
	ci_jcmd_t *jcmd = mod->jcmd;
	while (jcmd && jcmd->name) {
		count++ && (rv += ci_printf(__FMT, ""));
		rv += ci_printfln("%-19s - \"%s\"", ci_ssf("\"%s\"", jcmd->name), jcmd->desc);
		jcmd++;
	}
	count || (rv += ci_printfln("NULL"));
#endif	
		
	rv += ci_printfln(__FMT "%d", 		"order_start", 	mod->order_start);
	rv += ci_printfln(__FMT "%d", 		"order_stop", 	mod->order_stop);

	rv += ci_printf(__FMT, 				"node_map");
	rv += ci_node_map_dump_exln(&mod->node_map, CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_COMMA);

	count = 0;
	rv += ci_printf(__FMT, 				"worker_map[]");
	ci_node_map_each_set(&mod->node_map, node_id) {
		ci_worker_map_t *worker_map = &mod->worker_map[node_id];

		count++ && (rv += ci_printf(__FMT, ""));
		rv += ci_printf("%d - ", node_id);
		rv += ci_worker_map_dump_exln(worker_map, CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_COMMA);
	}

	count = 0;
	rv += ci_printf(__FMT, 				"sched_id[]");
	sched_id = mod->sched_id;
	while (sched_id && sched_id->name) {
		int sub_count = 0;
		
		count++ && (rv += ci_printf(__FMT, ""));
		rv += ci_printf("%-19s - ", ci_ssf("\"%s\"", sched_id->name));
		rv += ci_dump_pre("prio:", 			ci_sched_prio_map_dump(&sched_id->prio_map));
		rv += ci_dump_pre(", node:", 		ci_node_map_dump(&sched_id->node_map));
		ci_node_map_each_set(&mod->node_map, node_id) {
			rv += !sub_count++ ? ci_printf(", ") : ci_print_str_repeat(71, " ");
			rv += ci_printf("worker[%d]:", node_id) + ci_worker_map_dumpln(&sched_id->worker_map[node_id]);
		}
		sched_id++;
	}
	count || (rv += ci_printfln("NULL"));

	return rv;
#undef __FMT
}

void ci_mod_node_worker_iterator(ci_mod_t *mod, void (*iterator)(ci_mod_t *, int, int))
{
	ci_node_map_each_set(&mod->node_map, node_id)
		ci_worker_map_each_set(&mod->worker_map[node_id], worker_id)
			iterator(mod, node_id, worker_id);
}

static void mod_vect_call(ci_sched_ctx_t *ctx)
{
	ci_mod_vect_task_t *task = ci_container_of(ctx->sched_ent, ci_mod_vect_task_t, sched_ent);
	ci_mod_t *mod = task->mod;
	ci_mod_vect_param_t	*param = &task->param;

	ci_assert(task->flag & CI_MOD_VECT_TASK_BUSY);
	task->flag &= ~CI_MOD_VECT_TASK_BUSY;

	ci_ntc_mode_flag_printfln('.', "mod", (CI_PRF_PREFIX | CI_PRF_NO_INDENT), 
							  CI_PR_INDENT "< + %s, \"%s\", node/worker=%d/%02d >", 
						      ci_int_to_name(ci_modv_name, param->vect_id), mod->name,
						      ctx->worker->node_id, ctx->worker->worker_id);
	mod->vect[param->vect_id](mod, task->json);	/* waiting for callback */
}

static ci_sched_grp_t *mod_round_robin_vect_sched_grp(ci_mod_t *mod) 	/* round robin */
{
	ci_sched_grp_t *grp;

	ci_assert(ci_mod_info->vect_node_id < ci_node_info->nr_node);
	ci_assert(ci_mod_info->vect_worker_id < ci_node_by_id(ci_mod_info->vect_node_id)->nr_worker);
	
	if (!(grp = mod->vect_sched_grp[ci_mod_info->vect_node_id][ci_mod_info->vect_worker_id])) {	/* now allowed to run for the node/worker */
		int first_node, first_worker;

		first_node = ci_node_map_first_set(&mod->node_map);
		ci_assert(first_node >= 0);
		first_worker = ci_worker_map_first_set(&mod->worker_map[first_node]);
		ci_assert(first_worker >= 0);

		grp = mod->vect_sched_grp[first_node][first_worker];
		ci_assert(grp);
		return grp;
	} 

	if (++ci_mod_info->vect_worker_id >= ci_node_by_id(ci_mod_info->vect_node_id)->nr_worker) {
		if (++ci_mod_info->vect_node_id >= ci_node_info->nr_node)
			ci_mod_info->vect_node_id = 0;
		ci_mod_info->vect_worker_id = 0;
	}
		
	return grp;
}

static int mod_vect_call_all(ci_mod_vect_param_t *param)
{
	int rv;

	ci_assert((param->st_exp < 0) || (ci_mod_info->state == param->st_exp));
	ci_assert((!param->get_order && param->all_done) || (param->get_order && !param->all_done));

	if (param->st_new >= 0) 	/* transfer to new state */
		ci_mod_info->state = param->st_new;

	ci_mod_info->countdown == 0;
	ci_range_check(param->vect_id, CI_MODV_PROBE, CI_MODV_NR);

	/* how many */
	ci_mod_each(m, {
		if (m->vect[param->vect_id] && (!param->get_order || (param->get_order(m) == param->order))) {
			m->state = param->st_mod_new;
			ci_mod_info->countdown++;		
		}
	});

	/* dump progress */
	rv = ci_mod_info->countdown;
	if (!param->get_order || rv)
		ci_ntc_mode_flag_printfln('+', "mod", (CI_PRF_PREFIX | CI_PRF_NO_INDENT),
								  "[ %-18s ], countdown=%d%s", 
							      ci_int_to_name(ci_mods_name, ci_mod_info->state), rv,
							   	  param->get_order ? ci_ssf(", order=%03d", param->order) : "");
	
	if (!rv) { 	/* nothing to do */
		if (!param->get_order) {
			ci_assert(param->all_done);
			param->all_done();
		}
		
		return 0;
	}

	/* invoke each module's vect[vect_id] */
	ci_printf_info->indent = 2;
	ci_mod_each(m, {
		if (m->vect[param->vect_id] && (!param->get_order || (param->get_order(m) == param->order))) {
			ci_json_t *json = ci_json_create("mod_vect_json");	/* ci_todo ... */

			if (!(param->flag & CI_MOD_VECT_PARALLEL)) {
				ci_ntc_mode_flag_printfln('.', "mod", (CI_PRF_PREFIX | CI_PRF_NO_INDENT),
										  CI_PR_INDENT "< + %s, \"%s\" >", ci_int_to_name(ci_modv_name, param->vect_id), m->name);
				m->vect[param->vect_id](m, json);	/* waiting for callback */
			} else {
				ci_mod_vect_task_t *task = &m->vect_task;
				ci_sched_grp_t *grp	= mod_round_robin_vect_sched_grp(m);

				ci_assert(!(task->flag & CI_MOD_VECT_TASK_BUSY));
				task->flag |= CI_MOD_VECT_TASK_BUSY;
				task->mod = m;
				task->json = json;
				task->sched_ent.exec = mod_vect_call;
				ci_obj_copy(&task->param, param);		/* stack allocated pram, we need store it */
				
				ci_sched_ext_add(grp, &task->sched_ent);				
			}
		}
	});	

	return rv;
}

static void mod_common_all_done()
{
	ci_printf_info->indent = 0;
	ci_mod_each(m, m->state = ci_mod_info->state);
	ci_ntc_mode_flag_printfln('-', "mod", (CI_PRF_PREFIX | CI_PRF_NO_INDENT), 
						      "[ %-18s ]", ci_int_to_name(ci_mods_name, ci_mod_info->state));
}

static void ci_mod_ind_done(ci_mod_t *mod, ci_json_t *json, int vect_id)
{
	ci_ntc_mode_flag_printfln('.', "mod", (CI_PRF_PREFIX | CI_PRF_NO_INDENT),
							  CI_PR_INDENT "< - %s, \"%s\" >", ci_int_to_name(ci_modv_name, vect_id), mod->name);
}

/*
 *	probe
 */
static void mod_probe_all()
{
	ci_mod_vect_param_t param = {
		.vect_id 		= CI_MODV_PROBE,
		.st_exp			= CI_MODS_NULL,
		.st_new			= CI_MODS_PROBE,
		.st_mod_new		= CI_MODS_PROBE,
		.all_done		= mod_probe_all_done
	};

	mod_vect_call_all(&param);
}

void ci_mod_probe_done(ci_mod_t *mod, ci_json_t *json)	/* client invoke this */
{
	int all_done = 0;

	ci_mod_ind_done(mod, json, CI_MODV_PROBE);
	ci_assert(json);
	ci_json_destroy(json);
	
	ci_assert(mod->state == CI_MODS_PROBE);
	mod->state = CI_MODS_PROBE_DONE;

	ci_slk_protected(&ci_mod_info->lock, {
		all_done = !(--ci_mod_info->countdown);
		ci_assert(ci_mod_info->countdown >= 0);
	});	

	if (all_done)
		mod_probe_all_done();
}

ci_mod_t *ci_mod_by_name(const char *name)
{
	ci_mod_each(m, {
		if (ci_strequal(m->name, name))
			return m;
	});

	return NULL;
}

static void mod_probe_all_done()
{
	ci_assert(ci_mod_info->state == CI_MODS_PROBE);
	ci_assert(!ci_mod_info->countdown);
	ci_mod_info->state = CI_MODS_PROBE_DONE;

	mod_common_all_done();

	ci_mod_each(m, {
		mod_set_dft(m);	/* set default values */
		mod_check(m);
	});

	ci_sched_init();
	ci_mod_each(m, mod_malloc(m));

	ci_timer_mod_init(ci_mod_by_name("$tmr"));	/* manually start timer module since other .init rely on it */
	mod_init_all();
}


/*
 *	init
 */
static void mod_init_all()
{
	ci_mod_vect_param_t param = {
		.flag			= CI_MOD_VECT_PARALLEL,
		.vect_id 		= CI_MODV_INIT,
		.st_exp			= CI_MODS_PROBE_DONE,
		.st_new			= CI_MODS_INIT,
		.st_mod_new		= CI_MODS_INIT,
		.all_done		= mod_init_all_done
	};

	mod_vect_call_all(&param);
}

void ci_mod_init_done(ci_mod_t *mod, ci_json_t *json)
{
	int all_done = 0;

	ci_mod_ind_done(mod, json, CI_MODV_INIT);
	ci_assert(json);
	ci_json_destroy(json);
	
	ci_assert(mod->state == CI_MODS_INIT);
	mod->state = CI_MODS_INIT_DONE;

	ci_slk_protected(&ci_mod_info->lock, {
		all_done = !(--ci_mod_info->countdown);
		ci_assert(ci_mod_info->countdown >= 0);
	});	

	if (all_done)
		mod_init_all_done();
}

static void mod_init_all_done()
{
	ci_assert(ci_mod_info->state == CI_MODS_INIT);
	ci_assert(!ci_mod_info->countdown);
	ci_mod_info->state = CI_MODS_INIT_DONE;

	mod_common_all_done();

	ci_mod_info->order = 0;
	ci_sched_bind_paver_mask();
	mod_start_all();
}


/*
 *	start
 */
static int mod_get_order_start(ci_mod_t *mod)
{
	return mod->order_start;
}

static void mod_start_all()
{
	ci_mod_info->order = CI_MOD_ORDER_MIN;
	mod_start_all_continue();
}

static void mod_start_all_continue()
{
	int order_start;

	ci_mod_vect_param_t param = {
		.flag			= CI_MOD_VECT_PARALLEL,
		.vect_id 		= CI_MODV_START,
		.st_mod_new		= CI_MODS_START,
		.get_order		= mod_get_order_start
	};

	if ((order_start = ci_mod_info->order) > CI_MOD_ORDER_MAX) 
		goto __all_done;

	ci_loop_i(order, order_start, CI_MOD_ORDER_MAX) {
		if (order != CI_MOD_ORDER_MIN)			/* do state transfer only one time */
			param.st_exp = param.st_new = -1;
		else {
			param.st_exp = ci_mod_info->flag & CI_MODF_READY ? CI_MODS_STOP_DONE : CI_MODS_INIT_DONE;
			param.st_new = CI_MODS_START;
		}
		
		param.order = order;
		if (mod_vect_call_all(&param) > 0)		/* waiting for callback */
			return;
		
		param.st_exp = param.st_new = -1;		/* no check & modify state */
		ci_mod_info->order++;					/* update to next order */
	}

__all_done:
	mod_start_all_done();
}

void ci_mod_start_done(ci_mod_t *mod, ci_json_t *json)
{
	int all_done = 0;

	ci_mod_ind_done(mod, json, CI_MODV_START);
	ci_assert(json);
	ci_json_destroy(json);
	
	ci_assert(mod->state == CI_MODS_START);
	mod->state = CI_MODS_START_DONE;

	ci_slk_protected(&ci_mod_info->lock, {
		all_done = !(--ci_mod_info->countdown);
		ci_assert(ci_mod_info->countdown >= 0);
	});	

	if (all_done) {
		ci_mod_info->order++;
		mod_start_all_continue();	
	}
}

static void mod_start_all_done()
{
	ci_assert(ci_mod_info->state == CI_MODS_START);
	ci_assert(!ci_mod_info->countdown);
	ci_mod_info->state = CI_MODS_START_DONE;

	mod_common_all_done();

	if (!(ci_mod_info->flag & CI_MODF_READY)) {		/* first time */
		ci_mod_info->flag |= CI_MODF_READY;

		/* 
		 *	since we do parallel init, now it is in the worker context
		 *	no need to use ci_printf_block_lock()
		 */
		__ci_printf_set_prefix_override("ci");
//		ci_mod_ma_dump();
		ci_imp_printfln("ci all modules started");
		
		ci_init_done();
		__ci_printf_clear_prefix_override();
	}
}


/*
 *	stop
 */
static int mod_get_order_stop(ci_mod_t *mod)
{
	return mod->order_stop;
}

static void mod_stop_all()
{
	ci_mod_info->order = CI_MOD_ORDER_MIN;
	mod_stop_all_continue();
}

static void mod_stop_all_continue()
{
	int order_stop;

	ci_mod_vect_param_t param = {
		.vect_id 		= CI_MODV_STOP,
		.st_mod_new		= CI_MODS_STOP,
		.get_order		= mod_get_order_stop
	};

	if ((order_stop = ci_mod_info->order) > CI_MOD_ORDER_MAX) 
		goto __all_done;

	ci_loop_i(order, order_stop, CI_MOD_ORDER_MAX) {
		if (order != CI_MOD_ORDER_MIN)			/* do state transfer only one time */
			param.st_exp = param.st_new = -1;
		else {
			param.st_exp = CI_MODS_START_DONE;
			param.st_new = CI_MODS_STOP;
		}
		
		param.order = order;
		if (mod_vect_call_all(&param) > 0)		/* waiting for callback */
			return;
		
		param.st_exp = param.st_new = -1;		/* no check & modify state */
		ci_mod_info->order++;					/* update to next order */
	}

__all_done:
	mod_stop_all_done();
}

void ci_mod_stop_done(ci_mod_t *mod, ci_json_t *json)
{
	int all_done = 0;

	ci_mod_ind_done(mod, json, CI_MODV_STOP);
	ci_assert(json);
	ci_json_destroy(json);
	
	ci_assert(mod->state == CI_MODS_STOP);
	mod->state = CI_MODS_STOP_DONE;

	ci_slk_protected(&ci_mod_info->lock, {
		all_done = !(--ci_mod_info->countdown);
		ci_assert(ci_mod_info->countdown >= 0);
	});	

	if (all_done) {
		ci_mod_info->order++;
		mod_stop_all_continue();	
	}
}

static void mod_stop_all_done()
{
	ci_assert(ci_mod_info->state == CI_MODS_STOP);
	ci_assert(!ci_mod_info->countdown);
	ci_mod_info->state = CI_MODS_STOP_DONE;

	mod_common_all_done();

	if ((ci_mod_info->flag & CI_MODF_SHUT_DOWN)) 
		mod_shutdown_all();
}


/*
 *	shutdown
 */
static void mod_shutdown_all()
{
	ci_mod_vect_param_t param = {
		.vect_id 		= CI_MODV_SHUTDOWN,
		.st_exp			= CI_MODS_STOP_DONE,
		.st_new			= CI_MODS_SHUTDOWN,
		.st_mod_new		= CI_MODS_SHUTDOWN,
		.all_done		= mod_shutdown_all_done
	};

	mod_vect_call_all(&param);
}

void ci_mod_shutdown_done(ci_mod_t *mod, ci_json_t *json)
{
	int all_done = 0;

	ci_mod_ind_done(mod, json, CI_MODV_SHUTDOWN);
	ci_assert(json);
	ci_json_destroy(json);
	
	ci_assert(mod->state == CI_MODS_SHUTDOWN);
	mod->state = CI_MODS_SHUTDOWN_DONE;

	ci_slk_protected(&ci_mod_info->lock, {
		all_done = !(--ci_mod_info->countdown);
		ci_assert(ci_mod_info->countdown >= 0);
	});	

	if (all_done)
		mod_shutdown_all_done();
}

static void mod_shutdown_all_done()
{
	ci_assert(ci_mod_info->state == CI_MODS_SHUTDOWN);
	ci_assert(!ci_mod_info->countdown);
	ci_mod_info->state = CI_MODS_SHUTDOWN_DONE;

	mod_common_all_done();
	ci_finz_done();
}



/* ci_todo .... */
void ci_mod_bist_done(ci_mod_t *mod, ci_json_t *json)
{
}


