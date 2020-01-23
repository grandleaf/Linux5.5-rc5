/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_paver.h					CI Paver Resource Allocator
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_sched.h"
#include "ci_sta.h"


#define CI_SWDID_PAVER							"$paver"		/* string of worker data identifier */

extern int CI_WDID_PAVER;


#ifdef CI_PAVER_DEBUG
#define ci_paver_dbg_exec(...)					ci_dbg_exec(__VA_ARGS__)
#define ci_paver_dbg_paste(...)					ci_dbg_paste(__VA_ARGS__)
#define ci_paver_check_reset(ctx)				__ci_paver_check_reset(ctx)
void __ci_paver_check_reset(ci_sched_ctx_t *ctx);

#ifdef CI_PAVER_DEBUG_EXTRA
#define ci_paver_dbg_extra_exec(...)			ci_dbg_exec(__VA_ARGS__)
#else
#define ci_paver_dbg_extra_exec(...)			ci_nop()
#endif

#else
#define ci_paver_dbg_exec(...)					ci_nop()
#define ci_paver_dbg_paste(...)					
#define ci_paver_check_reset(ctx)				ci_nop()
#define ci_paver_dbg_extra_exec(...)			ci_nop()
#endif


typedef struct __ci_paver_bucket_t 				ci_paver_bucket_t;
typedef struct __ci_paver_t 					ci_paver_t;
typedef struct __ci_paver_pool_t 				ci_paver_pool_t;
typedef struct __ci_paver_node_info_t			ci_paver_node_info_t;


/* paver configuration */
typedef struct {
	const char					*name;				/* a unique name to describe the resource */
	int							 flag;

	int							 align;				/* object alignment, if 0, will override with sizeof(void *) */
	int							 size;				/* object size */
	int							 alloc_max;			/* max possible objects alloc for one request */
	int							 hold_min;			/* min objects held by a paver */
	int							 hold_max;			/* max objects held by a paver */

	int							 thr_avail;			/* a paver is available if free obj hit this threshold (from < obj_alloc_max) */	
	int							 thr_satisfy;		/* try to charge paver to this threshold if free < obj_alloc_max */
	int							 ret_lower;			/* return objects until free == ret_lower */
	int							 ret_upper;			/* return objects if free == ret_upper */

	ci_node_map_t				 node_map;					/* nodes that this paver available */
	ci_worker_map_t				 worker_map[CI_NODE_NR];	/* workers that this paver available */

	void					   (*init)(void *obj, void *cookie);	/* object init: invoked when a new bucket from buddy system */
	void						*init_cookie;						/* second param while invoke init */
	void					 (*reinit)(void *obj, void *cookie);	/* object re-init: invoked when a new object allocates from bucket */
	void						*reinit_cookie;						/* second param while invoke re-init */
} ci_paver_cfg_t;

typedef struct {
	int							 flag;
#define CI_PAVER_ALLOC_TASKF_QUEUED			0x0001	/* task already queued into pool */
#define CI_PAVER_ALLOC_TASKF_ALLOCATED		0x0002	/* a temporary flag used by pool to indicate bucket allocated */

	int							 rest;				/* how many I want to allocate */
	int							 done;				/* how many allocated */
	ci_list_t					 bucket_head;		/* bucket head, get buckets from pool */

	int							 paver_id;			/* paver->pool->paver_id */
	ci_sched_ctx_t				*ctx;				/* schedule context */
	
	ci_list_t					 link;				/* chain into pool's alloc_head */
	ci_list_t					 tlink;				/* a temporary link used by pool */
} ci_paver_alloc_task_t;

/* external meta data of each object */
typedef struct {
	ci_list_t					 link;				/* chain */

#ifdef CI_PAVER_DEBUG
	int							 __flag;			/* debug flags */
#define CI_PAVER_OBJ_ALLOC			0x0001			/* allocated */

	int							 __paver_id;		/* set in alloc, check in free */
	ci_sched_ctx_t 				*__ctx;				/* for context tracking */
	ci_access_tag_t				 __alloc_tag;		/* access tag for alloction */
	ci_access_tag_t				 __free_tag;		/* access tag for alloction */
#endif
} ci_paver_obj_head_t;

struct __ci_paver_bucket_t {
	int							 obj_nr;			/* how many objects in the bucket */
	ci_list_t					 obj_head;			/* chain all the data */
	ci_list_t					 link;				/* link to parent's head */
};


#ifdef CI_PAVER_STA
typedef struct {
	u64							 alloc;					/* alloc call */
	u64							 free;					/* free call */
	u64							 ready;					/* paver becomes ready */
	u64							 unready;				/* paver becomes unready */
	u64							 full_to_partial;		/* bucket status change */
	u64							 partial_to_full;
	u64							 partial_to_empty;
	u64							 empty_to_partial;

	u64							 bucket_pool_alloc;		/* allocate buckets from pool */
	u64							 bucket_pool_free;		/* free buckets to pool */
	u64							 bucket_buddy_alloc;	/* free buckets to pool */
	u64							 bucket_buddy_free;		/* free buckets to pool */

	u64							 max_free_obj;			/* for objects */
	u64							 min_free_obj;			
	u64						  	 max_total_obj;			
	u64							 min_total_obj;			
} ci_paver_sta_acc_t;

typedef struct {
	u64							 shuffle_free;			/* release resource to buddy system */
	u64							 shuffle_alloc_succ;	/* allocate success from buddy system in shuffle function */
	u64							 shuffle_alloc_fail;	/* allocate fail from buddy system in shuffle function */

	u64							 buddy_alloc_succ;		/* allocate bucket directly from buddy system successfully */
	u64							 buddy_alloc_fail;		/* allocate bucket directly from buddy system failed */
	
	u64							 bucket_alloc;			/* bucket alloc from the internal bucket pool */
	u64							 bucket_alloc_delayed;	/* pool get resource, and alloc bucket for queued alloc tasks */
} ci_paver_pool_sta_acc_t;	
#endif	/* ! CI_PAVER_STA */


struct __ci_paver_t {
	int							 flag;
#define CI_PAVERF_INITED			0x0001			/* the paver has been initialized and charged */
#define CI_PAVERF_READY				0x0002			/* hit satisfy, read to allocate, clear when hit 0 */

	int							 worker_id;			/* belong to which worker */
	ci_paver_pool_t				*pool;
	ci_sched_tab_t				*sched_tab;			/* needs access res & run */

	ci_list_t					 bucket_full;		/* headers for full, partial, empty buckets */
	ci_list_t					 bucket_partial;
	ci_list_t					 bucket_empty;

	int							 nr_free;			/* how many free objects */
	int							 nr_total;			/* total objects: full + partial + empty */

	ci_paver_alloc_task_t		 alloc_task;		/* cannot grab bucket from pool, queue me into pool */
	ci_list_t					 link;				/* chained to pool's paver_head */
	
#ifdef CI_PAVER_STA
	ci_sta_t				 	*sta;
#endif
};

struct __ci_paver_pool_t {
	const char					*name;				/* copy from pool_cfg's name */
	int							 paver_id;			/* system wide [1, N) id */
	int							 flag;
#define CI_PAVER_POOLF_Q_NO_RES			0x0001		/* no resource queued in pt_info */
#define CI_PAVER_POOLF_Q_HAS_RES		0x0002		/* has resource queued in pt_info */

	void						*bucket_base;		/* ba->mem_range.start */
	int							 bucket_size;		/* allocation size for bucket */
	int							 bucket_obj_nr;		/* how many objects in a bucket */
	int							 bucket_obj_offset;	/* first object offset in a bucket */
	int							 bucket_mask;		/* mask obj in bucket offset */
	int							 bucket_residue;	/* unused space */

	int							 obj_align;			/* object alignment */
	int							 obj_offset;		/* object offset in "size alloc" */
	int							 obj_size_user;		/* user asked size */
	int							 obj_size_alloc;	/* actually allocated size */
	
	int							 alloc_max;			/* max possible objects alloc for one request */
	int							 hold_min;			/* min objects held by a paver */
	int							 hold_max;			/* max objects held by a paver */
	int							 thr_avail;			/* a paver is available if free obj hit this threshold (from < obj_alloc_max) */	
	int							 thr_satisfy;		/* try to charge paver to this threshold if free < obj_alloc_max */
	int							 ret_lower;			/* return objects until free == ret_lower */
	int							 ret_upper;			/* return objects if free == ret_upper */

	ci_list_t				 	 paver_head;		/* chain children ci_paver_t */
	ci_worker_map_t				 worker_map;		/* copy from ci_paver_init_cfg_t */

	int							 bucket_nr;			/* how many buckets in bucket_head */
	ci_list_t					 bucket_head;		/* paver return/alloc buckets to/from here */

	void					   (*init)(void *obj, void *cookie);	/* object init: invoked when a new bucket from buddy system */
	void						*init_cookie;						/* second param while invoke init */
	void					 (*reinit)(void *obj, void *cookie);	/* object re-init: invoked when a new object allocates from bucket */
	void						*reinit_cookie;						/* second param while invoke re-init */

	ci_slk_t					 lock;				/* shared by paver_t, use lock to protect */
	ci_list_t					 paver_alloc_head;	/* chain paver's alloc_task */
	ci_list_t					 link;				/* chain pools into paver node info */

	ci_paver_node_info_t		*pn_info;			/* pointer to paver_node_info */
	ci_list_t					 res_link;			/* chain pools into pn_info's no_res/has_res head */ 
	ci_list_t					 tlink;				/* a temporary link used by node_res_shuffle */

#ifdef CI_PAVER_STA
	ci_sta_t				 	*sta;
#endif
};

struct __ci_paver_node_info_t {
	int							 node_id;			
	int							 flag;
#define CI_PAVER_NODEF_NO_RES			0x0001		/* no resource */	


	ci_balloc_t					*ba;				/* buddy system for paver */
	ci_list_t					 pool_head;			/* chain all the pools belongs to this node info */
	ci_list_t					 pool_has_res_head;	/* chain all the pools has spare buckets */
	ci_list_t					 pool_no_res_head;	/* chain all the pools want allocate buckets from ba */
	ci_slk_t					 lock;
	
#ifdef CI_PAVER_DEBUG
	int							 alloc_cnt[CI_WORKER_NR][CI_PAVER_NR];	/* check if counter > alloc_max */
	ci_paver_map_t				 alloc_map[CI_WORKER_NR];				/* check if user try to allocate resource without declaration */
#endif	
};

typedef struct {
	volatile int				 paver_nr;				/* use this to assign paver id */
	ci_paver_node_info_t		 pn_info[CI_NODE_NR];	/* buddy allocator ... */
} ci_paver_info_t;


#define ci_paver_node_info(node_id)		\
	({	\
		ci_assert(node_id < ci_node_info->nr_node);	\
		&ci_paver_info->pn_info[node_id];	\
	})
#define ci_paver_by_ctx(ctx, paver_id)		\
	({	\
		ci_paver_t **__paver_tab__ = ci_worker_data_by_ctx_not_nil(ctx, CI_WDID_PAVER);		\
		ci_range_check(paver_id, 1, CI_PAVER_NR);		\
		ci_assert(__paver_tab__[paver_id]);		\
		__paver_tab__[paver_id];		\
	})
#define ci_paver_pool_by_node_id(node_id, __paver_id)	\
	({	\
		ci_paver_pool_t *__paver_pool__, *__paver_pool_rv__ = NULL;	\
		ci_paver_node_info_t *__paver_node_info__ = ci_paver_node_info(node_id);	\
		ci_list_each(&__paver_node_info__->pool_head, __paver_pool__, link)		\
			if (__paver_pool__->paver_id == __paver_id) {	\
				__paver_pool_rv__ = __paver_pool__;	\
				break;	\
			}	\
		ci_assert(__paver_pool_rv__);	\
		__paver_pool_rv__;	\
	})
#define ci_paver_node_info_each(pni, ...)	\
	ci_node_id_each(__node_id__) {	\
		ci_paver_node_info_t *pni = ci_paver_node_info(__node_id__);	\
		__VA_ARGS__;	\
	}
#define ci_paver_node_info_pool_each(pni, pool, ...)	\
	ci_paver_node_info_each(pni, {	\
		ci_paver_pool_t *pool;	\
		ci_list_each(&pni->pool_head, pool, link) {	\
			__VA_ARGS__;	\
		}	\
	})

#ifdef CI_PAVER_DEBUG
#define ci_paver_mem_guard_set(addr, user_size, offset)		\
	do {	\
		u8 *__guard_start__ = (u8 *)(addr) + (offset);	\
		u8 *__guard_end__ = __guard_start__ + (user_size);	\
		ci_mem_guard_intl_set(__guard_start__, __guard_end__);	\
	} while (0)
#define ci_paver_mem_guard_check(addr, user_size, offset)		\
	do {	\
		u8 *__guard_start__ = (u8 *)(addr) + (offset);	\
		u8 *__guard_end__ = __guard_start__ + (user_size);	\
		ci_mem_guard_intl_check(__guard_start__, __guard_end__);	\
	} while (0)
#define ci_palloc(ctx, paver_id)		\
	({	\
		ci_paver_t *__palloc_paver__ = ci_paver_by_ctx(ctx, paver_id);		\
		void *__palloc_obj__ = __ci_palloc(__palloc_paver__);	\
		ci_paver_obj_head_t *__palloc_obj_head__ = (ci_paver_obj_head_t *)((u8 *)__palloc_obj__ - __palloc_paver__->pool->obj_offset);	\
		\
		ci_assert(!(__palloc_obj_head__->__flag & CI_PAVER_OBJ_ALLOC), "paver bug: double allocation!");	\
		\
		/* context check & set */		\
		ci_assert(ci_sched_ctx() == (ctx), "ci_palloc() invoked in a wrong context");	\
		ci_assert((ctx) == &__palloc_paver__->sched_tab->ctx);	\
		__palloc_obj_head__->__ctx = ctx;	\
		__palloc_obj_head__->__paver_id = paver_id;	\
		__palloc_obj_head__->__flag |= CI_PAVER_OBJ_ALLOC;	\
		__ci_access_tag_set(&__palloc_obj_head__->__alloc_tag);	\
		\
		/* someone corrupted the data, two asserts are the same */		\
		ci_paver_mem_guard_check(__palloc_obj__, __palloc_paver__->pool->obj_size_user, 0);	\
		ci_paver_mem_guard_check(__palloc_obj_head__, __palloc_paver__->pool->obj_size_user, __palloc_paver__->pool->obj_offset);	\
		\
		ci_align_check(__palloc_obj__, __palloc_paver__->pool->obj_align);		\
		__palloc_obj__;	\
	})
#define ci_pfree(ctx, paver_id, obj)	\
	do {	\
		ci_paver_t *__pfree_paver__ = ci_paver_by_ctx(ctx, paver_id);		\
		ci_paver_obj_head_t *__pfree_obj_head__ = (ci_paver_obj_head_t *)((u8 *)(obj) - __pfree_paver__->pool->obj_offset);	\
		ci_sched_ctx_t *__pfree_alloc_ctx__ = __pfree_obj_head__->__ctx;	\
		\
		if (!(__pfree_obj_head__->__flag & CI_PAVER_OBJ_ALLOC)) {	\
			ci_printfln("paver double free detected, paver_name=\"%s\", paver_id=%d, obj=%p", \
						__pfree_paver__->pool->name, paver_id, obj);	\
			ci_printf("last alloc : " CI_PR_ACCESS_TAG_FMT, ci_pr_access_tag_val(&__pfree_obj_head__->__alloc_tag));	\
			ci_printf("last free  : " CI_PR_ACCESS_TAG_FMT, ci_pr_access_tag_val(&__pfree_obj_head__->__free_tag));	\
			ci_bug();	\
		};	\
		__pfree_obj_head__->__flag &= ~CI_PAVER_OBJ_ALLOC;	\
		\
		/* context check */		\
		ci_assert(ci_sched_ctx() == (ctx), "ci_pfree() invoked in a wrong context");	\
		ci_assert((ctx) == &__pfree_paver__->sched_tab->ctx);	\
		ci_assert((ctx) == __pfree_alloc_ctx__, "paver alloc ctx != free ctx, alloc_ctx=%d/%02d, free_ctx=%d/%02d",	\
				  (__pfree_alloc_ctx__)->worker->node_id, (__pfree_alloc_ctx__)->worker->worker_id, 	\
				  (ctx)->worker->node_id, (ctx)->worker->worker_id);	\
		ci_assert((paver_id) == __pfree_obj_head__->__paver_id, "paver alloc paver_id != free paver_id,"	\
				  "alloc paver_id=%d, free paver_id=%d", __pfree_obj_head__->__paver_id, paver_id);	\
		__pfree_obj_head__->__ctx = NULL;	\
		\
		/* return obj pointer check */	\
		ci_align_check(obj, __pfree_paver__->pool->obj_align, "ci_pfree() got a invalid obj pointer:%p", obj);		\
		ci_mem_range_check(obj, &__pfree_paver__->pool->pn_info->ba->mem_range, \
						   "ci_pfree() got a invalid obj pointer %p out of memory range [ %p, %p ), pool_name=\"%s\"", 	\
						   obj, __pfree_paver__->pool->pn_info->ba->mem_range.start, 	\
						   __pfree_paver__->pool->pn_info->ba->mem_range.end,	\
						   __pfree_paver__->pool->name);	\
		\
		/* someone corrupted the data, two asserts are the same */		\
		ci_paver_mem_guard_check(obj, __pfree_paver__->pool->obj_size_user, 0);	\
		ci_paver_mem_guard_check(__pfree_obj_head__, __pfree_paver__->pool->obj_size_user, __pfree_paver__->pool->obj_offset);	\
		\
		__ci_access_tag_set(&__pfree_obj_head__->__free_tag);	\
		__ci_pfree(__pfree_paver__, obj);	\
		(obj) = NULL;	\
	} while (0)
#else
#define ci_paver_mem_guard_set(addr, user_size, offset)			ci_nop()
#define ci_paver_mem_guard_check(addr, user_size, offset)		ci_nop()

#define ci_palloc(ctx, paver_id)								__ci_palloc(ci_paver_by_ctx(ctx, paver_id))
#define ci_pfree(ctx, paver_id, obj)							__ci_pfree(ci_paver_by_ctx(ctx, paver_id), obj)
#endif


#ifdef CI_PAVER_STA
/* for paver, no_ctx version for init */
#define ci_paver_sta_acc_add(paver, name, val)			ci_sta_acc_add((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), val)		
#define ci_paver_sta_acc_inc(paver, name)				ci_paver_sta_acc_add(paver, name, 1)
#define ci_paver_sta_acc_inc_no_ctx(paver, name)		ci_sta_acc_add_no_ctx((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), 1)		

#define ci_paver_sta_acc_max(paver, name, val)			ci_sta_acc_max((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), val)
#define ci_paver_sta_acc_max_no_ctx(paver, name, val)	ci_sta_acc_max_no_ctx((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), val)

#define ci_paver_sta_acc_min(paver, name, val)			ci_sta_acc_min((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), val)
#define ci_paver_sta_acc_min_no_ctx(paver, name, val)	ci_sta_acc_min_no_ctx((paver)->sta, ci_offset_of(ci_paver_sta_acc_t, name), val)

/* for paver_pool, all without context */
#define ci_paver_pool_sta_acc_add(pool, name, val)		ci_sta_acc_add_no_ctx((pool)->sta, ci_offset_of(ci_paver_pool_sta_acc_t, name), val)
#define ci_paver_pool_sta_acc_inc(pool, name)			ci_paver_pool_sta_acc_add(pool, name, 1)
#else
#define ci_paver_sta_acc_add(paver, name, val)			ci_nop()
#define ci_paver_sta_acc_inc(paver, name)				ci_nop()
#define ci_paver_sta_acc_inc_no_ctx(paver, name)		ci_nop()

#define ci_paver_sta_acc_max(paver, name, val)			ci_nop()
#define ci_paver_sta_acc_max_no_ctx(paver, name, val)	ci_nop()

#define ci_paver_sta_acc_min(paver, name, val)			ci_nop()
#define ci_paver_sta_acc_min_no_ctx(paver, name, val)	ci_nop()

#define ci_paver_pool_sta_acc_add(pool, name, val)		ci_nop()
#define ci_paver_pool_sta_acc_inc(pool, name)			ci_nop()
#endif

		

/*
 *	exported functions
 */	
int  ci_paver_init();
int  ci_paver_pool_create(ci_paver_cfg_t *cfg);
int  ci_paver_pool_dump_brief(ci_paver_pool_t *pool);
void ci_paver_notify_map_check(ci_sched_ctx_t *ctx);
int  ci_paver_id_by_name(const char *name);

int  ci_paver_dump_rti(ci_paver_t *paver);
int  ci_paver_pool_dump_rti(ci_paver_pool_t *pool, int recursive);
int  ci_paver_node_info_dump_rti(ci_paver_node_info_t *info, int recursive);
int  ci_paver_info_dump_rti(int recurisve);

int  ci_paver_sta_dump_stop();
int  ci_paver_sta_dump_continue();

/*
 *	internal use only
 */
void *__ci_palloc(ci_paver_t *paver); 
void __ci_pfree(ci_paver_t *paver, void *obj);


