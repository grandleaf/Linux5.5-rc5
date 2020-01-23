/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_halloc.h					Heap Allocator (without free functions)
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#define halloc_name(ha)		((ha)->node ? ci_ssf("%s[%d]", (ha)->name, (ha)->node->node_id) : ci_ssf("%s", (ha)->name))


int ci_halloc_init(ci_halloc_t *ha, const char *name, u8 *start, u8 *end)
{
	ci_assert(ha && name && (end > start));
	
	ci_obj_zero(ha);
	ha->flag = CI_HALLOC_MT;
	ci_slk_init(&ha->lock);
	ci_list_init(&ha->head);

	ha->name = name;
	ha->range.start = ha->range.curr = start;
	ha->range.end = end;

	return 0;
}

void *ci_halloc(ci_halloc_t *ha, int size, int align, const char *name)
{
	int name_size;
	ci_halloc_rec_t *rec;

	ci_assert(ha && name && ((size > 0) || (size == -1)));
	name_size = ci_strsize(name);
	ci_max_set(align, PAL_CPU_ALIGN_SIZE);

	if (ha->flag & CI_HALLOC_MT)
		ci_slk_lock(&ha->lock);
	
	rec = (ci_halloc_rec_t *)ha->range.curr;	/* allocate rec object */
	ci_ptr_align_cpu_asg(rec);

	ha->range.curr = (u8 *)(rec + 1) + name_size;	/* pointer for allocation */
	ci_ptr_align_upper_asg(ha->range.curr, align);

	(size < 0) && (size = ha->range.end - ha->range.curr);	/* < 0 means grab all */
	ci_assert(size > 0);
	ci_panic_if(ha->range.curr + size > ha->range.end, "NOT ENOUGH MEMORY!  ha=<%p, \"%s\">, client=\"%s\"", ha, ha->name, name);

	ci_obj_zero(rec);	/* build rec object */
	ci_mem_anchor_set(rec);
	ci_memcpy(rec + 1, name, name_size);
	rec->range.start 	= ha->range.curr;
	rec->range.end 		= ha->range.curr = rec->range.start + size;
	rec->name			= (const char *)(rec + 1);
	ci_list_add_tail(&ha->head, &rec->link);

	if (ha->flag & CI_HALLOC_MT)
		ci_slk_unlock(&ha->lock);
	
	return rec->range.start;
}

int ci_halloc_dump(ci_halloc_t *ha)
{
#define __FMT_PRE				CI_PR_INDENT "[%03d] %-"
#define __FMT_PST				"s => %2lld.%03lld%%, rec=%p, range=" CI_PR_RANGE_BNP_FMT

	int i, rv, max_name;
	u64 total, used, avail;
	ci_halloc_rec_t *rec;
	char fmt_str[ci_sizeof(__FMT_PRE) + ci_sizeof(__FMT_PST) + 2];		/* try to make print nice looking */
	ci_dbg_paste(ci_halloc_rec_t *prev_rec = NULL);

	if (ha->flag & CI_HALLOC_MT)
		ci_slk_lock(&ha->lock);

	i = rv = max_name = 0;
	total = ha->range.end  - ha->range.start;
	used  = ha->range.curr - ha->range.start;
	avail = ha->range.end  - ha->range.curr;

	rv += ci_printf("%s=%p", halloc_name(ha), ha);
	rv += ci_printf(", range=" CI_PR_RANGE_EX_FMT, ci_pr_range_ex_val(&ha->range));
	rv += ha->node ? ci_printfln(", binding:%d", pal_numa_id_by_ptr(ha->range.start)) : ci_printfln(); 
	rv += ci_printf(CI_PR_INDENT "used=" CI_PR_BNP_FMT "->" CI_PR_PCT_FMT, ci_pr_bnp_val(used), ci_pr_pct_range_ex_used(&ha->range));
	rv += ci_printf(", avail=" CI_PR_BNP_FMT "->" CI_PR_PCT_FMT, ci_pr_bnp_val(avail), ci_pr_pct_range_ex_avail(&ha->range));
	rv += ci_printfln(", total=" CI_PR_BNP_FMT, ci_pr_bnp_val(ha->range.end  - ha->range.start));

	ci_list_each(&ha->head, rec, link) {	/* validation, also calculate the max name size */
		ci_dbg_exec(
			if (!ci_mem_anchor_valid(rec)) {
				ci_err_printfln("ci_halloc_rec=%p corrupted!  owner=<ha:%p, name:\"%s\">", rec, ha, ha->name);
				if (prev_rec)
					ci_printfln("possible caused by ci_halloc_rec=%p, name=\"%s\"", prev_rec, prev_rec->name);
				ci_panic();
			}

			prev_rec = rec;
		);

		ci_max_set(max_name, ci_strlen(rec->name));
	}

	rv += ci_snprintf(fmt_str, ci_sizeof(fmt_str), "%s%d%s", __FMT_PRE, max_name, __FMT_PST);		/* build the format string */
	ci_list_each(&ha->head, rec, link) {
		rv += ci_printfln(fmt_str, i, rec->name, ci_pr_pct_val(rec->range.end - rec->range.start, total), rec, ci_pr_range_bnp_val(&rec->range));
		i++;
	}	

	if (ha->flag & CI_HALLOC_MT)
		ci_slk_unlock(&ha->lock);

	return rv;
}

