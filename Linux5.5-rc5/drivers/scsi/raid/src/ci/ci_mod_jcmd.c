/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_mod_jcmd.c				CI Module for CLI
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

static int ci_jcmd_mod(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_ba(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_ha(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_node(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_paver(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_sched_dpt(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_worker_data(ci_mod_t *mod, ci_json_t *json);
static int ci_jcmd_worker(ci_mod_t *mod, ci_json_t *json);

#ifdef CI_WORKER_STA
static int ci_jcmd_cpu(ci_mod_t *mod, ci_json_t *json);
#endif


static ci_jcmd_t ci_mod_jcmd[] = {
	{ "ci.ba",			ci_jcmd_ba,				"buddy allocator",				"[--shared] [--json] [--paver] [--id <node_id>]  [--verbose] [--pending] [--free]"},

#ifdef CI_WORKER_STA
	{ "ci.cpu",			ci_jcmd_cpu,			"cpu usage"},
#endif
		
	{ "ci.ha",			ci_jcmd_ha,				"heap allocator",				"[--shared] [--id <node_id>]" },
	{ "ci.mod", 		ci_jcmd_mod, 			"modules' info",				"[--module <module>] [--verbose [--paver_id <paver_id>]]" },
	{ "ci.node",		ci_jcmd_node,			"node info" },
	{ "ci.paver",		ci_jcmd_paver,			"paver info",					"[--id <node_id>] [--verbose]" },					
	{ "ci.sd",			ci_jcmd_sched_dpt,		"schedule descriptor table", 	"[--all] [--help]" },
	{ "ci.wd",			ci_jcmd_worker_data,	"per-worker data table",		"[--id] [--help]" },
	{ "ci.worker", 		ci_jcmd_worker,			"worker info" },
	CI_EOT
};

static int ci_jcmd_mod_name_compare(void *a, void *b)
{
	return ci_strcmp(((ci_mod_t *)a)->name, ((ci_mod_t *)b)->name);
}

void ci_jcmd_get_mod_ary(ci_mod_t ***ary, int *count)
{
	ci_mod_t **p;
	
	*count = ci_list_count(&ci_mod_info->mod_head);
	p = *ary = ci_shr_balloc(ci_sizeof(ci_mod_t *) * *count);

	ci_mod_each(m, { *p++ = m; });
	ci_quick_sort(*ary, 0, *count - 1, ci_jcmd_mod_name_compare);
}

void ci_jcmd_put_mod_ary(ci_mod_t ***ary)
{
	ci_assert(ary);
	ci_shr_bfree(*ary);
	*ary = NULL;
}

static int ci_jcmd_cmd_compare(void *a, void *b)
{
	return ci_strcmp(((ci_jcmd_t *)a)->name, ((ci_jcmd_t *)b)->name);
}

void ci_jcmd_get_mod_jcmd_ary(ci_mod_t *mod, ci_jcmd_t ***ary, int *count)
{
	ci_jcmd_t *p, **q;
	
	*count = 0;
	*ary = NULL;
	
	for (p = mod->jcmd; p && p->name; (*count)++, p++)
		;
	if (!*count)
		return;

	q = *ary = ci_shr_balloc(ci_sizeof(ci_jcmd_t *) * *count);
	for (p = mod->jcmd; p->name; *q++ = p++)
		;

	ci_quick_sort(*ary, 0, *count - 1, ci_jcmd_cmd_compare);
}

void ci_jcmd_put_mod_jcmd_ary(ci_jcmd_t ***ary)
{
	ci_assert(ary);
	ci_shr_bfree(*ary);
	*ary = NULL;
}

void ci_jcmd_get_all_mod_jcmd_ary(ci_jcmd_t ***ary, int *count)
{
	ci_jcmd_t *p, **q;
	
	*count = 0;
	*ary = NULL;

	ci_mod_each(mod, {
		for (p = mod->jcmd; p && p->name; (*count)++, p++)
			;
	});

	ci_assert(*count);	
	q = *ary = ci_shr_balloc(ci_sizeof(ci_jcmd_t *) * *count);
	ci_mod_each(mod, {
		for (p = mod->jcmd; p && p->name; *q++ = p++)
			;
	});
	ci_quick_sort(*ary, 0, *count - 1, ci_jcmd_cmd_compare);
}

void ci_jcmd_put_all_mod_jcmd_ary(ci_jcmd_t ***ary)
{
	ci_jcmd_put_mod_jcmd_ary(ary);
}

static int __dump_all_modules()
{
	int rv = 0, cnt = 0;
	ci_mod_max_len_t *max_len = &ci_mod_info->max_len;
	int size[] = { max_len->mod_name, max_len->mod_desc };
	
	rv += ci_box_top(size, "NAME", "DESCRIPTION");
	ci_mod_each(m, rv += ci_box_body(!cnt++, size, m->name, m->desc));
	rv += ci_box_btm(size);
	return rv;
}

static int ci_jcmd_mod(ci_mod_t *mod, ci_json_t *json)
{
	int rv, nr_mod;
	ci_mod_t **ary;

	struct {
		struct {	/* all flags are u8 */
			u8		 verbose;
			u8		 module;
			u8		 help;
		} flag;

		char 		*module;		
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_nil_arg(&opt, verbose, 	'v',	"verbose mode"),
		ci_jcmd_opt_optional_has_arg(&opt, module, 		'm',	"dump the target module"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 		'h', 	"show help"),
		CI_EOT
	};	
	
	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);	

	ci_jcmd_get_mod_ary(&ary, &nr_mod);

	opt.flag.module && (opt.flag.verbose = 1);		/* if --module, then default --verbose */
	
	if (!opt.flag.verbose) 
		__dump_all_modules();
	else {
		int count = 0;
		
		ci_loop(i, nr_mod) {
			ci_mod_t *m = ci_mod_get(ary[i]->name);

			if (!m || (opt.flag.module && opt.module && !ci_strequal(m->name, opt.module)))
				continue;
			
			ci_assert(m);
			count++;
			ci_mod_dump(m);
			if ((i != nr_mod - 1) && !opt.flag.module)
				ci_printf("\n\n");
		}

		if (!count && opt.flag.module)
			ci_printfln("! module \"%s\" cannot be found.", opt.module);
	} 

	ci_jcmd_put_mod_ary(&ary);
	
	return 0;
}


/*
 *	ba dump command
 */
typedef struct {
	struct {	/* all flags are u8 */
		u8		json;
		u8		shared;
		u8 		paver;
		u8		node;
		
		u8		free;
		u8		verbose;
		u8		pending;
		
		u8		help;
		
		u8		id;
	} flag;

	int			id;
} ci_jcmd_ba_opt_t;

static int ci_jcmd_ba_dump(ci_mod_t *mod, ci_jcmd_ba_opt_t *opt, ci_balloc_t *ba)
{
	if (opt->flag.free)
		ci_balloc_free_cache(ba);

	if (opt->flag.verbose)
		ci_balloc_dump(ba);
	else if (!opt->flag.pending)
		ci_balloc_dump_brief(ba);
	
	if (opt->flag.pending) {
		if (opt->flag.verbose)
			ci_printfln();
		ci_balloc_dump_pending(ba);
	}

	return 0;
}

static int ci_jcmd_ba(ci_mod_t *mod, ci_json_t *json)
{
	int rv;

	ci_jcmd_ba_opt_t opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_nil_arg(&opt, shared,	's',	"shared buddy system allocator"),
		ci_jcmd_opt_optional_nil_arg(&opt, json,	'j',	"json buddy system allocator"),
		ci_jcmd_opt_optional_nil_arg(&opt, paver,	'p',	"paver buddy system allocator"),
		ci_jcmd_opt_optional_nil_arg(&opt, node,	'n',	"node buddy system allocator"),

		ci_jcmd_opt_optional_nil_arg(&opt, verbose, 'v',	"verbose mode"),
		ci_jcmd_opt_optional_nil_arg(&opt, pending, 'd',	"dump pending allocations"),
		ci_jcmd_opt_optional_nil_arg(&opt, free, 	'f',	"free cache"),
		
		ci_jcmd_opt_optional_has_arg(&opt, id,		'i',	"node id"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};	
	
	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);

	if (!opt.flag.shared && !opt.flag.paver && !opt.flag.json && !opt.flag.node && !opt.flag.id) {
		ci_jcmd_ba_dump(mod, &opt, &ci_node_info->ba_shr);
		ci_printfln();
		ci_jcmd_ba_dump(mod, &opt, &ci_node_info->ba_json);
		
		ci_node_each(node, {
			ci_printfln();
			ci_jcmd_ba_dump(mod, &opt, &node->ba_paver);
		});

		ci_node_each(node, {
			ci_printfln();
			ci_jcmd_ba_dump(mod, &opt, &node->ba);
		});
		return 0;	
	}

	opt.flag.shared && ci_jcmd_ba_dump(mod, &opt, &ci_node_info->ba_shr);
	opt.flag.json && ci_jcmd_ba_dump(mod, &opt, &ci_node_info->ba_json);

	if (!opt.flag.node && !opt.flag.paver && !opt.flag.id)
		return 0;

	(!opt.flag.paver && !opt.flag.node) && (opt.flag.paver = opt.flag.node = 1);
	
	if (opt.flag.id) {
		if (!ci_in_range(opt.id, 0, ci_node_info->nr_node)) {
			ci_printf("node id must in the range of [ %d, %d )\n", 0, ci_node_info->nr_node);
			return -CI_E_RANGE;
		}

		opt.flag.paver && ci_jcmd_ba_dump(mod, &opt, &ci_node_by_id(opt.id)->ba_paver);
		opt.flag.node && ci_jcmd_ba_dump(mod, &opt, &ci_node_by_id(opt.id)->ba);
		
		return 0;
	}

	if (opt.flag.paver) 
		ci_node_each(node, {
			node->node_id && ci_printfln();
			ci_jcmd_ba_dump(mod, &opt, &node->ba_paver);
		});

	if (opt.flag.node) 
		ci_node_each(node, {
			node->node_id && ci_printfln();
			ci_jcmd_ba_dump(mod, &opt, &node->ba);
		});

	return 0;
}

static int ci_jcmd_ha(ci_mod_t *mod, ci_json_t *json)
{
	int rv;
	
	struct {
		struct {	/* all flags are u8 */
			u8		id;
			u8		shared;
			u8		help;
		} flag;

		int			id;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_has_arg(&opt, id,		'i',	"node id"),
		ci_jcmd_opt_optional_nil_arg(&opt, shared,	's',	"shared ha"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};	
	
	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);

	if (ci_jcmd_no_opt(json)) {
		ci_shr_halloc_dump();
		ci_loop(node_id, ci_node_info->nr_node) {
			ci_printfln();
			ci_node_halloc_dump(node_id);
		}
		return 0;	
	}

	if (opt.flag.shared) {
		ci_shr_halloc_dump();
		if (!opt.flag.id)
			return 0;
	}

	if (!ci_in_range(opt.id, 0, ci_node_info->nr_node)) {
		ci_printf("node id must in the range of [ %d, %d )\n", 0, ci_node_info->nr_node);
		return -CI_E_RANGE;
	}

	ci_node_halloc_dump(opt.id);
	return 0;
}

static int ci_jcmd_node(ci_mod_t *mod, ci_json_t *json)
{
	ci_jcmd_require_no_opt(json);
	ci_node_info_dump();
	return 0;
}

static int ci_jcmd_sched_dpt(ci_mod_t *mod, ci_json_t *json)
{
	int rv;
	ci_mod_t *m;
	ci_sched_id_t *sid;
	ci_worker_map_t dummy_map;
	ci_mod_max_len_t *max_len = &ci_mod_info->max_len;
	int worker_map_len, nr_col = 6 + ci_node_info->nr_node, cnt = 0;
	char worker_buf[CI_NODE_NR][14], prio_map_buf[16], node_map_buf[16], worker_map_buf[CI_NODE_NR][64];

	/* param parse */
	struct {
		struct {	/* all flags are u8 */
			u8		all;
			u8		help;
		} flag;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_nil_arg(&opt, all,		'a',	"show all"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};

	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);


	/* eval the map length */
	ci_worker_map_sn_dump(worker_map_buf[0], ci_sizeof(worker_map_buf[0]), &dummy_map);
	worker_map_len = ci_strlen(worker_map_buf[0]);

	const char *name[6 + CI_NODE_NR] = {	/* set col name */
		"ID",				/* 0 */
		"NAME",				/* 1 */
		"DESCRIPTION",		/* 2 */
		"MODULE",			/* 3 */
		"PRIO_MAP",			/* 4 */
		"NODE_MAP"			/* 5 */
	};
	
	int size[6 + CI_NODE_NR] = { 
		ci_max(2, ci_nr_digit(CI_SCHED_DPT_NR)), 	/* sd */
		max_len->sched_id_name, 
		max_len->sched_id_desc,
		ci_max(6, max_len->mod_name),
		8,								/* prio_map */
		8,								/* node_map */
	};		/* sd, name, desc, mod, prio, node_map, worker_map[] */

	ci_loop(i, ci_node_info->nr_node) {	/* set worker_map name & size */
		ci_snprintf(worker_buf[i], ci_sizeof(worker_buf[i]), "WORKER_MAP.%02d", i);
		name[6 + i] = worker_buf[i];
		size[6 + i] = worker_map_len;
	}


	/* draw top */
	ci_box_top_ary(size, name, nr_col);	

	/* now modify the pointer to row buffer */
	name[4] = prio_map_buf;	
	name[5] = node_map_buf;
	ci_loop(i, ci_node_info->nr_node) 
		name[6 + i] = worker_map_buf[i];
	size[0] |= CI_PR_BOX_ALIGN_CENTER;
	size[4] |= CI_PR_BOX_ALIGN_CENTER;
	size[5] |= CI_PR_BOX_ALIGN_CENTER;

	/* draw body */
	ci_loop(sd, 1, CI_SCHED_DPT_NR) {
		if (ci_sched_dpt_by_dpt(sd, &m, &sid) < 0)
			continue;
		if ((sid->flag & CI_SIDF_SYSTEM) && !opt.flag.all)
			continue;

		name[0] = ci_ssf("%d", sd);
		name[1] = sid->name;
		name[2] = sid->desc;
		name[3] = m->name;

		ci_sched_prio_map_sn_dump(prio_map_buf, ci_sizeof(prio_map_buf), &sid->prio_map);
		ci_node_map_sn_dump(node_map_buf, ci_sizeof(node_map_buf), &sid->node_map);
		ci_loop(i, ci_node_info->nr_node) 
			ci_worker_map_sn_dump(worker_map_buf[i], ci_sizeof(worker_map_buf[i]), &sid->worker_map[i]);
		
		ci_box_body_ary(!cnt++, size, name, nr_col);
	}

	/* draw bottom */
	ci_box_btm_ary(size, nr_col);

	return 0;
}

static int ci_jcmd_worker_data_table(ci_mod_t *mod, ci_json_t *json)
{
	const char *data_name;
	ci_worker_map_t dummy_map;
	int worker_map_len, nr_col = 3 + ci_node_info->nr_node, cnt = 0, max_data_name_len = 0;
	char worker_buf[CI_NODE_NR][14], node_map_buf[16], worker_map_buf[CI_NODE_NR][64];

	/* figure out the max len of name */
	ci_loop(i, 1, CI_WORKER_DATA_NR) {
		if ((data_name = ci_node_info->worker_data_name[i]))
			ci_max_set(max_data_name_len, ci_strlen(data_name));
	}

	const char *name[3 + CI_NODE_NR] = {	/* set col name */
		"ID",				/* 0 */
		"NAME",				/* 1 */
		"NODE_MAP"			/* 2 */
	};	

	int size[3 + CI_NODE_NR] = { 
		ci_max(2, ci_nr_digit(CI_WORKER_DATA_NR)), 	/* wdid */
		max_data_name_len, 							/* worker data name */
		8,											/* node_map */
	};		

	/* eval the map length */
	ci_worker_map_sn_dump(worker_map_buf[0], ci_sizeof(worker_map_buf[0]), &dummy_map);
	worker_map_len = ci_strlen(worker_map_buf[0]);
	
	ci_loop(i, ci_node_info->nr_node) {	/* set worker_map name & size */
		ci_snprintf(worker_buf[i], ci_sizeof(worker_buf[i]), "WORKER_MAP.%02d", i);
		name[3 + i] = worker_buf[i];
		size[3 + i] = worker_map_len;
	}

	/* draw top */
	ci_box_top_ary(size, name, nr_col);	

	/* body pointers */
	name[2] = node_map_buf;
	ci_loop(i, ci_node_info->nr_node) 
		name[3 + i] = worker_map_buf[i];
	size[2] |= CI_PR_BOX_ALIGN_CENTER;	

	/* draw body */
	ci_loop(i, 1, CI_WORKER_DATA_NR) {
		ci_node_map_t node_map;
		ci_worker_map_t worker_map;
	
		if (!(data_name = ci_node_info->worker_data_name[i]))
			continue;

		/* iterate the sd for all workers */
		ci_node_map_zero(&node_map);
		ci_worker_map_zero(&worker_map);
		ci_node_worker_each(node, worker, {
			if (worker->data[i]) {
				ci_node_map_set_bit(&node_map, worker->node_id);
				ci_worker_map_set_bit(&worker_map, worker->worker_id);
			}
		});

		name[0] = ci_ssf("%d", i);
		name[1] = data_name;

		ci_node_map_sn_dump(node_map_buf, ci_sizeof(node_map_buf), &node_map);
		ci_loop(i, ci_node_info->nr_node) 
			ci_worker_map_sn_dump(worker_map_buf[i], ci_sizeof(worker_map_buf[i]), &worker_map);

		ci_box_body_ary(!cnt++, size, name, nr_col);
	}

	/* draw bottom */
	ci_box_btm_ary(size, nr_col);
	
	return 0;
}

static int ci_jcmd_worker_data(ci_mod_t *mod, ci_json_t *json)
{
#define __FMT		"%12s : "

	int rv;

	struct {
		struct {	/* all flags are u8 */
			u8		id;
			u8		help;
		} flag;

		int			id;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_has_arg(&opt, id,		'i',	"worker data id"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};	
	
	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);

	if (ci_jcmd_no_opt(json)) 
		return ci_jcmd_worker_data_table(mod, json);

	/* dump detail for the wdid */
	if ((opt.id < 1) || (opt.id >= CI_WORKER_DATA_NR)) {
		ci_printfln("invalid id=%d, must in the range of [ 1, %d ).", opt.id, CI_WORKER_DATA_NR);
		return 0;
	}

	if (!ci_node_info->worker_data_name[opt.id]) {
		ci_printfln("cannot find any worker data associates with id=%d\n", opt.id);
		return 0;
	}
	
	ci_printfln(__FMT "%d", 		"id", 			opt.id);
	ci_printfln(__FMT "%s", 		"name", 		ci_node_info->worker_data_name[opt.id]);

	ci_node_each(node, {
		int cnt = 0;
		ci_worker_each(node, worker, {
			if (!cnt++)
				ci_printfln(__FMT "%d", "node", worker->node_id);
			ci_printfln(__FMT "< worker/data = %02d/%p >", "", worker->worker_id, worker->data[opt.id]);
		});
	});

	return 0;
#undef __FMT	
}

static int ci_jcmd_worker(ci_mod_t *mod, ci_json_t *json)
{
	enum {
		JCMD_WORKER_TABLE_NODE_WORKER,
		JCMD_WORKER_TABLE_NUMA_CPU,
		JCMD_WORKER_TABLE_NR_BUSY,
		JCMD_WORKER_TABLE_LVL_BUSY,
		JCMD_WORKER_TABLE_STACK_NUMA_BINDING,
		JCMD_WORKER_TABLE_STACK_USAGE,
#ifdef CI_WORKER_STA					 
		JCMD_WORKER_TABLE_WORKER_STA,
#endif			
		JCMD_WORKER_TABLE_NR_COL
	};

	int title_size[JCMD_WORKER_TABLE_NR_COL];
	int size[JCMD_WORKER_TABLE_NR_COL] = {
		[JCMD_WORKER_TABLE_NODE_WORKER]			= 11 | CI_PR_BOX_ALIGN_CENTER,			/* node/worker */
		[JCMD_WORKER_TABLE_NUMA_CPU]			=  8 | CI_PR_BOX_ALIGN_CENTER,			/* numa/cpu */	
		[JCMD_WORKER_TABLE_NR_BUSY] 			=  9 | CI_PR_BOX_ALIGN_CENTER,			/* sched_tab's nr_busy */	
		[JCMD_WORKER_TABLE_LVL_BUSY] 			=  8 | CI_PR_BOX_ALIGN_CENTER,			/* sched_tab's lvl_busy */	
		[JCMD_WORKER_TABLE_STACK_NUMA_BINDING]	=  5 | CI_PR_BOX_ALIGN_CENTER,			/* stack numa binding */
		[JCMD_WORKER_TABLE_STACK_USAGE]			=  0,									/* stack usage */
#ifdef CI_WORKER_STA					 
		[JCMD_WORKER_TABLE_WORKER_STA]			=  8 | CI_PR_BOX_ALIGN_CENTER			/* CPU usage */
#endif					 
	};


	ci_jcmd_require_no_opt(json);
	ci_node_worker_each(node, worker, {		/* measure size */
		int stack_usage = pal_worker_stack_usage(worker->pal_worker);
		char *p = ci_ssf(CI_PR_BNP_FMT ", " CI_PR_PCT_FMT, ci_pr_bnp_val(stack_usage), ci_pr_pct_val(stack_usage, PAL_WORKER_STACK_SIZE));
		ci_max_set(size[JCMD_WORKER_TABLE_STACK_USAGE], ci_strlen(p));
	});
	size[JCMD_WORKER_TABLE_STACK_USAGE] |= CI_PR_BOX_ALIGN_CENTER;

	memcpy(title_size, size, ci_sizeof(size));
#ifdef CI_WORKER_STA
	size[JCMD_WORKER_TABLE_WORKER_STA] &= ~CI_PR_BOX_ALIGN_CENTER;
	size[JCMD_WORKER_TABLE_WORKER_STA] |= CI_PR_BOX_ALIGN_RIGHT;
#endif	

	ci_node_each(node, {
		int cnt = 0;
		node->node_id && ci_printfln();
		
		ci_box_top(title_size, "NODE/WORKER", "NUMA/CPU", "NR_BUSY", "LVL_BUSY", "STACK", "STACK USAGE"
#ifdef CI_WORKER_STA
				   , "CPU%"
#endif
			);

		ci_worker_each(node, worker, {
			pal_worker_t *pw = worker->pal_worker;
			ci_sched_tab_t *tab = &worker->sched_tab;
			int stack_usage = pal_worker_stack_usage(pw);
			int bind = pal_numa_id_by_ptr(pw->stack);

#ifdef CI_WORKER_STA
			char *cpu_usage;
			u64 busy_cycle;

			if (ci_sta_last_hist_val(worker->sta, "1_SEC", 0, &busy_cycle) < 0)
				cpu_usage = "n/a";
			else {
				ci_min_set(busy_cycle, PAL_CYCLE_PER_SEC);		/* PAL_CYCLE_OVERHEAD might cause 1~2% offset */
				cpu_usage = ci_ssf(CI_PR_PCT_FMT, ci_pr_pct_val(busy_cycle, PAL_CYCLE_PER_SEC));
			}
#endif
			
			ci_box_body(!cnt++, size, 
						ci_ssf("%d/%02d", node->node_id, worker->worker_id), 
						ci_ssf("%d/%02d", pw->numa_id, pw->cpu_id),
						ci_ssf("%i", tab->nr_busy),
						ci_ssf("%d", tab->lvl_busy),
						ci_ssf("%s%d", bind == pw->numa_id ? "" : "*", bind),
						ci_ssf(CI_PR_BNP_FMT ", " CI_PR_PCT_FMT, ci_pr_bnp_val(stack_usage), ci_pr_pct_val(stack_usage, PAL_WORKER_STACK_SIZE))
#ifdef CI_WORKER_STA
				   		, cpu_usage
#endif
					   );
		});
		ci_box_btm(size);
	});

	return 0;
}

static int ci_jcmd_paver_table(ci_mod_t *mod, ci_json_t *json, int node_id)
{
	enum {
		JCMD_PAVER_TABLE_ID,
		JCMD_PAVER_TABLE_NAME,
		JCMD_PAVER_TABLE_WORKER_MAP,
		JCMD_PAVER_TABLE_BUCKET_SIZE,
		JCMD_PAVER_TABLE_OBJ_SIZE,
		JCMD_PAVER_TABLE_OBJ_PER_BUCKET,
		JCMD_PAVER_TABLE_OBJ_BUSY,
		JCMD_PAVER_TABLE_OBJ_TOTAL,
		JCMD_PAVER_TABLE_BUSY_PERCENT,
		JCMD_PAVER_TABLE_MEMORY,
		JCMD_PAVER_TABLE_NR_COL
	};

	ci_paver_pool_t *pool;
	ci_worker_map_t dummy_map;
	const char *body[JCMD_PAVER_TABLE_NR_COL];
	char worker_map_buf[64];
	int row_cnt, node_cnt, head_size[JCMD_PAVER_TABLE_NR_COL], body_size[JCMD_PAVER_TABLE_NR_COL];
	
	const char *name[JCMD_PAVER_TABLE_NR_COL] = {
		[JCMD_PAVER_TABLE_ID] 				= "ID",
		[JCMD_PAVER_TABLE_NAME] 			= "NAME",
		[JCMD_PAVER_TABLE_WORKER_MAP] 		= "WORKER_MAP",
		[JCMD_PAVER_TABLE_BUCKET_SIZE] 		= "BKT_SZ",
		[JCMD_PAVER_TABLE_OBJ_SIZE] 		= "OBJ_SZ",
		[JCMD_PAVER_TABLE_OBJ_PER_BUCKET] 	= "OBJ/BKT",
		[JCMD_PAVER_TABLE_OBJ_BUSY] 		= "OBJ_BUSY",
		[JCMD_PAVER_TABLE_OBJ_TOTAL] 		= "OBJ_TTL",
		[JCMD_PAVER_TABLE_BUSY_PERCENT] 	= "BUSY/TTL",
		[JCMD_PAVER_TABLE_MEMORY] 			= "MEMORY_TTL"
	};

	ci_worker_map_sn_dump(worker_map_buf, ci_sizeof(worker_map_buf), &dummy_map);		/* eval the map length */

	ci_loop(i, JCMD_PAVER_TABLE_NR_COL)
		head_size[i] = ci_strlen(name[i]);

	head_size[JCMD_PAVER_TABLE_BUCKET_SIZE]++;
	head_size[JCMD_PAVER_TABLE_WORKER_MAP] += 3;	/* append .00 (node_id) */
	head_size[JCMD_PAVER_TABLE_MEMORY] = 25;		/* big enough? */
	
	ci_max_set(head_size[JCMD_PAVER_TABLE_WORKER_MAP], ci_strlen(worker_map_buf));
	ci_paver_node_info_pool_each(pni, p, {
		ci_max_set(head_size[1], ci_strlen(p->name));
	});

	ci_memcpy(body_size, head_size, ci_sizeof(head_size));
	head_size[JCMD_PAVER_TABLE_MEMORY] |= CI_PR_BOX_ALIGN_CENTER;
	
	body_size[JCMD_PAVER_TABLE_BUCKET_SIZE] 	|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_OBJ_SIZE] 		|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_OBJ_PER_BUCKET] 	|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_OBJ_BUSY] 		|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_OBJ_TOTAL] 		|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_BUSY_PERCENT] 	|= CI_PR_BOX_ALIGN_RIGHT;	
	body_size[JCMD_PAVER_TABLE_MEMORY] 			|= CI_PR_BOX_ALIGN_RIGHT;

	node_cnt = 0;
	ci_paver_node_info_each(pni, {
		if ((node_id >= 0) && (node_id != pni->node_id))
			continue;
		
		row_cnt = 0;
		node_cnt++ && ci_printfln();

		name[JCMD_PAVER_TABLE_WORKER_MAP] = ci_ssf("%s.%02d", "WORKER_MAP", pni->node_id);
		ci_box_top_ary(head_size, name, JCMD_PAVER_TABLE_NR_COL);	
		
		ci_list_each(&pni->pool_head, pool, link) {
			ci_paver_t *paver;
			int obj_total, obj_free, mem_total;

			obj_total = obj_free = 0;
			mem_total = pool->bucket_nr * pool->bucket_size;	/* free buckets */
			
			ci_list_each(&pool->paver_head, paver, link) {
				obj_total += paver->nr_total;
				obj_free += paver->nr_free;
				mem_total += pool->bucket_size * (paver->nr_total / pool->bucket_obj_nr);
			}
			
			ci_worker_map_sn_dump(worker_map_buf, ci_sizeof(worker_map_buf), &pool->worker_map);
			
			body[JCMD_PAVER_TABLE_ID] 				= ci_ssf("%d", pool->paver_id);
			body[JCMD_PAVER_TABLE_NAME] 			= pool->name;
			body[JCMD_PAVER_TABLE_WORKER_MAP] 		= worker_map_buf;
			body[JCMD_PAVER_TABLE_BUCKET_SIZE]		= ci_ssf("%i", pool->bucket_size);
			body[JCMD_PAVER_TABLE_OBJ_SIZE] 		= ci_ssf("%i", pool->obj_size_alloc);
			body[JCMD_PAVER_TABLE_OBJ_PER_BUCKET] 	= ci_ssf("%i", pool->bucket_obj_nr);
			body[JCMD_PAVER_TABLE_OBJ_BUSY] 		= ci_ssf("%i", obj_total - obj_free);
			body[JCMD_PAVER_TABLE_OBJ_TOTAL] 		= ci_ssf("%i", obj_total);
			body[JCMD_PAVER_TABLE_BUSY_PERCENT] 	= ci_ssf(CI_PR_PCT_FMT, ci_pr_pct_val(obj_total - obj_free, obj_total));
			body[JCMD_PAVER_TABLE_MEMORY] 			= ci_ssf(CI_PR_BNP_FMT, ci_pr_bnp_val(mem_total));

			ci_box_body_ary(!row_cnt++, body_size, body, JCMD_PAVER_TABLE_NR_COL);
		}
		
		ci_box_btm_ary(head_size, JCMD_PAVER_TABLE_NR_COL);
	});
	
	return 0;
}

static int ci_jcmd_paver(ci_mod_t *mod, ci_json_t *json)
{
	int rv, cnt = 0;
	ci_paver_pool_t *pool;
	
	struct {
		struct {	/* all flags are u8 */
			u8		id;
			u8		verbose;
			u8		help;
			u8		paver_id;
		} flag;

		int			id;
		int			paver_id;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_has_arg(&opt, id,			'i',	"node id"),
		ci_jcmd_opt_optional_nil_arg(&opt, verbose,		'v',	"verbose mode"),
		ci_jcmd_opt_optional_has_arg(&opt, paver_id,	'p',	"paver id"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 		'h', 	"show help"),
		CI_EOT
	};	

	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;	
	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);
	
	if (!opt.flag.verbose) {
		int node_id = -1;
		
		(opt.flag.id) && (node_id = opt.id);
		ci_jcmd_paver_table(mod, json, node_id);
		
		return 0;
	}

	if (opt.flag.id && !ci_in_range(opt.id, 0, ci_node_info->nr_node)) {
		ci_printf("node id must in the range of [ %d, %d )\n", 0, ci_node_info->nr_node);
		return -CI_E_RANGE;
	}

	ci_paver_node_info_each(pni, {
		if (opt.flag.id && (opt.id != pni->node_id))
			continue;

		ci_list_each(&pni->pool_head, pool, link) {
			if (opt.flag.paver_id && (opt.paver_id != pool->paver_id))
				continue;
			
			if (cnt++) {
				ci_print_hline(120);
				ci_printf("\n\n");
			}

			ci_printf("[ NODE_ID = %02d, PAVER_ID = %d ]\n\n", pni->node_id, pool->paver_id);
			
			ci_paver_pool_dump_brief(pool);
			ci_printfln();
			ci_printf_info->indent++;
			ci_paver_pool_dump_rti(pool, 1);
			ci_printf_info->indent--;
			ci_printfln();

		}
	});

	return 0;
}

#ifdef CI_WORKER_STA
static int ci_jcmd_cpu(ci_mod_t *mod, ci_json_t *json)
{
#define __COL			(28 + 1)		/* 1 for head, rest for cpu */
#define __BSIZE			6
	char *head[__COL];
	char *body[__COL];
	char head_name[__COL][__BSIZE];
	char body_name[__COL][__BSIZE];
	int size[__COL], max_col = 0, row = 0;

#if 0
	/* decide the max_col */
	ci_node_each(node, ci_max_set(max_col, node->nr_worker + 1));
	if (max_col > __COL)
		max_col = (max_col + 1) / 2;
	ci_min_set(max_col, __COL);
#endif

	/* formula for my linux sim, windows sim, and axnp-sim */
	max_col = ci_node_info->node[0]->nr_worker + 1;
	if ((max_col == ci_node_info->node[ci_node_info->nr_node - 1]->nr_worker + 1))
		max_col = ci_node_info->node[0]->nr_worker / 2 + 1;
	ci_min_set(max_col, __COL);

	/* init head_name */
	ci_loop(i, __COL) {
		ci_snprintf(head_name[i], __BSIZE, "+%d", i - 1);
		head[i] = head_name[i];
		body[i] = body_name[i];
		size[i] = 3 | CI_PR_BOX_ALIGN_RIGHT;
	}
	ci_snprintf(head_name[0], __BSIZE, " ");
	size[0] = 4;

	/* draw box */
	ci_box_top_ary(size, (const char **)head, max_col);
	ci_node_each(node, {
		ci_snprintf(body_name[0], __BSIZE, "%d/00", node->node_id);

		ci_worker_each(node, worker, {
			u64 busy_cycle, percent;
			int i = worker->worker_id % (max_col - 1);
			
			if (ci_sta_last_hist_val(worker->sta, "1_SEC", 0, &busy_cycle) < 0) 
				ci_snprintf(body_name[i + 1], __BSIZE, "n/a");
			else {
				percent = ci_min(100, ((busy_cycle * 1000) / PAL_CYCLE_PER_SEC + 5) / 10);
				ci_snprintf(body_name[i + 1], __BSIZE, "%3llu", percent);
			}

			if ((i == max_col - 2) || (worker->worker_id == node->nr_worker - 1)) {
				ci_loop(j, i + 1, max_col - 1)
					ci_snprintf(body_name[j + 1], __BSIZE, " ");

				ci_box_body_ary(!row++, size, (const char **)body, max_col);
				ci_snprintf(body_name[0], __BSIZE, "%d/%02d", node->node_id, worker->worker_id + 1);
			}
		});
		
	});

	ci_box_btm_ary(size, max_col);
	return 0;

#undef __COL
#undef __BSIZE
}
#endif


ci_mod_def(mod_ci, {
	.name = "$cmd",
	.desc = "ci built-in commands",
	.jcmd = ci_mod_jcmd		
});

