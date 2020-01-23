/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_paver.c					CI Paver Resource Allocator
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

int CI_WDID_PAVER;


#ifdef CI_PAVER_STA
//#define CI_PAVER_HIST_DUMP_NAME				"5_SEC"
//#define CI_PAVER_POOL_HIST_DUMP_NAME		"5_SEC"

static ci_sta_cfg_hist_t paver_sta_cfg_hist[] = {
	{ "1_SEC",			1000,				60 	},		/* 1000 ms, 60 samples */
	{ "5_SEC",			5000,				60 	},		/* 5000 ms, 60 samples */
	{ "1_MIN",			60000,				60 	},		/* 1 minute, 60 samples */
	CI_EOT
};

static ci_sta_cfg_hist_t paver_pool_sta_cfg_hist[] = {
	{ "1_SEC",			1000,				60 	},		/* 1000 ms, 60 samples */
	{ "5_SEC",			5000,				60 	},		/* 5000 ms, 60 samples */
	{ "1_MIN",			60000,				60 	},		/* 1 minute, 60 samples */
	CI_EOT
};

#ifdef CI_PAVER_HIST_DUMP_NAME
static ci_sta_dump_dpt_t paver_dump_dpt[] = ci_sta_dump_dpt_maker(ci_paver_sta_acc_t, 
	alloc,
	unready,
/*	
	free,
	ready,
	unready,
	full_to_partial,
	partial_to_full,
	partial_to_empty,
	empty_to_partial,
*/	

	bucket_pool_alloc,
	bucket_pool_free,
	bucket_buddy_alloc,
	bucket_buddy_free,

/*
	max_free_obj,
	min_free_obj,
	max_total_obj,
	min_total_obj
 */	
	max_total_obj,
);
#endif /* !CI_PAVER_HIST_DUMP_NAME */

#ifdef CI_PAVER_POOL_HIST_DUMP_NAME
static ci_sta_dump_dpt_t paver_pool_dump_dpt[] = ci_sta_dump_dpt_maker(ci_paver_pool_sta_acc_t, 
	shuffle_free,
	shuffle_alloc_succ,
	shuffle_alloc_fail,
	buddy_alloc_succ,
	buddy_alloc_fail,
	bucket_alloc,
	bucket_alloc_delayed
);
#endif /* !CI_PAVER_POOL_HIST_DUMP_NAME */

#endif /* !CI_PAVER_STA */


static ci_int_to_name_t paver_pool_flag_to_name[] = {
	ci_int_name(CI_PAVER_POOLF_Q_NO_RES),
	ci_int_name(CI_PAVER_POOLF_Q_HAS_RES),
	CI_EOT	
};

static ci_int_to_name_t paver_flag_to_name[] = {
	ci_int_name(CI_PAVERF_INITED),
	ci_int_name(CI_PAVERF_READY),
};

static ci_int_to_name_t paver_alloc_task_flag_to_name[] = {
	ci_int_name(CI_PAVER_ALLOC_TASKF_QUEUED),
	ci_int_name(CI_PAVER_ALLOC_TASKF_ALLOCATED),
	CI_EOT	
};

static ci_int_to_name_t paver_node_info_flag_to_name[] = {
	ci_int_name(CI_PAVER_NODEF_NO_RES),
	CI_EOT	
};


static void ci_paver_free_increased(ci_paver_t *paver);
static ci_paver_bucket_t *ci_paver_pool_bucket_balloc(ci_paver_pool_t *pool);
static void ci_paver_pool_bucket_increased(ci_paver_pool_t *pool, int can_free_to_node);


static void ci_paver_info_init()
{
	static ci_paver_info_t paver_info;
	ci_paver_info = &paver_info;

	ci_node_id_each(node_id) {
		ci_paver_node_info_t *info = &ci_paver_info->pn_info[node_id];
		
		info->node_id = node_id;
		info->ba = &ci_node_info->node[node_id]->ba_paver;
		
		ci_list_init(&info->pool_head);
		ci_list_init(&info->pool_has_res_head);
		ci_list_init(&info->pool_no_res_head);
		ci_slk_init(&info->lock);
	}
}

int ci_paver_init()
{
	void *data;
	int alloc_size = ci_sizeof(ci_paver_t *) * CI_PAVER_NR;

	ci_paver_info_init();
	CI_WDID_PAVER = ci_worker_data_register(CI_SWDID_PAVER);		/* register per worker data */

	ci_node_worker_each(node, worker, {
		data = ci_node_halloc(node->node_id, alloc_size, 0, ci_ssf("paver_tab.%d.%02d", node->node_id, worker->worker_id));
		ci_memzero(data, alloc_size);
		ci_worker_data_set(ci_sched_ctx_by_id(node->node_id, worker->worker_id), CI_WDID_PAVER, data);
	});

	return 0;
}

static void ci_paver_charge(ci_paver_t *paver)
{
	ci_paver_bucket_t *bucket;
	ci_paver_pool_t *pool = paver->pool;

	while (paver->nr_free < pool->thr_satisfy) {
		bucket = ci_paver_pool_bucket_balloc(pool);
		ci_assert(bucket, "out of memory, try enlarging CI_BALLOC_PAVER_SIZE");

		paver->nr_free += pool->bucket_obj_nr;
		paver->nr_total += pool->bucket_obj_nr; 
		ci_list_add_head(&paver->bucket_full, &bucket->link);
	}

	ci_paver_sta_acc_max_no_ctx(paver, max_total_obj, paver->nr_total);
	ci_paver_free_increased(paver);
	paver->flag |= CI_PAVERF_INITED;
}

static void __ci_paver_init(ci_paver_pool_t *pool, ci_paver_t *paver, int worker_id)
{
	ci_paver_alloc_task_t *pat = &paver->alloc_task;

	/* init the pavere */
	ci_obj_zero(paver);
	paver->pool 		= pool;
	paver->worker_id 	= worker_id;
	paver->sched_tab	= &ci_worker_by_id(pool->pn_info->node_id, worker_id)->sched_tab;

	ci_list_init(&paver->bucket_full);
	ci_list_init(&paver->bucket_partial);
	ci_list_init(&paver->bucket_empty);
	ci_list_add_tail(&pool->paver_head, &paver->link);

	/* init statistics */
#ifdef CI_PAVER_STA
	ci_sta_cfg_t cfg = {
		.hist				= paver_sta_cfg_hist,
		.acc_size			= ci_sizeof(ci_paver_sta_acc_t)
	};	

	cfg.name		= pool->name;
	cfg.node_id 	= pool->pn_info->node_id;
	cfg.worker_id 	= worker_id;
	
	paver->sta = ci_sta_create(&cfg);
	
#ifdef CI_PAVER_HIST_DUMP_NAME	
	ci_sta_set_hist_dump(paver->sta, CI_PAVER_HIST_DUMP_NAME, paver_dump_dpt, 1, 0);				
#endif
#endif

	/* init alloc_task */
	ci_list_init(&pat->bucket_head);
	pat->ctx = ci_sched_ctx_by_id(pool->pn_info->node_id, worker_id);
	pat->paver_id = pool->paver_id;
	
	ci_paver_charge(paver);
}

static void ci_paver_cfg_check(ci_paver_cfg_t *cfg)
{
#ifdef CI_PAVER_DEBUG
	ci_assert(cfg->alloc_max > 0);
	ci_assert(!ci_node_map_empty(&cfg->node_map));

	ci_node_map_each_set(&cfg->node_map, node_id) 
		ci_assert(!ci_worker_map_empty(&cfg->worker_map[node_id]));
#endif	
}

static void ci_paver_calc_obj(ci_paver_pool_t *pool)
{
	int head_size, alloc_size;

#ifdef CI_PAVER_DEBUG
	/* | head | padding | head_guard | user_obj | tail_guard | padding |	=> head_size = head + padding + head_guard */
	head_size = ci_sizeof(ci_paver_obj_head_t) + ci_sizeof(ci_mem_guard_head_t);
	ci_align_upper_asg(head_size, pool->obj_align);		/* put in padding */
	alloc_size = head_size + pool->obj_size_user + ci_sizeof(ci_mem_guard_tail_t);
#else
	/*	| link | padding | user_obj | padding | 		--> head_size = link + padding */
	head_size = ci_sizeof(ci_paver_obj_head_t);
	ci_align_upper_asg(head_size, pool->obj_align);		/* put in padding */
	alloc_size = head_size + pool->obj_size_user;
#endif

	ci_align_upper_asg(alloc_size, pool->obj_align);	/* padding for tail_guard */

	pool->obj_offset = head_size;
	pool->obj_size_alloc = alloc_size; 
}

static void ci_paver_calc_bucket(ci_paver_pool_t *pool, ci_paver_cfg_t *cfg)
{
	int head_size, head_padding, tail_size, alloc_size; /* alloc_size is the buddy's actual allocation size */

#ifdef CI_BALLOC_DEBUG
	head_size = ci_sizeof(ci_mem_guard_head_t) + ci_sizeof(ci_paver_bucket_t);
	tail_size = ci_sizeof(ci_mem_guard_tail_t);
#else
	head_size = ci_sizeof(ci_paver_bucket_t);
	tail_size = 0;
#endif		

	head_padding = ci_align_upper(head_size, pool->obj_align) - head_size;
	head_size += head_padding;
	pool->bucket_obj_offset = head_padding + ci_sizeof(ci_paver_bucket_t);	/* first object located in a bucket */

	alloc_size = head_size + pool->obj_size_alloc * cfg->alloc_max + tail_size;
	alloc_size = pool->bucket_size = 1 << ci_log2_ceil(alloc_size);		/* round up to power of 2 for buddy system */
	ci_max_set(alloc_size, 1 << CI_PAVER_ALLOC_SHIFT);					/* apply the minimum alloc size */

	pool->bucket_base = pool->pn_info->ba->mem_range.start;
	pool->bucket_size = alloc_size;
	pool->bucket_obj_nr = (alloc_size - head_size - tail_size) / pool->obj_size_alloc;
	pool->bucket_mask = alloc_size - 1;							/* ptr - (ptr & mask) we get the start address of a bucket */
	pool->bucket_residue = alloc_size - head_size - tail_size - pool->obj_size_alloc * pool->bucket_obj_nr;

#ifdef CI_BALLOC_DEBUG	
	pool->bucket_size -= CI_MEM_GUARD_SIZE;		/* consider buddy's head guard */
#endif
}

static void ci_paver_calc_thr(ci_paver_pool_t *pool, ci_paver_cfg_t *cfg)
{
	pool->alloc_max		= cfg->alloc_max;
	pool->hold_min		= cfg->hold_min;
	pool->hold_max		= cfg->hold_max;
	pool->thr_avail		= cfg->thr_avail;
	pool->thr_satisfy	= cfg->thr_satisfy;
	pool->ret_lower		= cfg->ret_lower;
	pool->ret_upper		= cfg->ret_upper;

	!pool->thr_avail 	&& (pool->thr_avail 	= pool->bucket_obj_nr * CI_PAVER_MUL_AVAIL);
	!pool->thr_satisfy 	&& (pool->thr_satisfy 	= pool->bucket_obj_nr * CI_PAVER_MUL_SATISFY);
	!pool->ret_lower 	&& (pool->ret_lower 	= pool->bucket_obj_nr * CI_PAVER_MUL_RET_LOWER);
	!pool->ret_upper 	&& (pool->ret_upper 	= pool->bucket_obj_nr * CI_PAVER_MUL_RET_UPPER);
	!pool->hold_min 	&& (pool->hold_min 		= pool->bucket_obj_nr * CI_PAVER_MUL_HOLD_MIN);
	!pool->hold_max 	&& (pool->hold_max 		= pool->bucket_obj_nr * CI_PAVER_MUL_HOLD_MAX);

	ci_assert(pool->thr_avail 	> cfg->alloc_max);
	ci_assert(pool->thr_satisfy > pool->thr_avail);
	ci_assert(pool->ret_lower 	> pool->thr_satisfy);
	ci_assert(pool->ret_upper 	> pool->ret_lower);
	ci_assert(pool->hold_min 	> pool->ret_lower);
	ci_assert(pool->hold_max 	> pool->ret_upper);
}

static void ci_paver_pool_do_cfg(ci_paver_pool_t *pool, ci_paver_cfg_t *cfg)
{
	/* handle align, user size, alloc size */
	pool->obj_align = cfg->align >= PAL_CPU_ALIGN_SIZE ? cfg->align : PAL_CPU_ALIGN_SIZE;
	ci_assert(ci_is_power_of_two(pool->obj_align));
	pool->obj_size_user = cfg->size; 
	ci_assert(pool->obj_size_user > 0);
	ci_paver_calc_obj(pool);

	/* other direct copy */
	pool->init = cfg->init;
	pool->init_cookie = cfg->init_cookie;
	pool->reinit = cfg->reinit;
	pool->reinit_cookie = cfg->reinit_cookie;

	/* bucket configuration */
	ci_paver_calc_bucket(pool, cfg);

	/* thresholds */
	ci_paver_calc_thr(pool, cfg);

	/* chores */
	ci_assert(cfg->name && ci_strlen(cfg->name));
	pool->name = cfg->name;
}

static void ci_paver_pool_init(ci_paver_pool_t *pool, ci_paver_cfg_t *cfg)
{
	ci_paver_node_info_t *info;

	info = pool->pn_info;
	ci_paver_pool_do_cfg(pool, cfg);
	
	ci_slk_init(&pool->lock);
	ci_list_init(&pool->paver_head);
	ci_list_init(&pool->bucket_head);
	ci_list_init(&pool->paver_alloc_head);

	ci_slk_protected(&info->lock, ci_list_add_tail(&info->pool_head, &pool->link));	

	/* init statistics */
#ifdef CI_PAVER_STA
	ci_sta_cfg_t sta_cfg = {
		.hist				= paver_pool_sta_cfg_hist,
		.acc_size			= ci_sizeof(ci_paver_pool_sta_acc_t)
	};	

	sta_cfg.name		= pool->name;
	sta_cfg.node_id 	= pool->pn_info->node_id;
	
	pool->sta = ci_sta_create(&sta_cfg);
	
#ifdef CI_PAVER_POOL_HIST_DUMP_NAME	
	ci_sta_set_hist_dump(pool->sta, CI_PAVER_POOL_HIST_DUMP_NAME, paver_pool_dump_dpt, 1, 0);				
#endif
#endif
	
	ci_paver_pool_dump_brief(pool);
}

int ci_paver_id_by_name(const char *name)
{
	ci_paver_pool_t *pool;
	ci_paver_node_info_t *info;

	ci_assert(name && ci_strlen(name));
	ci_node_id_each(node_id) {
		info = ci_paver_node_info(node_id);
			
		ci_slk_lock(&info->lock);
		ci_list_each(&info->pool_head, pool, link) 
			if (ci_strequal(name, pool->name)) {
				ci_slk_unlock(&info->lock);
				return pool->paver_id;
			}
		ci_slk_unlock(&info->lock);
	};

	return -1;
}

int ci_paver_pool_create(ci_paver_cfg_t *cfg)
{
	int paver_id;
	ci_balloc_t	*ba;
	ci_paver_t *paver;
	ci_paver_pool_t *pool;
	ci_paver_node_info_t *info;

	ci_paver_cfg_check(cfg);
	paver_id = ci_atomic_inc_fetch(&ci_paver_info->paver_nr);	/* 0 is reserved/unused */

	ci_node_map_each_set(&cfg->node_map, node_id) {
		/* pool creating */
		info = ci_paver_node_info(node_id);
		ba = info->ba;
		pool = ci_balloc(ba, ci_sizeof(ci_paver_pool_t));
		ci_obj_zero(pool);
		
		pool->pn_info = info;
		pool->paver_id 	= paver_id;
		ci_worker_map_copy(&pool->worker_map, &cfg->worker_map[node_id]);
		ci_paver_pool_init(pool, cfg);

		/* paver creating */
		ci_worker_map_each_set(&cfg->worker_map[node_id], worker_id) {
			paver = ci_balloc(ba, ci_sizeof(ci_paver_t));
			__ci_paver_init(pool, paver, worker_id);

			/* set paver into the per worker data */
			ci_paver_t **paver_tab = ci_worker_data_by_id_not_nil(node_id, worker_id, CI_WDID_PAVER);
			ci_assert(!paver_tab[paver_id]);
			paver_tab[paver_id] = paver;
		}
	}

	return paver_id;
}

int ci_paver_pool_dump_brief(ci_paver_pool_t *pool)
{
	int rv = 0;

	rv += ci_imp_printfln("paver_pool[\"%s\"], node_id=%d, paver_id=%d, addr=%p, flag=%s", 
						   pool->name, pool->pn_info->node_id, pool->paver_id, pool, 
						   ci_flag_str(paver_pool_flag_to_name, pool->flag));

	rv += ci_printfln(CI_PR_INDENT "ba=%p, bucket_base=%p", pool->pn_info->ba, pool->bucket_base);
	rv += ci_printfln(CI_PR_INDENT "bucket { size=%d, obj_nr=%d, obj_offset=%d, residue=%d, mask=%#X }",
					  pool->bucket_size, pool->bucket_obj_nr, pool->bucket_obj_offset, pool->bucket_residue, pool->bucket_mask);
	rv += ci_printfln(CI_PR_INDENT "obj    { align=%d, offset=%d, size_user=%d, size_alloc=%d }",
					  pool->obj_align, pool->obj_offset, pool->obj_size_user, pool->obj_size_alloc);
	rv += ci_printfln(CI_PR_INDENT "alloc_max=%d, hold_min=%d, hold_max=%d, thr_avail=%d, thr_satisfy=%d, ret_lower=%d, ret_upper=%d",
					  pool->alloc_max, pool->hold_min, pool->hold_max, pool->thr_avail, pool->thr_satisfy, pool->ret_lower, pool->ret_upper);

	return rv;
}

static void ci_paver_node_res_shuffle(ci_paver_node_info_t *info)
{
	ci_list_def(inc_head);
	ci_paver_bucket_t *bucket;
	ci_paver_pool_t *free_pool, *alloc_pool;

	ci_slk_lock(&info->lock);

	if (ci_list_empty(&info->pool_no_res_head)) {
		info->flag &= ~CI_PAVER_NODEF_NO_RES;
		goto __exit;
	}

	while (!ci_list_empty(&info->pool_has_res_head) && !ci_list_empty(&info->pool_no_res_head)) {
		free_pool = ci_list_del_head_obj(&info->pool_has_res_head, ci_paver_pool_t, res_link);

		ci_slk_lock(&free_pool->lock);
		/* we do not want free buckets if itself has pending alloc requests */
		if (!free_pool->bucket_nr || !ci_list_empty(&free_pool->paver_alloc_head)) {
			free_pool->flag &= ~CI_PAVER_POOLF_Q_HAS_RES;
			ci_slk_unlock(&free_pool->lock);
			continue;		
		}

		/* free a bucket */
		bucket = ci_list_del_head_obj(&free_pool->bucket_head, ci_paver_bucket_t, link);
		ci_bfree(info->ba, bucket);
		ci_paver_pool_sta_acc_inc(free_pool, shuffle_free);
		
		if (!--free_pool->bucket_nr)	/* remove it out of has_res */
			free_pool->flag &= ~CI_PAVER_POOLF_Q_HAS_RES;
		else	
			ci_list_add_tail(&info->pool_has_res_head, &free_pool->res_link);	/* queue it to end, round-robin */
		ci_slk_unlock(&free_pool->lock);

		/* alloc pool from no_res */
		alloc_pool = ci_list_head_obj(&info->pool_no_res_head, ci_paver_pool_t, res_link);
		if (!(bucket = ci_paver_pool_bucket_balloc(alloc_pool))) {
			ci_paver_pool_sta_acc_inc(alloc_pool, shuffle_alloc_fail);
			continue;
		}

		/* clear no_res */
		ci_paver_pool_sta_acc_inc(alloc_pool, shuffle_alloc_succ);
		ci_list_del(&alloc_pool->res_link);
		alloc_pool->flag &= ~CI_PAVER_POOLF_Q_NO_RES;

		/* add bucket to pool */
		ci_slk_lock(&alloc_pool->lock);
		alloc_pool->bucket_nr++;
		ci_list_add_tail(&alloc_pool->bucket_head, &bucket->link);
		ci_slk_unlock(&alloc_pool->lock);

		ci_list_add_tail(&inc_head, &alloc_pool->tlink);
	}

__exit:
	ci_list_each_safe(&inc_head, alloc_pool, tlink) {
		ci_list_dbg_poison_set(&alloc_pool->tlink);
		ci_paver_pool_bucket_increased(alloc_pool, 0);
	}

	ci_slk_unlock(&info->lock);
}

static void ci_paver_pool_no_res(ci_paver_pool_t *pool)
{
	ci_paver_node_info_t *info = pool->pn_info;

	if (!((pool->flag & CI_PAVER_POOLF_Q_HAS_RES) || !(pool->flag & CI_PAVER_POOLF_Q_NO_RES)))
		return;

	ci_slk_lock(&info->lock);
	ci_slk_lock(&pool->lock);

	if (pool->flag & CI_PAVER_POOLF_Q_HAS_RES) {
		pool->flag &= ~CI_PAVER_POOLF_Q_HAS_RES;
		ci_list_del(&pool->res_link);
	}

	if (!(pool->flag & CI_PAVER_POOLF_Q_NO_RES)) {
		info->flag |= CI_PAVER_NODEF_NO_RES;
		pool->flag |= CI_PAVER_POOLF_Q_NO_RES;
		ci_list_add_tail(&info->pool_no_res_head, &pool->res_link);
	}
	
	ci_slk_unlock(&pool->lock);
	ci_slk_unlock(&info->lock);
}

static void ci_paver_pool_has_res(ci_paver_pool_t *pool)
{
	int res_shuffle = 0;
	ci_paver_node_info_t *info = pool->pn_info;

	if (!((pool->flag & CI_PAVER_POOLF_Q_NO_RES) || 
		  (!(pool->flag & CI_PAVER_POOLF_Q_HAS_RES) && ci_list_empty(&pool->paver_alloc_head) && pool->bucket_nr)))
		return;		

	ci_slk_lock(&info->lock);
	ci_slk_lock(&pool->lock);
	
	if (pool->flag & CI_PAVER_POOLF_Q_NO_RES) {
		pool->flag &= ~CI_PAVER_POOLF_Q_NO_RES;
		ci_list_del(&pool->res_link);
	}

	if (!(pool->flag & CI_PAVER_POOLF_Q_HAS_RES) && ci_list_empty(&pool->paver_alloc_head) && pool->bucket_nr) {
		info->flag &= ~CI_PAVER_NODEF_NO_RES;
		pool->flag |= CI_PAVER_POOLF_Q_HAS_RES;
		ci_list_add_tail(&info->pool_has_res_head, &pool->res_link);
		res_shuffle = 1;
	}

	ci_slk_unlock(&pool->lock);
	ci_slk_unlock(&info->lock);

	if (res_shuffle)
		ci_paver_node_res_shuffle(info);
}

static ci_paver_bucket_t *ci_paver_pool_bucket_balloc(ci_paver_pool_t *pool)
{
	u8 *ptr, *obj;
	ci_paver_bucket_t *bucket; 
	ci_paver_obj_head_t *obj_head;

	/* try allocate from ba */
	if (!(bucket = ci_balloc(pool->pn_info->ba, pool->bucket_size))) 
		return NULL;

	/* bucket init */
	ci_memzero(bucket, pool->bucket_size);
	ci_list_init(&bucket->obj_head);
	bucket->obj_nr = pool->bucket_obj_nr;
	
	/* add objects to bucket */
	ptr = (u8 *)bucket + pool->bucket_obj_offset;
	ci_loop(pool->bucket_obj_nr) {
		obj_head = (ci_paver_obj_head_t *)ptr;
		ci_list_add_head(&bucket->obj_head, &obj_head->link);

		obj = ptr + pool->obj_offset;
		ci_paver_mem_guard_set(obj, pool->obj_size_user, 0);	

		if (pool->init)
			pool->init(obj, pool->init_cookie);

		ptr += pool->obj_size_alloc;
	}

	/* done */
	ci_assert((u8 *)bucket + pool->bucket_size - ptr == pool->bucket_residue);
	return bucket;
}

/* alloc until hit pool->satisfy */
static void ci_paver_bucket_alloc(ci_paver_t *paver)
{
	ci_list_def(alloc_head);
	ci_paver_bucket_t *bucket;
	ci_paver_alloc_task_t *pat;
	int required, allocated, pool_allocated, buddy_required, buddy_allocated, allocated_obj;
	ci_paver_pool_t *pool = paver->pool;

	ci_assert(pool->thr_satisfy > paver->nr_free);
	ci_assert(!(paver->alloc_task.flag & CI_PAVER_ALLOC_TASKF_QUEUED));
	required = ci_div_ceil(pool->thr_satisfy - paver->nr_free + 1, pool->bucket_obj_nr);
	pool_allocated = buddy_allocated = 0;

	/* allocate from pool */
	ci_slk_protected(&pool->lock, {
		ci_loop(required) {
			if (!(bucket = ci_list_del_head_obj(&pool->bucket_head, ci_paver_bucket_t, link)))
				break;		
			ci_list_add_tail(&alloc_head, &bucket->link);
			pool->bucket_nr--, pool_allocated++;
		}

		ci_paver_dbg_extra_exec(ci_list_count_check(&pool->bucket_head, pool->bucket_nr));
		ci_paver_sta_acc_add(paver, bucket_pool_alloc, pool_allocated);
		ci_paver_pool_sta_acc_inc(pool, bucket_alloc);
	});

	/* run out of pool's bucket, we need allocate from buddy.  no need to lock, we can miss one or two occasionally */
	if ((buddy_required = required - pool_allocated) && !(pool->flag & CI_PAVER_POOLF_Q_NO_RES)) {
		if (pool->pn_info->flag & CI_PAVER_NODEF_NO_RES)	/* node out of resource, no need to call balloc */
			ci_paver_pool_no_res(pool);
		else
			ci_loop(buddy_required) {
				if (!(bucket = ci_paver_pool_bucket_balloc(pool))) {	/* let's try balloc */
					ci_paver_pool_sta_acc_inc(pool, buddy_alloc_fail);
					ci_paver_pool_no_res(pool);
					break;
				}
				
				ci_list_add_tail(&alloc_head, &bucket->link);
				buddy_allocated++;
			}

		ci_paver_pool_sta_acc_add(pool, buddy_alloc_succ, buddy_allocated);
		ci_paver_sta_acc_add(paver, bucket_buddy_alloc, buddy_allocated);
	}

	/* update paver if allocated any */
	if ((allocated = pool_allocated + buddy_allocated)) {
		ci_list_merge_head(&paver->bucket_full, &alloc_head);
		allocated_obj = allocated * pool->bucket_obj_nr;		/* bucket to objects */
		paver->nr_total += allocated_obj, paver->nr_free += allocated_obj;
		ci_paver_free_increased(paver);

		if (allocated == required)	/* we got all we want */
			return;	
	}

	/* now enough memory, queue the alloc task */
	pat = &paver->alloc_task;
	ci_assert(!pat->rest && !pat->done && ci_list_empty(&pat->bucket_head));
	pat->rest = required - allocated;
	ci_assert(pat->rest > 0);
	pat->flag |= CI_PAVER_ALLOC_TASKF_QUEUED;

	ci_slk_protected(&pool->lock, {
		ci_list_add_tail(&pool->paver_alloc_head, &paver->alloc_task.link);
	});	
}

static void ci_paver_alloc_task_notify(ci_paver_alloc_task_t *pat)
{
	ci_sched_tab_t *tab = &pat->ctx->worker->sched_tab;

	ci_slk_protected(&tab->pn_map_lock, {
		ci_paver_map_set_bit(&tab->pn_map, pat->paver_id);
	});
	
	ci_worker_soft_irq(pat->ctx->worker);	/* notify */
}

static void ci_paver_pool_bucket_increased(ci_paver_pool_t *pool, int can_free_to_node)
{
	int free_to_node;
	ci_list_t alloc_head;	/* success allocate bucket for these pavers */
	ci_paver_bucket_t *bucket;
	ci_paver_alloc_task_t *pat;

	ci_slk_lock(&pool->lock);
	ci_paver_dbg_extra_exec(ci_list_count_check(&pool->bucket_head, pool->bucket_nr));

	/* no paver is waiting for resource */
	if (ci_list_empty(&pool->paver_alloc_head) || !pool->bucket_nr) {
		free_to_node = 1;
		goto __exit;
	}

	/* round-robin the alloc task and do bucket allocation */
	ci_list_init(&alloc_head);
	while (pool->bucket_nr && !ci_list_empty(&pool->paver_alloc_head)) {
		/* get alloc task */
		pat = ci_list_del_head_obj(&pool->paver_alloc_head, ci_paver_alloc_task_t, link);
		ci_assert(pat && (pat->rest > 0));

		/* allocate bucket and add to alloc task */
		bucket = ci_list_del_head_obj(&pool->bucket_head, ci_paver_bucket_t, link);
		ci_assert(bucket && (bucket->obj_nr == pool->bucket_obj_nr));
		pool->bucket_nr--;
		ci_list_add_tail(&pat->bucket_head, &bucket->link);
		if (--pat->rest)	/* NOT all done for this one */
			ci_list_add_tail(&pool->paver_alloc_head, &pat->link);	/* put to the end, round-robin fairness */
		pat->done++;

		if (!(pat->flag & CI_PAVER_ALLOC_TASKF_ALLOCATED)) {
			pat->flag |= CI_PAVER_ALLOC_TASKF_ALLOCATED;
			ci_list_add_tail(&alloc_head, &pat->tlink);
		}

		ci_paver_pool_sta_acc_inc(pool, bucket_alloc_delayed);
	}

	/* for each allocated paver, notify resource ready */
	ci_list_each(&alloc_head, pat, tlink) {
		pat->flag &= ~CI_PAVER_ALLOC_TASKF_ALLOCATED;
		ci_paver_alloc_task_notify(pat);
	}

	/* clean up */
	ci_list_dbg_del_all(&alloc_head);
	ci_paver_dbg_extra_exec(ci_list_count_check(&pool->bucket_head, pool->bucket_nr));
	free_to_node = ci_list_empty(&pool->paver_alloc_head) && pool->bucket_nr;

__exit:	
	ci_slk_unlock(&pool->lock);

	if (free_to_node && can_free_to_node)
		ci_paver_pool_has_res(pool);
}

/* free until hit pool->ret_low */
static void ci_paver_bucket_free(ci_paver_t *paver)
{
	int bucket_nr = 0;
	ci_list_def(free_head);
	ci_paver_bucket_t *bucket;
	ci_paver_pool_t *pool = paver->pool;

	while (paver->nr_free > pool->ret_lower) {
		if (!(bucket = ci_list_del_head_obj(&paver->bucket_full, ci_paver_bucket_t, link)))
			break;		/* we do not have enough buckets in full to free */

		ci_list_add_tail(&free_head, &bucket->link);
		paver->nr_free -= pool->bucket_obj_nr;
		paver->nr_total -= pool->bucket_obj_nr;
		bucket_nr++;
	}

	if (ci_unlikely(ci_list_empty(&free_head)))
		return;		/* too many fragment, nothing to free */

	ci_paver_sta_acc_min(paver, min_free_obj, paver->nr_free);
	ci_paver_sta_acc_min(paver, min_total_obj, paver->nr_total);
	ci_paver_sta_acc_add(paver, bucket_pool_free, bucket_nr);

	ci_slk_protected(&pool->lock, {
		pool->bucket_nr += bucket_nr;
		ci_list_merge_head(&pool->bucket_head, &free_head);
		ci_paver_dbg_extra_exec(ci_list_count_check(&pool->bucket_head, pool->bucket_nr));
	});

	ci_paver_pool_bucket_increased(pool, 1);
}

static void ci_paver_free_increased(ci_paver_t *paver)
{
	ci_paver_pool_t *pool = paver->pool;

#ifdef CI_PAVER_STA		
	if (paver->flag & CI_PAVERF_INITED)
		ci_paver_sta_acc_max(paver, max_free_obj, paver->nr_free);
	else
		ci_paver_sta_acc_max_no_ctx(paver, max_free_obj, paver->nr_free);
#endif		


	/* < avail, no action needed */
	if (paver->nr_free < pool->thr_avail)	
		return;

	/* hit satisfy, we need set res paver map, unqueue sched_grp */
	if (ci_unlikely(!(paver->flag & CI_PAVERF_READY))) {
		paver->flag |= CI_PAVERF_READY;
		ci_sched_paver_ready(paver->sched_tab, pool->paver_id);

#ifdef CI_PAVER_STA		
		if (paver->flag & CI_PAVERF_INITED)
			ci_paver_sta_acc_inc(paver, ready);
		else
			ci_paver_sta_acc_inc_no_ctx(paver, ready);
#endif		
	}

	/* total < hold_min, no need to return */
	if (ci_unlikely(paver->nr_total <= pool->hold_min))
		return;
	
	/* free > upper, let's return resource */
	if (ci_unlikely(paver->nr_free >= pool->ret_upper))
		ci_paver_bucket_free(paver);
}

static void ci_paver_free_decreased(ci_paver_t *paver)
{
	ci_paver_pool_t *pool = paver->pool;

	ci_paver_sta_acc_min(paver, min_free_obj, paver->nr_free);

	if (paver->nr_free >= pool->alloc_max)		/* we still have enough resource */
		return;

	if (!(paver->flag & CI_PAVERF_READY))		/* we already set it unready and ask for memory */
		return;

	if (!(paver->alloc_task.flag & CI_PAVER_ALLOC_TASKF_QUEUED)) {
		ci_paver_bucket_alloc(paver);
		if (paver->nr_free >= pool->alloc_max)		/* allocate successfully */
			return;
	}

	ci_assert(paver->flag & CI_PAVERF_READY);
	paver->flag &= ~CI_PAVERF_READY;
	ci_sched_paver_unready(paver->sched_tab, paver->pool->paver_id);
	ci_paver_sta_acc_inc(paver, unready);
}

static void ci_paver_bucket_pool_queued_alloc_callback(ci_paver_t *paver)
{
	int nr_obj;
	ci_paver_pool_t *pool = paver->pool;
	ci_paver_alloc_task_t *pat = &paver->alloc_task;

	ci_slk_lock(&pool->lock);

	if (ci_unlikely(!pat->done)) {	/* mt window, already handled by we got notified again */
		ci_assert(ci_list_empty(&pat->bucket_head));
		ci_slk_unlock(&pool->lock);
		return;	
	}
	
	ci_assert(pat->flag & CI_PAVER_ALLOC_TASKF_QUEUED, "pat=%p, flag=%#X, rest=%d", pat, pat->flag, pat->rest);

	if (!pat->rest) 
		pat->flag &= ~CI_PAVER_ALLOC_TASKF_QUEUED;	

	ci_assert(!ci_list_empty(&pat->bucket_head));
	ci_paver_dbg_extra_exec(ci_assert(ci_list_count(&pat->bucket_head) == pat->done));	
	
	nr_obj = pat->done * pool->bucket_obj_nr;
	ci_list_merge_tail(&paver->bucket_full, &pat->bucket_head);
	paver->nr_free += nr_obj, paver->nr_total += nr_obj;
	pat->done = 0;

	ci_slk_unlock(&pool->lock);
	ci_paver_free_increased(paver);
}

void ci_paver_notify_map_check(ci_sched_ctx_t *ctx)
{
	ci_paver_t *paver;
	ci_paver_map_t map;
	ci_sched_tab_t *tab = &ctx->worker->sched_tab;

	ci_assert(ctx == ci_sched_ctx());
	ci_slk_protected(&tab->pn_map_lock, {
		ci_paver_map_copy(&map, &tab->pn_map);
		ci_paver_map_zero(&tab->pn_map);
	});

	ci_paver_map_each_set(&map, paver_id) {
		paver = ci_paver_by_ctx(ctx, paver_id);
		ci_paver_bucket_pool_queued_alloc_callback(paver);
	}
}

#ifdef CI_PAVER_DEBUG
void __ci_paver_check_reset(ci_sched_ctx_t *ctx)
{
	ci_paver_map_t *map;
	int node_id, worker_id;
	ci_paver_node_info_t *info;

	ci_node_worker_id_by_ctx(ctx, &node_id, &worker_id);
	info = ci_paver_node_info(node_id);
	map = &info->alloc_map[worker_id];
	ci_paver_map_zero(map);
}

static void __ci_paver_check_inc(ci_paver_t *paver)
{
	ci_paver_map_t *map;
	int node_id, worker_id, paver_id, alloc_nr;
	ci_paver_node_info_t *info;
	ci_sched_ctx_t *ctx = &paver->sched_tab->ctx;
	ci_sched_grp_t *grp = ctx->sched_grp;
	ci_paver_pool_t *pool = paver->pool;

	paver_id = pool->paver_id;
	ci_assert(ci_paver_map_bit_is_set(&grp->paver_map, paver_id),
			  "sched_grp \"%s\" try to allocate from paver \"%s\" without declaration",
			  grp->name, pool->name);
	
	ci_node_worker_id_by_ctx(ctx, &node_id, &worker_id);
	info = pool->pn_info;
	map = &info->alloc_map[worker_id];
	
	if (ci_unlikely(ci_paver_map_bit_is_clear(map, paver_id))) {
		ci_paver_map_set_bit(map, paver_id);
		info->alloc_cnt[worker_id][paver_id] = 0;
	}

	alloc_nr = ++info->alloc_cnt[worker_id][paver_id];
	ci_assert(alloc_nr <= pool->alloc_max, 
			  "trying to allocate more objects than the alloc_max, sched_grp=\"%s\", paver=\"%s\", alloc_max=%d",
			  grp->name, pool->name, pool->alloc_max);
}
#endif

void *__ci_palloc(ci_paver_t *paver) 
{
	u8 *obj;
	ci_paver_bucket_t *bucket;
	ci_paver_obj_head_t *obj_head;
	ci_paver_pool_t *pool = paver->pool;

	/* allocate without declaration? or allocates too much? */
	ci_paver_dbg_exec(__ci_paver_check_inc(paver));

	/* grab bucket from partial/full */
	bucket = ci_list_head_obj(&paver->bucket_partial, ci_paver_bucket_t, link);		/* try partial first */
	if (ci_unlikely(!bucket)) {
		bucket = ci_list_head_obj(&paver->bucket_full, ci_paver_bucket_t, link);	/* no partial available, get from full */
		ci_assert(bucket, "ci_paver_cfg_t.alloc_max to small?");
		ci_list_add_head(&paver->bucket_partial, ci_list_del(&bucket->link));		/* move from full to partial */
		ci_paver_sta_acc_inc(paver, full_to_partial);
	}

	/* get head object */
	ci_range_check_i(bucket->obj_nr, 1, pool->bucket_obj_nr);
	obj_head = ci_list_del_head_obj(&bucket->obj_head, ci_paver_obj_head_t, link);
	ci_assert(obj_head);
	if (ci_unlikely(!--bucket->obj_nr)) {	/* move from partial to empty (bucket_empty: just for tracking purpose) */
		ci_list_add_head(&paver->bucket_empty, ci_list_del(&bucket->link));
		ci_paver_sta_acc_inc(paver, partial_to_empty);
	} 

	/* update counter, alloc new buckets if needed */
	paver->nr_free--;
	ci_range_check(paver->nr_free, 0, paver->nr_total);
	ci_paver_free_decreased(paver);

	ci_paver_sta_acc_inc(paver, alloc);
	obj = (u8 *)obj_head + pool->obj_offset;

	if (pool->reinit)
		pool->reinit(obj, pool->reinit_cookie);

	return obj;
}

void __ci_pfree(ci_paver_t *paver, void *__obj)
{
	u8 *obj, *buddy;
	ci_paver_pool_t *pool;
	ci_paver_bucket_t *bucket;
	ci_paver_obj_head_t *obj_head;

	obj = (u8 *)__obj;
	pool = paver->pool;
	obj_head = (ci_paver_obj_head_t *)(obj - pool->obj_offset);

	/* get the bucket by offset calculation */
	buddy = obj - ((obj - (u8 *)pool->bucket_base) & pool->bucket_mask);
#ifdef CI_BALLOC_DEBUG
	buddy += ci_sizeof(ci_mem_guard_head_t);
#endif		
	bucket = (ci_paver_bucket_t *)buddy;

	/* add object to bucket */
	ci_list_add_head(&bucket->obj_head, &obj_head->link);
	ci_range_check_i(bucket->obj_nr, 0, pool->bucket_obj_nr - 1);
	bucket->obj_nr++;
	if (ci_unlikely(bucket->obj_nr == 1)) {		/* empty to partial */
		ci_list_add_head(&paver->bucket_partial, ci_list_del(&bucket->link));
		ci_paver_sta_acc_inc(paver, empty_to_partial);
	} else if (ci_unlikely(bucket->obj_nr == pool->bucket_obj_nr)) {		/* partial/empty(not likely) to full */
		ci_list_add_head(&paver->bucket_full, ci_list_del(&bucket->link));
		ci_paver_sta_acc_inc(paver, partial_to_full);
	}

	/* update counter, free the bucket if needed */	
	paver->nr_free++;
	ci_range_check_i(paver->nr_free, 1, paver->nr_total);
	ci_paver_free_increased(paver);
	ci_paver_sta_acc_inc(paver, free);
}

int ci_paver_dump_rti(ci_paver_t *paver)
{
	int rv = 0;
	ci_paver_pool_t *pool = paver->pool;
	u64 size = pool->bucket_size * (paver->nr_total / pool->bucket_obj_nr);

#define __FMT		"%-22s : "
	rv += ci_printfln(__FMT "\"%s\"", 		"pool.name", 				pool->name);
	rv += ci_printfln(__FMT "%s", 			"flag", 					ci_flag_str(paver_flag_to_name, paver->flag));
	rv += ci_printfln(__FMT "%d/%02d",		"ctx",						pool->pn_info->node_id, paver->worker_id);
	rv += ci_printfln(__FMT "%d",			"bucket.full", 				ci_list_count_safe(&paver->bucket_full));
	rv += ci_printfln(__FMT "%d",			"bucket.partial", 			ci_list_count_safe(&paver->bucket_partial));
	rv += ci_printfln(__FMT "%d",			"bucket.empty", 			ci_list_count_safe(&paver->bucket_empty));
	rv += ci_printfln(__FMT "%d",			"nr_free", 					paver->nr_free);
	rv += ci_printfln(__FMT "%d",			"nr_total", 				paver->nr_total);
	rv += ci_printfln(__FMT "%s",			"alloc.task.flag",			ci_flag_str(paver_alloc_task_flag_to_name, paver->alloc_task.flag));
	rv += ci_printfln(__FMT "%d",			"alloc.task.rest",			paver->alloc_task.rest);
	rv += ci_printfln(__FMT "%d",			"alloc.task.done",			paver->alloc_task.done);
	rv += ci_printfln(__FMT "%d",			"alloc.task.bucket_head",	ci_list_count_safe(&paver->alloc_task.bucket_head));
	rv += ci_printfln(__FMT CI_PR_BNP_FMT, 	"memory",					ci_pr_bnp_val(size));
#undef __FMT	

	return rv;

}

int ci_paver_pool_dump_rti(ci_paver_pool_t *pool, int recursive)
{
	int rv = 0;
	ci_paver_t *paver;
	u64 free_size, total_size, paver_size = 0; 

	ci_list_each(&pool->paver_head, paver, link) 
		paver_size += pool->bucket_size * (paver->nr_total / pool->bucket_obj_nr);
	free_size = pool->bucket_nr * pool->bucket_size;
	total_size = paver_size + free_size;

#define __FMT		"%-16s : "
	rv += ci_printfln(__FMT "\"%s\"", 		"name", 					pool->name);
	rv += ci_printfln(__FMT "%d", 			"node_id", 					pool->pn_info->node_id);
	rv += ci_printfln(__FMT "%d", 			"paver_id",					pool->paver_id);
	rv += ci_printfln(__FMT "%s", 			"flag", 					ci_flag_str(paver_pool_flag_to_name, pool->flag));
	rv += ci_printfln(__FMT "%d",			"bucket_nr",				pool->bucket_nr);
	rv += ci_printfln(__FMT "%d",			"paver_alloc_head",			ci_list_count_safe(&pool->paver_alloc_head));
	rv += ci_printfln(__FMT CI_PR_BNP_FMT, 	"mem_paver",				ci_pr_bnp_val(paver_size));
	rv += ci_printfln(__FMT CI_PR_BNP_FMT, 	"mem_free",					ci_pr_bnp_val(free_size));
	rv += ci_printfln(__FMT CI_PR_BNP_FMT, 	"mem_total",				ci_pr_bnp_val(total_size));
#undef __FMT	

	if (!recursive)
		return rv;

	ci_printf_info->indent += 2;
	rv += ci_printfln();
	ci_list_each(&pool->paver_head, paver, link) {
		rv += ci_paver_dump_rti(paver);
		rv += ci_printfln();
	}
	ci_printf_info->indent -= 2;

	return rv;
}
	
int ci_paver_node_info_dump_rti(ci_paver_node_info_t *info, int recursive)
{
	int rv = 0;
	ci_paver_pool_t *pool;

#define __FMT		"%-17s : "
	rv += ci_printfln(__FMT "%d", 		"node_id", 					info->node_id);
	rv += ci_printfln(__FMT "%s", 		"flag", 					ci_flag_str(paver_node_info_flag_to_name, info->flag));
	rv += ci_printfln(__FMT "%d", 		"pool_has_res_head", 		ci_list_count_safe(&info->pool_has_res_head));
	rv += ci_printfln(__FMT "%d", 		"pool_no_res_head", 		ci_list_count_safe(&info->pool_no_res_head));
#undef __FMT	

	if (!recursive)
		return rv;

	ci_printf_info->indent += 2;
	rv += ci_printfln();
	ci_list_each(&info->pool_head, pool, link) {
		rv += ci_paver_pool_dump_rti(pool, recursive);
		rv += ci_printfln();
	}
	ci_printf_info->indent -= 2;

	return rv;
}

int ci_paver_info_dump_rti(int recurisve)
{
	ci_paver_node_info_each(pni, {
		ci_paver_node_info_dump_rti(pni, recurisve);
		ci_printfln();
	});
	
	return 0;
}

int ci_paver_sta_dump_stop()
{
	int rv = 0;

#ifdef CI_PAVER_STA
	ci_sta_t *sta;
	ci_sta_acc_t *acc;
	ci_paver_t *paver;

	ci_paver_node_info_pool_each(pni, pool, {
		ci_list_each(&pool->paver_head, paver, link) {
			sta = paver->sta;
			ci_list_each(&sta->hist_head, acc, link) 
				if ((acc->flag & CI_STA_ACCF_DUMP) && !(acc->flag & CI_STA_ACCF_DUMP_PAUSE)) {
					acc->flag |= CI_STA_ACCF_DUMP_PAUSE;
					rv++;
				}
		}
	});
#endif

	return rv;
}

int ci_paver_sta_dump_continue()
{
	int rv = 0;

#ifdef CI_PAVER_STA
	ci_sta_t *sta;
	ci_sta_acc_t *acc;
	ci_paver_t *paver;

	ci_paver_node_info_pool_each(pni, pool, {
		ci_list_each(&pool->paver_head, paver, link) {
			sta = paver->sta;
			ci_list_each(&sta->hist_head, acc, link) 
				if ((acc->flag & CI_STA_ACCF_DUMP) && (acc->flag & CI_STA_ACCF_DUMP_PAUSE)) {
					acc->flag &= ~CI_STA_ACCF_DUMP_PAUSE;
					rv++;
				}
		}
	});
#endif

	return rv;
}

