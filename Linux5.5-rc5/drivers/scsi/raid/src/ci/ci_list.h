/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_list.h					CI Double fielded List
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_macro.h"
#include "ci_printf.h"
#include "ci_type.h"
#include "ci_util.h"

#ifdef CI_LIST_DEBUG
#define ci_list_assert(x, ...)						ci_assert(x, __VA_ARGS__)
#define ci_list_dbg_exec(...)						ci_dbg_exec(__VA_ARGS__)
#define ci_list_dbg_del_all(head)					ci_list_del_all(head)
#else
#define ci_list_assert(x, ...)						ci_nop()
#define ci_list_dbg_exec(...)						ci_nop()
#define ci_list_dbg_del_all(head)					ci_nop()
#endif

#ifdef CI_CLIST_COUNT_CHECK		/* caution: very slow */
#define ci_clist_count_check(chead)			ci_assert((chead)->count == ci_list_count(&(chead)->list))	
#else
#define ci_clist_count_check(chead)			ci_nop()
#endif

/*
 *	list definitions
 */
typedef struct __ci_list_t {
	struct __ci_list_t					*prev;
	struct __ci_list_t					*next;

	ci_access_tag;
} ci_list_t;

typedef struct {	/* holds a *data pointer */
	void								*data;
	ci_list_t							 link;
} ci_data_list_t;
	


/*
 *	basic operations 
 */
#define ci_list_def(head)								ci_list_t head = { .prev = &(head), .next = &(head) }
#define ci_list_init(head)								({(head)->next = (head)->prev = (head); (head); })
#define ci_list_empty(head)								((head)->next == (head))

#define ci_list_head(head)								(ci_unlikely(ci_list_empty(head)) ? NULL : (head)->next)
#define ci_list_tail(head)								(ci_unlikely(ci_list_empty(head)) ? NULL : (head)->prev)
#define ci_list_prev(head, ent)							(ci_unlikely((head) == (ent)->prev) ? NULL : (ent)->prev)
#define ci_list_next(head, ent)							(ci_unlikely((head) == (ent)->next) ? NULL : (ent)->next)

#define ci_list_add_head(head, ent)						ci_list_add_after(head, ent)
#define ci_list_add_tail(head, ent)						ci_list_add_before(head, ent)
#define ci_list_del_head(head)							(ci_unlikely(ci_list_empty(head)) ? NULL : ci_list_del((head)->next))
#define ci_list_del_tail(head)							(ci_unlikely(ci_list_empty(head)) ? NULL : ci_list_del((head)->prev))

#define ci_list_err_msg(ent, msg)		\
	"%s"	\
	"\n            ptr : %p"	\
	"\n           prev : %p"	\
	"\n           next : %p"	\
	"\n      prev_file : %s"	\
	"\n      prev_func : %s()"	\
	"\n      prev_line : %d"	\
	"\n prev_timestamp : %#llX", 	\
	msg, (ent), (ent)->prev, (ent)->next, (ent)->__access_tag.file, (ent)->__access_tag.func,	\
	(ent)->__access_tag.line, (ent)->__access_tag.timestamp

/* check macros */
#define ci_list_dbg_poison_set(e)		\
	ci_list_dbg_exec(	\
		(e)->prev = (ci_list_t *)CI_POISON_LIST_PREV;		\
		(e)->next = (ci_list_t *)CI_POISON_LIST_NEXT;		\
	)
#define ci_list_dbg_add_check(e)		\
	do {	\
		ci_list_assert(ci_ptr_is_uninited((e)->prev) || ((e)->prev == (ci_list_t *)CI_POISON_LIST_PREV),	\
					   ci_list_err_msg(e, "LIST DOUBLE ADD"));		\
		ci_list_assert(ci_ptr_is_uninited((e)->next) || ((e)->next == (ci_list_t *)CI_POISON_LIST_NEXT),	\
					   ci_list_err_msg(e, "LIST DOUBLE ADD"));		\
	} while (0)
#define ci_list_dbg_del_check(e)		\
	do {	\
		ci_list_assert(!ci_ptr_is_uninited((e)->prev) && ((e)->prev != (ci_list_t *)CI_POISON_LIST_PREV),	\
					   ci_list_err_msg(e, "LIST DOUBLE DEL"));		\
		ci_list_assert(!ci_ptr_is_uninited((e)->next) && ((e)->next != (ci_list_t *)CI_POISON_LIST_NEXT),	\
					   ci_list_err_msg(e, "LIST DOUBLE DEL"));		\
	} while (0)

#define ci_list_add_after(prev_e, new_e)		\
	({		\
		ci_list_t *__add_after_prev_e__ = (prev_e), *__add_after_new_e__ = (new_e);		\
		ci_list_dbg_add_check(__add_after_new_e__);	\
		\
		__add_after_new_e__->prev = __add_after_prev_e__;		\
		__add_after_new_e__->next = __add_after_prev_e__->next;		\
		__add_after_prev_e__->next = __add_after_new_e__;		\
		__add_after_new_e__->next->prev = __add_after_new_e__;		\
		\
		ci_list_dbg_exec(ci_access_tag_set(__add_after_new_e__));		\
		__add_after_new_e__;		\
	})

#define ci_list_add_before(next_e, new_e)		\
	({		\
		ci_list_t *__add_before_next_e__ = (next_e), *__add_before_new_e__ = (new_e);		\
		ci_list_dbg_add_check(__add_before_new_e__);	\
		\
		__add_before_new_e__->prev = __add_before_next_e__->prev;		\
		__add_before_new_e__->next = __add_before_next_e__;		\
		__add_before_new_e__->prev->next = __add_before_new_e__;		\
		__add_before_next_e__->prev = __add_before_new_e__;		\
		\
		ci_list_dbg_exec(ci_access_tag_set(__add_before_new_e__));		\
		__add_before_new_e__;		\
	})

#define ci_list_del(ent)		\
	({		\
		ci_list_t *__del_ent__ = (ent);		\
		ci_list_dbg_del_check(__del_ent__);		\
		\
		__del_ent__->prev->next = __del_ent__->next;		\
		__del_ent__->next->prev = __del_ent__->prev;		\
		\
		ci_list_dbg_poison_set(__del_ent__);		\
		ci_list_dbg_exec(ci_access_tag_set(__del_ent__));		\
		__del_ent__;		\
	})


/*
 *	basic operations for obj
 */
#define ci_list_head_obj(head, type, field)				\
			(ci_unlikely(ci_list_empty(head)) ? NULL : ci_container_of((head)->next, type, field))
#define ci_list_tail_obj(head, type, field)				\
			(ci_unlikely(ci_list_empty(head)) ? NULL : ci_container_of((head)->prev, type, field))
#define ci_list_prev_obj(head, obj, field)		\
			(ci_unlikely((head) == (obj)->field.prev) ? NULL : ci_container_of((obj)->field.prev, ci_typeof(*(obj)), field))
#define ci_list_next_obj(head, obj, field)		\
			(ci_unlikely((head) == (obj)->field.next) ? NULL : ci_container_of((obj)->field.next, ci_typeof(*(obj)), field))
#define ci_list_del_head_obj(head, type, field)			\
			(ci_unlikely(ci_list_empty(head)) ? NULL : ci_container_of(ci_list_del((head)->next), type, field))
#define ci_list_del_tail_obj(head, type, field)			\
			(ci_unlikely(ci_list_empty(head)) ? NULL : ci_container_of(ci_list_del((head)->prev), type, field))

#define ci_list_head_obj_not_nil(head, type, field)		\
	({	\
		ci_assert(!ci_list_empty(head));		\
		ci_container_of((head)->next, type, field);		\
	})


/*
 *	iterators for list entries, safe version support del in iterating 
 */
#define ci_list_each_ent(head, ent)		\
	for ((ent) = (head)->next; (ent) != (head); (ent) = (ent)->next)
#define ci_list_each_ent_reverse(head, ent)		\
    for ((ent) = (head)->prev; (ent) != (head); (ent) = (ent)->prev)
#define ci_list_each_ent_safe(head, ent)	\
    for (ci_list_t *__tmp__ = ((ent) = (head)->next)->next; (ent) != (head); (ent) = __tmp__, __tmp__ = __tmp__->next)
#define ci_list_each_ent_with_index(head, ent, idx)		\
	for (int idx = !((ent) = (head)->next); (ent) != (head); (ent) = (ent)->next, idx++)

#define ci_list_each_ent_reverse_safe(head, ent)	\
    for (ci_list_t *__tmp__ = ((ent) = (head)->prev)->prev; (ent) != (head); (ent) = __tmp__, __tmp__ = __tmp__->prev)
#define ci_list_each_ent_reverse_with_index(head, ent, idx)		\
    for (int idx = !((ent) = (head)->prev); (ent) != (head); (ent) = (ent)->prev, idx++)
#define ci_list_each_ent_safe_with_index(head, ent, idx, ...)	\
	do {	\
		int idx = 0;		\
		ci_list_each_ent_safe(head, ent) {	\
			{ __VA_ARGS__; }		\
			idx++;	\
		} \
	} while (0)
#define ci_list_each_ent_reverse_safe_with_index(head, ent, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_list_each_ent_reverse_safe(head, ent) {	\
			{ __VA_ARGS__; }		\
			idx++;	\
		} \
	} while (0)


/*
 *	iterators for list objects, safe version support del in iterating 
 */
#define ci_list_each(head, obj, field)		\
	for ((obj) = ci_container_of((head)->next, ci_typeof(*(obj)), field);	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of((obj)->field.next, ci_typeof(*(obj)), field))
#define ci_list_each_reverse(head, obj, field)		\
	for ((obj) = ci_container_of((head)->prev, ci_typeof(*(obj)), field);	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of((obj)->field.prev, ci_typeof(*(obj)), field))
#define ci_list_each_safe(head, obj, field)		\
	for (ci_list_t *__tmp__ = ((obj) = ci_container_of((head)->next, ci_typeof(*(obj)), field))->field.next;	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of(__tmp__, ci_typeof(*(obj)), field), __tmp__ = __tmp__->next)
#define ci_list_each_with_index(head, obj, field, idx)		\
	for (int idx = !((obj) = ci_container_of((head)->next, ci_typeof(*(obj)), field));	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of((obj)->field.next, ci_typeof(*(obj)), field), idx++)

#define ci_list_each_reverse_safe(head, obj, field)		\
	for (ci_list_t *__tmp__ = ((obj) = ci_container_of((head)->prev, ci_typeof(*(obj)), field))->field.prev;	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of(__tmp__, ci_typeof(*(obj)), field), __tmp__ = __tmp__->prev)
#define ci_list_each_reverse_with_index(head, obj, field, idx)		\
	for (int idx = !((obj) = ci_container_of((head)->prev, ci_typeof(*(obj)), field));	\
		 &(obj)->field != (head);	\
		 (obj) = ci_container_of((obj)->field.prev, ci_typeof(*(obj)), field), idx++)
#define ci_list_each_safe_with_index(head, obj, field, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_list_each_safe(head, obj, field)	{	\
			{ __VA_ARGS__; }		\
			idx++;		\
		}	\
	} while (0)
#define ci_list_each_reverese_safe_with_index(head, obj, field, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_list_each_reverse_safe(head, obj, field)	{	\
			{ __VA_ARGS__; }		\
			idx++;		\
		}	\
	} while (0)

/* merge list src into list dst's tail */
#define ci_list_merge_tail(dst, src)		\
	({		\
		ci_list_t *__dst__ = (dst), *__src__ = (src);		\
		\
		if (ci_likely(!ci_list_empty(__src__))) {		\
			__src__->next->prev = __dst__->prev;		\
			__dst__->prev->next = __src__->next;		\
			__src__->prev->next = __dst__;		\
			__dst__->prev = __src__->prev;		\
			ci_list_init(__src__);		\
		}		\
		\
		__dst__;		\
	})

/* merge list src into list dst's head */
#define ci_list_merge_head(dst, src)		\
	({		\
		ci_list_t *__dst__ = (dst), *__src__ = (src);		\
		\
		if (ci_likely(!ci_list_empty(__src__))) {		\
			__src__->prev->next = __dst__->next;		\
			__dst__->next->prev = __src__->prev;		\
			__src__->next->prev = __dst__;		\
			__dst__->next = __src__->next;		\
			ci_list_init(__src__);		\
		}		\
		\
		__dst__;		\
	})

/* same as ci_list_merge_tail(), except dst will be initialized */
#define ci_list_move(dst, src)		\
	({		\
		ci_list_t *__move_dst__ = (dst), *__move_src__ = (src);		\
		\
		ci_list_init(__move_dst__);		\
		ci_list_merge_tail(__move_dst__, __move_src__);		\
		__move_dst__;		\
	})

/* get number of entries in the list.  warning: slow, use with caution */
static inline int ci_list_count(ci_list_t *head)
{
	int count = 0;
	ci_list_t *ent;

	ci_list_each_ent(head, ent)
		count++;
	return count;
}

/* a safe counter function for multi-thread, it's locking free, so might be imprecise  */
static inline int ci_list_count_safe(ci_list_t *head)
{
	int count = 0;
	ci_list_t *ent;

	ci_list_each_ent(head, ent) {
		if (ci_ptr_is_uninited(ent) || ((u64)ent == CI_POISON_LIST_NEXT)) {
			count = -count;
			break;
		}
		
		count++;
	}

	return count;
}

/* check list count */
#define ci_list_count_check(head, count)	ci_assert(ci_list_count(head) == (count))

/* forward the ent n steps */
#define ci_list_forward(head, ent, n)		\
	({	\
		ci_loop(n) {	\
			(ent) = (ent)->next;	\
			ci_assert((ent) != ci_list_head(head));	\
		}	\
		(ent);	\
	})
	
/* backward the ent n steps */
#define ci_list_backward(head, ent, n)		\
	({	\
		ci_loop(n) {	\
			(ent) = (ent)->prev;	\
			ci_assert((ent) != ci_list_head(head));	\
		}	\
		(ent);	\
	})	
	
/* get entity in position n, index start from 0 */
#define ci_list_ent_n(head, n)		\
	({	\
		ci_list_t *__ent_n_head__ = (head), *__ent_n__ = ci_list_head(__ent_n_head__);		\
		ci_assert(__ent_n__);		\
		ci_list_forward(__ent_n_head__, __ent_n__, n);	\
	})

/* get entity in position n, index start from 0 */
#define ci_list_ent_backward_n(head, n)		\
	({	\
		ci_list_t *__ent_n_head__ = (head), *__ent_n__ = ci_list_tail(__ent_n_head__);		\
		ci_assert(__ent_n__);		\
		ci_list_backward(__ent_n_head__, __ent_n__, n);	\
	})
	
/* split a list, new_head holds entities from [0, ent), ori_head holds entities from [ent, ...) */
#define ci_list_split_ori_ent(ori_head, new_head, ent)		\
	({	\
		ci_list_t *__split_ori_head__ = (ori_head), *__split_new_head__ = (new_head), *__split_ent__ = (ent);	\
		\
		ci_assert(!ci_list_empty(__split_ori_head__));		\
		__split_new_head__->next = __split_ori_head__->next;	\
		__split_new_head__->next->prev = __split_new_head__;	\
		__split_new_head__->prev = __split_ent__->prev;	\
		__split_new_head__->prev->next = __split_new_head__;	\
		\
		__split_ori_head__->next = __split_ent__;	\
		__split_ent__->prev = __split_ori_head__;	\
		\
		__split_new_head__;	\
	})

	
/* split a list, new_head holds entities from [ent, ...), ori_head holds entities from [0, ent) */
#define ci_list_split_new_ent(ori_head, new_head, ent)		\
	({	\
		ci_list_t *__split_ori_head__ = (ori_head), *__split_new_head__ = (new_head), *__split_ent__ = (ent);	\
		\
		ci_assert(!ci_list_empty(__split_ori_head__));		\
		__split_new_head__->next = __split_ent__;	\
		__split_new_head__->prev = __split_ori_head__->prev;	\
		__split_new_head__->prev->next = __split_new_head__;	\
		\
		__split_ori_head__->prev = __split_ent__->prev;	\
		__split_ori_head__->prev->next = __split_ori_head__;	\
		\
		__split_ent__->prev = __split_new_head__;	\
		\
		__split_new_head__;	\
	})

/* similar to ci_list_split_ori_ent(), but use a index n */
#define ci_list_split_ori_n(ori_head, new_head, n)		\
	({	\
		ci_list_t *__split_n_head__ = (ori_head);	\
		ci_list_t *__split_n_ent__ = ci_list_ent_n(__split_n_head__, n);	\
		ci_list_split_ori_ent(__split_n_head__, new_head, __split_n_ent__);	\
	})

/* similar to ci_list_split_new_ent(), but use a backward index n (0 indicates the last one) */
#define ci_list_split_new_backward_n(ori_head, new_head, n)		\
	({	\
		ci_list_t *__split_n_head__ = (ori_head);	\
		ci_list_t *__split_n_ent__ = ci_list_ent_backward_n(__split_n_head__, n);	\
		ci_list_split_new_ent(__split_n_head__, new_head, __split_n_ent__);	\
	})


#if 0
#define ci_list_count(head)		\
	({		\
		int __count__ = 0;		\
		ci_list_t *__ent__, *__head__ = (head);		\
		\
		ci_list_each_ent(__head__, __ent__)		\
			__count__++;		\
		\
		__count__;		\
	})
#endif

/*
 *	comparator decides ascending or descending, choose the right expectation to make it faster 
 */
#define __ci_list_add_sort(head, obj, field, comparator, iterator, add)		\
	({	\
		ci_typeof(obj) __obj__, __add_pos__ = NULL;		\
		\
		iterator(head, __obj__, field)		\
			if (comparator(obj, __obj__) >= 0)		\
				break;		\
			else		\
				__add_pos__ = __obj__;		\
		\
		add(__add_pos__ ? &__add_pos__->field : (head), &(obj)->field);	\
		obj;		\
	})

#define ci_list_add_sort(head, obj, field, comparator)		\
			ci_list_add_sort_asc(head, obj, field, comparator)
#define ci_list_add_sort_asc(head, obj, field, comparator)	/* expect the new object is bigger */	\
			__ci_list_add_sort(head, obj, field, comparator, ci_list_each_reverse, ci_list_add_before)	
#define ci_list_add_sort_dsc(head, obj, field, comparator)	/* expect the new object is smaller */	\
			__ci_list_add_sort(head, obj, field, comparator, ci_list_each, ci_list_add_after)	






/*
 * counted list
 */
typedef struct __ci_clist_t {
	ci_list_t								 list;		

	union {
		int									 count;
		struct __ci_clist_t					*owner;
	};
} ci_clist_t;


#define ci_clist_def(chead)								ci_clist_t chead = { .list = { &(chead).list, &(chead).list }, .count = 0 }
#define ci_clist_init(chead)							({ci_list_init(&(chead)->list); (chead)->count = 0; (chead); })
#define ci_clist_empty(chead)							ci_list_empty(&(chead)->list)
#define ci_list_to_clist(ent)							ci_container_of(ent, ci_clist_t, list)

#define ci_clist_head(chead)		\
	(	\
		ci_unlikely(ci_clist_empty(chead)) ? NULL :	\
										  (ci_assert(ci_list_to_clist((chead)->list.next)->owner == (chead)),		\
										  ci_list_to_clist((chead)->list.next))		\
	)
#define ci_clist_tail(chead)		\
	(	\
		ci_unlikely(ci_clist_empty(chead)) ? NULL :	\
										  (ci_assert(ci_list_to_clist((chead)->list.prev)->owner == (chead)),		\
										  ci_list_to_clist((chead)->list.prev))		\
	)
#define ci_clist_prev(chead, cent)		\
	(	\
		ci_assert((chead) == (cent)->owner),		\
		ci_unlikely(&(chead)->list == (cent)->list.prev) ? NULL :	\
								 						(ci_assert(ci_list_to_clist((cent)->list.prev)->owner == (chead)),	\
														ci_list_to_clist((cent)->list.prev))	\
	)
#define ci_clist_next(chead, cent)		\
	(	\
		ci_assert((chead) == (cent)->owner),		\
		ci_unlikely(&(chead)->list == (cent)->list.next) ? NULL :	\
								 						(ci_assert(ci_list_to_clist((cent)->list.next)->owner == (chead)),	\
														ci_list_to_clist((cent)->list.next))	\
	)

#define ci_clist_head_inc(chead)		\
	({	\
		ci_assert(chead);	\
		(chead)->count++;		\
		ci_assert((chead)->count > 0);	\
		ci_clist_count_check(chead);	\
		(chead);		\
	})

#define ci_clist_head_dec(chead)		\
	({	\
		ci_assert(chead);	\
		(chead)->count--;		\
		ci_assert((chead)->count >= 0);	\
		ci_clist_count_check(chead);	\
		(chead);		\
	})

#define ci_clist_add_before(next_e, new_e)			\
	({	\
		ci_list_add_before(&(next_e)->list, &(new_e)->list);	\
		(new_e)->owner = (next_e)->owner;	\
		ci_clist_head_inc((new_e)->owner);	\
		(new_e);	\
	})

#define ci_clist_add_after(prev_e, new_e)			\
	({	\
		ci_list_add_after(&(prev_e)->list, &(new_e)->list);	\
		(new_e)->owner = (prev_e)->owner;	\
		ci_clist_head_inc((new_e)->owner);	\
		(new_e);	\
	})

#define ci_clist_add_head(chead, cent)		\
	({	\
		ci_list_add_after(&(chead)->list, &(cent)->list);	\
		(cent)->owner = (chead);	\
		ci_clist_head_inc(chead);	\
		(cent);	\
	})

#define ci_clist_add_tail(chead, cent)		\
	({	\
		ci_list_add_before(&(chead)->list, &(cent)->list);	\
		(cent)->owner = (chead);	\
		ci_clist_head_inc(chead);	\
		(cent);	\
	})

#define ci_clist_del(cent)			\
	({		\
		ci_clist_t *__cent__ = (cent);		\
		\
		ci_assert(__cent__->owner);		\
		ci_list_del(&__cent__->list);	\
		ci_clist_head_dec(__cent__->owner);	\
		ci_list_dbg_exec(__cent__->owner = NULL);	\
		__cent__;	\
	})

#define ci_clist_del_head(chead)		\
	({	\
		ci_clist_t *__head_cent__ = ci_clist_head(chead);		\
		if (__head_cent__)	\
			ci_clist_del(__head_cent__);		\
		__head_cent__;		\
	})

#define ci_clist_del_tail(chead)		\
	({	\
		ci_clist_t *__tail_cent__ = ci_clist_tail(chead);		\
		if (__tail_cent__)	\
			ci_clist_del(__tail_cent__);		\
		__tail_cent__;		\
	})


/*
 *	basic operations for obj
 */
#define ci_clist_head_obj(chead, type, field)			ci_container_of_safe(ci_clist_head(chead), type, field)
#define ci_clist_tail_obj(chead, type, field)			ci_container_of_safe(ci_clist_tail(chead), type, field)
#define ci_clist_prev_obj(chead, obj, field)			ci_container_of_safe(ci_clist_prev(chead, &(obj)->field), ci_typeof(*(obj)), field)
#define ci_clist_next_obj(chead, obj, field)			ci_container_of_safe(ci_clist_next(chead, &(obj)->field), ci_typeof(*(obj)), field)
#define ci_clist_del_head_obj(chead, type, field)		ci_container_of_safe(ci_clist_del_head(chead), type, field)
#define ci_clist_del_tail_obj(chead, type, field)		ci_container_of_safe(ci_clist_del_tail(chead), type, field)


/*
 *	iterators for clist entries, safe version support del in iterating 
 */
#define ci_clist_each_ent(chead, cent)		\
	for ((cent) = ci_list_to_clist((chead)->list.next); &(cent)->list != &(chead)->list; (cent) = ci_list_to_clist((cent)->list.next))
#define ci_clist_each_ent_reverse(chead, cent)		\
    for ((cent) = ci_list_to_clist((chead)->list.prev); &(cent)->list != &(chead)->list; (cent) = ci_list_to_clist((cent)->list.prev))
#define ci_clist_each_ent_safe(chead, cent)	\
    for (ci_list_t *__tmp__ = ((cent) = ci_list_to_clist((chead)->list.next))->list.next;		\
		 (cent) != (chead);	\
		 (cent) = ci_list_to_clist(__tmp__), __tmp__ = __tmp__->next)
#define ci_clist_each_ent_with_index(chead, cent, idx)		\
	for (int idx = !((cent) = ci_list_to_clist((chead)->list.next));	\
		 &(cent)->list != &(chead)->list;		\
		 (cent) = ci_list_to_clist((cent)->list.next), idx++)

#define ci_clist_each_ent_reverse_safe(chead, cent)	\
    for (ci_list_t *__tmp__ = ((cent) = ci_list_to_clist((chead)->list.prev))->list.prev;		\
		 (cent) != (chead);	\
		 (cent) = ci_list_to_clist(__tmp__), __tmp__ = __tmp__->prev)
#define ci_clist_each_ent_reverse_with_index(chead, cent, idx)		\
	for (int idx = !((cent) = ci_list_to_clist((chead)->list.prev));	\
		 &(cent)->list != &(chead)->list;		\
		 (cent) = ci_list_to_clist((cent)->list.prev), idx++)
#define ci_clist_each_ent_safe_with_index(chead, cent, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_clist_each_ent_safe(chead, cent) {	\
			{ __VA_ARGS__; }		\
			idx++;	\
		}	\
	} while (0)
#define ci_clist_each_ent_reverse_safe_with_index(chead, cent, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_clist_each_ent_reverse_safe(chead, cent) {	\
			{ __VA_ARGS__; }		\
			idx++;	\
		}	\
	} while (0)


/*
 *	iterators for clist objects, safe version support del in iterating 
 */
#define ci_clist_each(chead, obj, field)		\
	for ((obj) = ci_container_of((chead)->list.next, ci_typeof(*(obj)), field.list);	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of((obj)->field.list.next, ci_typeof(*(obj)), field.list))
#define ci_clist_each_reverse(chead, obj, field)		\
	for ((obj) = ci_container_of((chead)->list.prev, ci_typeof(*(obj)), field.list);	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of((obj)->field.list.prev, ci_typeof(*(obj)), field.list))
#define ci_clist_each_safe(chead, obj, field)		\
	for (ci_list_t *__tmp__ = ((obj) = ci_container_of((chead)->list.next, ci_typeof(*(obj)), field.list))->field.list.next;	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of(__tmp__, ci_typeof(*(obj)), field.list), __tmp__ = __tmp__->next)
#define ci_clist_each_with_index(chead, obj, field, idx)		\
	for (int idx = !((obj) = ci_container_of((chead)->list.next, ci_typeof(*(obj)), field.list));	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of((obj)->field.list.next, ci_typeof(*(obj)), field.list), idx++)

#define ci_clist_each_reverse_safe(chead, obj, field)		\
	for (ci_list_t *__tmp__ = ((obj) = ci_container_of((chead)->list.prev, ci_typeof(*(obj)), field.list))->field.list.prev;	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of(__tmp__, ci_typeof(*(obj)), field.list), __tmp__ = __tmp__->prev)
#define ci_clist_each_reverse_with_index(chead, obj, field, idx)		\
	for (int idx = !((obj) = ci_container_of((chead)->list.prev, ci_typeof(*(obj)), field.list));	\
		 &(obj)->field != (chead);	\
		 (obj) = ci_container_of((obj)->field.list.prev, ci_typeof(*(obj)), field.list), idx++)
#define ci_clist_each_safe_with_index(chead, obj, field, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_clist_each_safe(chead, obj, field) {		\
			{ __VA_ARGS__; }		\
			idx++;	\
		} \
	} while (0)
#define ci_clist_each_reverse_safe_with_index(chead, obj, field, idx, ...)		\
	do {	\
		int idx = 0;		\
		ci_clist_each_reverse_safe(chead, obj, field) {		\
			{ __VA_ARGS__; }		\
			idx++;	\
		} \
	} while (0)


/* merge clist src into list dst's tail, slower than the ci_list since the owner re-assign */
#define ci_clist_set_owner(dst, src)		\
	({	\
		ci_clist_t *__owner_cent__;		\
		\
		ci_clist_each_ent(src, __owner_cent__)		\
			__owner_cent__->owner = (dst);		\
		dst;		\
	})

#define ci_clist_merge_tail(dst, src)		\
	({		\
		ci_clist_set_owner(dst, src);		\
		ci_list_merge_tail(&(dst)->list, &(src)->list);		\
		(dst)->count += (src)->count;		\
		(src)->count = 0;		\
		ci_clist_count_check(dst);		\
		ci_clist_count_check(src);		\
		dst;	\
	})

/* merge clist src into list dst's head */
#define ci_clist_merge_head(dst, src)		\
	({		\
		ci_clist_set_owner(dst, src);		\
		ci_list_merge_head(&(dst)->list, &(src)->list);		\
		(dst)->count += (src)->count;		\
		(src)->count = 0;		\
		ci_clist_count_check(dst);		\
		ci_clist_count_check(src);		\
		dst;	\
	})

/* move clist src to dst, dst will be reinitialized */
#define ci_clist_move(dst, src)		\
	({		\
		ci_clist_t *__dst__ = (dst), *__src__ = (src);		\
		\
		ci_clist_init(__dst__);		\
		ci_clist_merge_tail(__dst__, __src__);		\
		__dst__;		\
	})

/* get number of entries in the list: alias */
#define ci_clist_count(chead)				 ((chead)->count)

/*
 *	comparator decides ascending or descending, choose the right expectation to make it faster 
 */
#define __ci_clist_add_sort(head, obj, field, comparator, iterator, add1, add2)		\
	({	\
		ci_typeof(obj) __obj__, __add_pos__ = NULL;		\
		\
		iterator(head, __obj__, field)		\
			if (comparator(obj, __obj__) >= 0)		\
				break;		\
			else		\
				__add_pos__ = __obj__;		\
		\
		if (__add_pos__)		\
			add1(&__add_pos__->field, &(obj)->field);	\
		else	\
			add2(head, &(obj)->field);		\
		obj;		\
	})

#define ci_clist_add_sort(head, obj, field, comparator)		\
			ci_clist_add_sort_asc(head, obj, field, comparator)
#define ci_clist_add_sort_asc(head, obj, field, comparator)	/* expect the new object is bigger */	\
			__ci_clist_add_sort(head, obj, field, comparator, ci_clist_each_reverse, ci_clist_add_before, ci_clist_add_tail)	
#define ci_clist_add_sort_dsc(head, obj, field, comparator)	/* expect the new object is smaller */	\
			__ci_clist_add_sort(head, obj, field, comparator, ci_clist_each, ci_clist_add_after, ci_clist_add_head)	



/*
 *	Chores
 */
#define ci_list_at_index(head, idx)		\
	({	\
		int __list_count_down__ = idx;	\
		ci_list_t *__index_ent__ = ci_list_head(head);	\
		\
		ci_assert(__list_count_down__ >= 0);	\
		while (__index_ent__ && (__list_count_down__-- > 0))	\
			__index_ent__ = ci_list_next(head, __index_ent__);	\
		\
		__index_ent__;	\
	})
#define ci_list_obj_at_index(head, type, field, index)	\
	({	\
		ci_list_t *__obj_ent__;		\
		type *__index_obj__ = NULL;	\
		\
		if ((__obj_ent__ = ci_list_at_index(head, index)))	\
			__index_obj__ = ci_container_of(__obj_ent__, type, field);	\
		__index_obj__;	\
	})
#define ci_list_del_all(head)		\
	do {	\
		ci_list_t *__del_all_ent__, *__del_all_head__ = (head);		\
		ci_list_each_ent_safe(__del_all_head__, __del_all_ent__)	\
			ci_list_del(__del_all_ent__);	\
		ci_list_init(__del_all_head__);	\
	} while (0)
#define ci_list_ary_init(ary)	\
	ci_ary_each(ary, h) ci_list_init(h)
		
