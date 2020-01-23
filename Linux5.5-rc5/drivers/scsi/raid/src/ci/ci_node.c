/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_node.c					CI Node
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

static ci_node_info_t node_info;

int ci_node_info_dump() 
{
	int rv = 0;
	ci_big_nr_parse_t mem_p = { .flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_POWER_OF_2 };

	rv += ci_imp_printf("node_info: %p, nr_node=%d", ci_node_info, ci_node_info->nr_node);
	ci_dump_preln(", node_com_map: ", ci_node_map_dump_ex(&ci_node_info->node_com_map, CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_COMMA));

	ci_node_each(node, {
		mem_p.big_nr = node->range.end - node->range.start;
		ci_big_nr_parse(&mem_p);
		
		rv += ci_printf("node_id=%d, numa_id=%d, ptr=%p, nr_worker=%d", node->node_id, node->numa_id, node, node->nr_worker);
		rv += ci_dump_preln(", worker_com_map=", ci_worker_map_dump(&node->worker_com_map));
		rv += ci_printf(CI_PR_INDENT "mem_range [%p, %p), %d.%03d %sB (%lli)\n", 
						node->range.start, node->range.end,
				  		mem_p.int_part, mem_p.dec_part, mem_p.unit, mem_p.big_nr);	

		ci_worker_each(node, worker, {
			pal_worker_t *pw = worker->pal_worker;
			rv += ci_printf(CI_PR_INDENT "worker < node/worker=%d/%02d, %p > => pal_worker < numa/cpu=%d/%02d, %p >\n", 
							node->node_id, worker->worker_id, worker, pw->numa_id, pw->cpu_id, pw);
		});
	});
	
	return rv;
}

static void __ci_node_init(ci_node_t *node)
{
	u8 *ptr;
	ci_balloc_t *ba;

	/* ha for node */
	ci_halloc_init(&node->ha, "ha_node", node->range.start, node->range.end);
	node->ha.node = node;

	/* ba for node */
	ba = &node->ba;
	ptr = ci_node_halloc(node->node_id, CI_BALLOC_NODE_SIZE, PAL_CPU_CACHE_LINE_SIZE, ci_ssf("ba.%d", node->node_id));	
	ci_balloc_init(ba, "ba_node", ptr, ptr + CI_BALLOC_NODE_SIZE);
	ba->node_id = node->node_id;

	/* ba for paver */
	ba = &node->ba_paver;
	ptr = ci_node_halloc(node->node_id, CI_BALLOC_PAVER_SIZE, PAL_CPU_CACHE_LINE_SIZE, ci_ssf("ba_paver.%d", node->node_id));	
	ci_balloc_init(ba, "ba_paver", ptr, ptr + CI_BALLOC_PAVER_SIZE);
	ba->node_id = node->node_id;
}

int ci_node_pre_init()
{
	ci_node_info = &node_info;
	ci_slk_init(&ci_node_info->lock);
	return 0;
}

static void ci_node_alloc_shr_ba()
{
	u8 *ptr;

	/* shared ba */
	ptr = ci_shr_halloc(CI_BALLOC_SHR_SIZE, 0, "ba_shr");
	ci_balloc_init(&ci_node_info->ba_shr, "ba_shr", ptr, ci_node_info->ha_shr.range.end);
	ci_node_info->ba_shr.flag |= CI_BALLOC_LAZY_CONQUER;

	/* ba for json */
	ptr = ci_shr_halloc(CI_BALLOC_JSON_SIZE, 0, "ba_json"); 
	ci_balloc_init(&ci_node_info->ba_json, "ba_json", ptr, ptr + CI_BALLOC_JSON_SIZE);

}

int ci_node_init()
{
	ci_halloc_init(&ci_node_info->ha_shr, "ha_shr", ci_node_info->range.start, ci_node_info->range.end);
	ci_node_each(node, __ci_node_init(node));
	ci_node_alloc_shr_ba();
	
	ci_node_info_dump();
	return 0;
}

