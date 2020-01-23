/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_balloc.c			Buddy System Memory Allocator (without free functions)
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

#define CI_MAP_PTN_FREE								0xFF
#define CI_MAP_BIT_BUSY								0x80
#define CI_MAP_SHIFT_MASK							(~(CI_MAP_BIT_BUSY))

#define balloc_ptr_to_map_idx(ba, ptr)				\
	({	\
		int __map_idx__ = (int)(((u8 *)(ptr) - (ba)->mem_range.start) >> CI_BALLOC_MIN_SHIFT);		\
		ci_range_check((u8 *)(ptr), (ba)->mem_range.start, (ba)->mem_range.end);	\
		ci_range_check(__map_idx__, 0, ci_mem_range_len(&(ba)->map_range));		\
		__map_idx__;	\
	})
#define balloc_map_idx_to_ptr(ba, map_idx)			\
	({	\
		u8 *__ptr__ = (ba)->mem_range.start + ((map_idx) << CI_BALLOC_MIN_SHIFT);		\
		ci_range_check(__ptr__, (ba)->mem_range.start, (ba)->mem_range.end);	\
		__ptr__;	\
	})
#define balloc_insert_bucket(ba, ptr, shift)		balloc_insert_bucket_ex(ba, ptr, balloc_ptr_to_map_idx(ba, ptr), shift)
#define balloc_remove_bucket(ba, ptr, shift)		balloc_remove_bucket_ex(ba, ptr, balloc_ptr_to_map_idx(ba, ptr), shift)
#define balloc_map_len(shift)						((1 << (shift)) >> CI_BALLOC_MIN_SHIFT)
#define balloc_name(ba)								((ba)->node_id >= 0 ? ci_ssf("%s[%d]", (ba)->name, (ba)->node_id) : ci_ssf("%s", (ba)->name))


static ci_int_to_name_t balloc_flag_to_name[] = {
	ci_int_name(CI_BALLOC_MT),
	ci_int_name(CI_BALLOC_LAZY_CONQUER),
	CI_EOT	
};

static void balloc_insert_bucket_ex(ci_balloc_t *ba, u8 *ptr, int map_idx, int shift)
{
	u8 *map = ba->map_range.start + map_idx;

	ci_ptr_align_check(ptr - ba->map_range.start, CI_BALLOC_MIN_SIZE);
	ci_range_check(ptr, ba->mem_range.start, ba->mem_range.end);
	ci_range_check(map_idx, 0, ci_mem_range_len(&ba->map_range));
	ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);

	ci_assert((*map == CI_MAP_PTN_FREE) || ((*map & CI_MAP_BIT_BUSY) && ((*map & CI_MAP_SHIFT_MASK) == shift)));
	*map = shift;
	
	ci_list_dbg_poison_set((ci_list_t *)ptr);		/* uncertain data in middle */
	ci_list_add_head(&ba->bucket[shift], (ci_list_t *)ptr);
	ci_balloc_bucket_map_set_bit(&ba->bucket_map, shift);
//ci_printf("insert %p to [%02d], size=%i, map_idx=%d, *map=%#02X\n", ptr, shift, 1 << shift, map_idx, *map);
}

static void balloc_remove_bucket_ex(ci_balloc_t *ba, u8 *ptr, int map_idx, int shift)
{
	u8 *map = ba->map_range.start + map_idx;

	ci_ptr_align_check(ptr - ba->map_range.start, CI_BALLOC_MIN_SIZE);
	ci_range_check(map_idx, 0, ci_mem_range_len(&ba->map_range));
	ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);

	ci_assert((*map == CI_MAP_PTN_FREE) || (!(*map & CI_MAP_BIT_BUSY) && (*map == shift)));
	*map = shift | CI_MAP_BIT_BUSY;
	
	ci_list_del((ci_list_t *)ptr);
	if (ci_unlikely(ci_list_empty(&ba->bucket[shift])))
		ci_balloc_bucket_map_clear_bit(&ba->bucket_map, shift);	
//ci_printf("remove %p to [%02d], size=%i, map_idx=%d, *map=%#02X\n", ptr, shift, 1 << shift, map_idx, *map);
}

void balloc_bucket_init(ci_balloc_t *ba)
{
	int shift, size;
	u8 *start = ba->mem_range.start;

	ci_loop(idx, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET) {
		ci_list_init(ba->bucket + idx);
		ci_list_init(ba->cache + idx);	/* also init cache */
	}

	for (;;) {
		if (!(size = (int)(ba->mem_range.end - start)))		/* nothing left */
			break;
		
		if ((shift = ci_min(u32_last_set(size), CI_BALLOC_MAX_SHIFT)) < CI_BALLOC_MIN_SHIFT) {	/* too small */
			ba->mem_range.end -= 1 << shift;
			break;
		}

		ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);
//		ci_dbg_exec(ci_memset(start, CI_BUF_PTN_INIT, 1 << shift));

		balloc_insert_bucket(ba, start, shift);
		start += 1 << shift;
	}
}

int ci_balloc_init(ci_balloc_t *ba, const char *name, u8 *start, u8 *end)
{
	int min_len;
	ci_assert(ba && name && (end > start));
	
	ci_obj_zero(ba);
	ba->flag |= CI_BALLOC_MT;
	ci_balloc_bucket_map_zero(&ba->bucket_map);
	ci_slk_init(&ba->lock);
	ba->name = name;
	ba->node_id = -1;		/* doesn't matter */

	ci_ptr_align_cache_asg(start);	/* start, end to cache line size */
	ci_ptr_align_lower_asg(end, PAL_CPU_CACHE_LINE_SIZE);

	/* allocate memory for map */
	ba->map_range.start= start;			
	ba->map_range.end = start + (end - start) / (1 + CI_BALLOC_MIN_SIZE);

	/* [start, end) for data buffers */
	ba->mem_range.start = ci_ptr_align_cache(ba->map_range.end);		
	ba->mem_range.end = end;
	ci_assert(ci_mem_range_len(&ba->mem_range) > 0);

	/* size readjust */
	min_len = ci_min(ci_mem_range_len(&ba->map_range), ci_mem_range_len(&ba->mem_range) / CI_BALLOC_MIN_SIZE);
	ba->map_range.end = ba->map_range.start + min_len;
	ba->mem_range.end = ba->mem_range.start + min_len * CI_BALLOC_MIN_SIZE;

	/* fill with pre-defined pattern and do init */
	ci_memset(ba->map_range.start, CI_MAP_PTN_FREE, ci_mem_range_len(&ba->map_range));
	balloc_bucket_init(ba);

	return 0;
}

static void balloc_divide(ci_balloc_t *ba, int from)
{
	u8 *ptr;
	int to, map_idx;

	if ((to = ci_balloc_bucket_map_next_set(&ba->bucket_map, from)) < 0)
		return;		/* no memory */

	ci_range_check(to, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);
	ptr = (u8 *)ci_list_head(&ba->bucket[to]);
	ci_assert(ptr);
	map_idx = balloc_ptr_to_map_idx(ba, ptr);

	balloc_remove_bucket_ex(ba, ptr, map_idx, to);		/* remove the big chunk */
	ba->map_range.start[map_idx] = CI_MAP_PTN_FREE;		/* offset the remove's busy set */
	balloc_insert_bucket_ex(ba, ptr, map_idx, from);

	ci_loop_i(idx, to - 1, from, -1)	/* divide & insert */
		balloc_insert_bucket(ba, ptr + (1 << idx), idx);
}

static u8 *balloc_conquer(ci_balloc_t *ba, u8 *ptr)
{
	u8 *lower_ptr, *upper_ptr;
	int map_idx, map_len, shift, mask, upper_map_idx, lower_map_idx;

	map_idx = balloc_ptr_to_map_idx(ba, ptr);
	shift = ba->map_range.start[map_idx];
	ci_assert(!(shift & CI_MAP_BIT_BUSY));
	if (shift >= CI_BALLOC_MAX_SHIFT)
		return NULL;

	map_len = balloc_map_len(shift);
	mask = (1 << ((shift + 1) - CI_BALLOC_MIN_SHIFT)) - 1;

	if (map_idx & mask) {	/* I am the upper buddy */
		upper_map_idx = map_idx;
		lower_map_idx = map_idx - map_len;
		if (ci_unlikely(lower_map_idx < 0))
			return NULL;
	} else {				/* I am lower buddy */
		lower_map_idx = map_idx;
		upper_map_idx = map_idx + map_len;
		if (ci_unlikely(upper_map_idx + map_len > ci_mem_range_len(&ba->map_range)))
			return NULL;
	}

	if (ba->map_range.start[lower_map_idx] != ba->map_range.start[upper_map_idx])	/* all free and same shift? */
		return NULL;

	lower_ptr = balloc_map_idx_to_ptr(ba, lower_map_idx);	/* remove lower */
	balloc_remove_bucket_ex(ba, lower_ptr, lower_map_idx, shift);
	ba->map_range.start[lower_map_idx] = CI_MAP_PTN_FREE;

	upper_ptr = balloc_map_idx_to_ptr(ba, upper_map_idx);	/* remove upper */
	balloc_remove_bucket_ex(ba, upper_ptr, upper_map_idx, shift);
	ba->map_range.start[upper_map_idx] = CI_MAP_PTN_FREE;

	balloc_insert_bucket_ex(ba, lower_ptr, lower_map_idx, shift + 1);	/* insert merged */
	ba->nr_conquer++;
	return lower_ptr;
}

static u8 *balloc_intl(ci_balloc_t *ba, int shift)
{
	ci_list_t *ent, *bucket;

	ba->nr_alloc++;
	bucket = &ba->bucket[shift];
	
	if (ci_list_empty(bucket) && (shift < CI_BALLOC_MAX_SHIFT))
		balloc_divide(ba, shift);

	if (ci_unlikely(!(ent = ci_list_head(bucket))))
		ba->nr_alloc_fail++;
	else {
		balloc_remove_bucket(ba, (u8 *)ent, shift);
		ba->size_alloc += 1 << shift;
		ci_max_set(ba->max_size_alloc, ba->size_alloc);
	}

	return (u8 *)ent;
}

static void bfree_intl(ci_balloc_t *ba, u8 *ptr, int map_idx, int shift)
{
	balloc_insert_bucket_ex(ba, ptr, map_idx, shift);

	ba->nr_free++;
	ba->size_alloc -= 1 << shift;

	while ((ptr = balloc_conquer(ba, ptr)))
		;
}

void ci_balloc_free_cache(ci_balloc_t *ba)
{
	ci_list_t *head, *ent;

	ci_loop(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET) {
		head = &ba->cache[shift];

		if (!ci_list_empty(head)) {
			ci_list_each_ent_safe(&ba->cache[shift], ent) {
				ci_list_dbg_poison_set(ent);		/* insert to bucket without remove from cache */
				bfree_intl(ba, (u8 *)ent, balloc_ptr_to_map_idx(ba, ent), shift);
				
				ba->nr_obj_in_cache--;
				ba->size_in_cache -= 1 << shift;
			}
			ci_list_init(head);
		}
	}
}

void *__ci_balloc(ci_balloc_t *ba, int size)
{
	u8 *ptr;
	int shift;
	ci_list_t *ent;

	ci_assert(ba && (size > 0));
	shift = ci_max(ci_log2_ceil(size), CI_BALLOC_MIN_SHIFT);
	ci_assert(shift < CI_BALLOC_NR_BUCKET, "size=%i is too big", size);
//ci_printf("xxxxxxxxxxxxxxxxx shift=%d, size=%d\n", shift, size);	

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_lock(&ba->lock);

	if (!(ba->flag & CI_BALLOC_LAZY_CONQUER)) {
		ptr = balloc_intl(ba, shift);
		goto __exit;
	}

	ba->nr_cache_alloc++;
	if ((ent = ci_list_del_head(&ba->cache[shift]))) {		/* cached */
		ba->nr_obj_in_cache--;
		ba->size_in_cache -= 1 << shift;
		ptr = (u8 *)ent;
		goto __exit;
	}

	ba->nr_cache_alloc_fail++;
	if ((ptr = balloc_intl(ba, shift)))		/* try alloc */
		goto __exit;

	ci_balloc_free_cache(ba);		/* free all cached */
	ptr = balloc_intl(ba, shift);

__exit:
	if (ba->flag & CI_BALLOC_MT)
		ci_slk_unlock(&ba->lock);
	
	return ptr;
}

void __ci_bfree(ci_balloc_t *ba, void *ptr)
{
	int map_idx, shift;
	
	map_idx = balloc_ptr_to_map_idx(ba, ptr);
	shift = ba->map_range.start[map_idx] & CI_MAP_SHIFT_MASK;
	ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);

	ci_list_dbg_poison_set((ci_list_t *)ptr);	/* user damage the memory */

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_lock(&ba->lock);

	if (!(ba->flag & CI_BALLOC_LAZY_CONQUER))
		bfree_intl(ba, (u8 *)ptr, map_idx, shift);
	else {
		ci_list_add_head(&ba->cache[shift], (ci_list_t *)ptr);
		ba->nr_obj_in_cache++;
		ba->size_in_cache += 1 << shift;
		ci_max_set(ba->max_size_in_cache, ba->size_in_cache);
		ba->nr_cache_free++;
	}
	
	if (ba->flag & CI_BALLOC_MT)
		ci_slk_unlock(&ba->lock);
}

int __ci_bfree_size(ci_balloc_t *ba, u8 *ptr)
{
	int map_idx, shift;

	map_idx = balloc_ptr_to_map_idx(ba, ptr);
	shift = ba->map_range.start[map_idx] & CI_MAP_SHIFT_MASK;
	ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);

	return 1 << shift;
}

static int ci_balloc_free_bytes(ci_balloc_t *ba)
{
	int bytes = 0;

	ci_balloc_bucket_map_each_set(&ba->bucket_map, idx) 
		bytes += (1 << idx) * ci_list_count(&ba->bucket[idx]);

	if (ba->flag & CI_BALLOC_LAZY_CONQUER)
		ci_loop_i(idx, CI_BALLOC_MIN_SHIFT, CI_BALLOC_MAX_SHIFT)
			bytes += (1 << idx) * ci_list_count(&ba->cache[idx]);

	return bytes;
}

static int ci_balloc_usage_dump(ci_balloc_t *ba)
{
	int total, used, avail, rv = 0;

	total = ci_mem_range_len(&ba->mem_range);
	avail = ci_balloc_free_bytes(ba);
	used  = total - avail;

	rv += ci_printf(CI_PR_INDENT "used=" CI_PR_BNP_FMT "->" CI_PR_PCT_FMT, ci_pr_bnp_val(used), ci_pr_pct_val(used, total));
	rv += ci_printf(", avail=" CI_PR_BNP_FMT "->" CI_PR_PCT_FMT, ci_pr_bnp_val(avail), ci_pr_pct_val(avail, total));
	rv += ci_printfln(", total=" CI_PR_BNP_FMT, ci_pr_bnp_val(total));

	return rv;
}

int ci_balloc_dump(ci_balloc_t *ba)
{
	int rv = 0;

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_lock(&ba->lock);

	rv += ci_printf("%s=%p, flag=%s", balloc_name(ba), ba, ci_flag_str(balloc_flag_to_name, ba->flag));
	rv += ba->node_id >= 0 ? ci_printfln(", bind:%d", pal_numa_id_by_ptr(ba->mem_range.start)) : ci_printfln();
	rv += ci_balloc_usage_dump(ba);
	rv += ci_printfln(CI_PR_INDENT "map_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->map_range));
	rv += ci_printfln(CI_PR_INDENT "mem_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->mem_range));

	rv += ci_printf("\n" CI_PR_INDENT "nr_alloc=%lli, nr_alloc_fail=%lli", ba->nr_alloc, ba->nr_alloc_fail);
	rv += ba->nr_alloc ? ci_printfln(", success=" CI_PR_PCT_FMT, ci_pr_pct_val(ba->nr_alloc - ba->nr_alloc_fail, ba->nr_alloc))
					   : ci_printfln();
	rv += ci_printfln(CI_PR_INDENT "nr_free=%lli, nr_pending=%lli", ba->nr_free, ci_balloc_nr_pending(ba));
	rv += ci_printfln(CI_PR_INDENT "size_alloc=%lli, max_size_alloc=%lli (" CI_PR_PCT_FMT "), nr_conquer=%lli", 
					  ba->size_alloc, ba->max_size_alloc, 
					  ci_pr_pct_val(ba->max_size_alloc, ci_mem_range_len(&ba->mem_range)), ba->nr_conquer);

	if (ba->flag & CI_BALLOC_LAZY_CONQUER) {
		rv += ci_printf("\n" CI_PR_INDENT "nr_cache_alloc=%lli, nr_cache_alloc_fail=%lli", ba->nr_cache_alloc, ba->nr_cache_alloc_fail);
		rv += ba->nr_cache_alloc ? ci_printfln(", hit=" CI_PR_PCT_FMT, 
												ci_pr_pct_val(ba->nr_cache_alloc - ba->nr_cache_alloc_fail, ba->nr_cache_alloc))
								 : ci_printfln();
		rv += ci_printfln(CI_PR_INDENT "nr_cache_free=%lli, nr_obj_in_cache=%lli", ba->nr_cache_free, ba->nr_obj_in_cache);
		rv += ci_printfln(CI_PR_INDENT "size_in_cache=%lli, max_size_in_cache=%lli", ba->size_in_cache, ba->max_size_in_cache);
	}

	rv += ci_printf("\n" CI_PR_INDENT "bucket_map=0x");
	ci_balloc_bucket_map_dump_exln(&ba->bucket_map, CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_COMMA);

	ci_balloc_bucket_map_each_set(&ba->bucket_map, idx) {
		ci_list_t *ent, *head = &ba->bucket[idx];
		
		rv += ci_printfln("\n" CI_PR_INDENT "[%02d] total=%d, size=%i ", idx, ci_list_count(head), 1 << idx);
		ci_list_each_ent(head, ent)
			rv += ci_printfln(CI_PR_INDENT "%s[%p, %p)", 
							  ci_pr_outline_str(ent == ci_list_tail(head) ? CI_PR_OUTLINE_DIRECT_LAST : CI_PR_OUTLINE_DIRECT), 
							  ent, (u8 *)ent + (1 << idx));
	}

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_unlock(&ba->lock);

	return rv;
}

int ci_balloc_dump_brief(ci_balloc_t *ba)
{
	int rv = 0;

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_lock(&ba->lock);

	rv += ci_printf("%s=%p, flag=%s", balloc_name(ba), ba, ci_flag_str(balloc_flag_to_name, ba->flag));
	rv += ba->node_id >= 0 ? ci_printfln(", bind:%d", pal_numa_id_by_ptr(ba->mem_range.start)) : ci_printfln();
	rv += ci_balloc_usage_dump(ba);
	rv += ci_printfln(CI_PR_INDENT "map_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->map_range));
	rv += ci_printfln(CI_PR_INDENT "mem_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->mem_range));

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_unlock(&ba->lock);

	return rv;
}

int ci_balloc_dump_pending(ci_balloc_t *ba)
{
	int rv, total;

	if (ba->flag & CI_BALLOC_MT)
		ci_slk_lock(&ba->lock);

	ci_balloc_free_cache(ba);	/* remove objects from cache first */

	rv = total = 0;
	rv += ci_printf("%s=%p, flag=%s", balloc_name(ba), ba, ci_flag_str(balloc_flag_to_name, ba->flag));
	rv += ci_balloc_usage_dump(ba);
	rv += ci_printfln(CI_PR_INDENT "map_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->map_range));
	rv += ci_printfln(CI_PR_INDENT "mem_range=" CI_PR_RANGE_BNP_FMT, ci_pr_range_bnp_val(&ba->mem_range));
	rv += ci_printfln(CI_PR_INDENT "nr_pending=%lli", ci_balloc_nr_pending(ba));

	if (!ci_balloc_nr_pending(ba))
		goto __exit;
	
	ci_loop(shift_loop, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET) {
		u8 *start, *end;
		int shift, sub_total = 0;
		
		ci_loop(map, ba->map_range.start, ba->map_range.end) {
			if ((*map == CI_MAP_PTN_FREE) || !(*map & CI_MAP_BIT_BUSY))
				continue;

			shift = *map & CI_MAP_SHIFT_MASK;
			ci_range_check(shift, CI_BALLOC_MIN_SHIFT, CI_BALLOC_NR_BUCKET);
			if (shift == shift_loop)
				sub_total++, total++;
		}

		if (!sub_total)
			continue;

		rv += ci_printfln("\n" CI_PR_INDENT "[%02d] total=%d, size=%i", shift_loop, sub_total, 1 << shift_loop);
		ci_loop(map, ba->map_range.start, ba->map_range.end) {	/* again */
			if ((*map == CI_MAP_PTN_FREE) || !(*map & CI_MAP_BIT_BUSY))
				continue;

			shift = *map & CI_MAP_SHIFT_MASK;
			if (shift != shift_loop)
				continue;

			start = balloc_map_idx_to_ptr(ba, map - ba->map_range.start);
			end = start + (1 << shift);

			sub_total--;
			rv += ci_printf(CI_PR_INDENT "%s[%p, %p)", 
							ci_pr_outline_str(!sub_total ? CI_PR_OUTLINE_DIRECT_LAST : CI_PR_OUTLINE_DIRECT), start, end);

			ci_ba_dbg_exec(	
				ci_mem_guard_head_t *guard = (ci_mem_guard_head_t *)start;
				ci_access_tag_t *acc = &guard->__access_tag;
			
				rv += ci_printf(", obj=%p, " CI_PR_ACCESS_TAG_FMT, start + ci_sizeof(ci_mem_guard_head_t), ci_pr_access_tag_val(acc));
			);

			rv += ci_printfln();
		}
	}

	ci_assert(total == ci_balloc_nr_pending(ba));

__exit:
	if (ba->flag & CI_BALLOC_MT)
		ci_slk_unlock(&ba->lock);

	return rv;
}

 
