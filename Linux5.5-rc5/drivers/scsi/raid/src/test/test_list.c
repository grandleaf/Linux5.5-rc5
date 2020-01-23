#include "ci.h"

#define STRESS_TEST_NR_OBJ_A			1024

ci_pragma_no_warning(CI_WARN_UNUSED_FUNCTION)

#define NR_OBJ_A			32

typedef struct {
	int						 val;
	int						 dummy[25];
	union {
		ci_list_t		 link;
		ci_clist_t		 clink;
	};

	u8						 ci_rsvd[3];		/* reserved field */
	int						 ci_rsvd;
} obj_a_t;

static obj_a_t objs[NR_OBJ_A];
static obj_a_t objs2[NR_OBJ_A];
static ci_list_t list_head;
ci_list_def(list_head_reverse);

static void list_head_init()
{
	ci_here();
	ci_list_init(&list_head);
	ci_loop(i, NR_OBJ_A) {
		obj_a_t *obj = objs + i;
		obj->val = i;
		ci_list_add_tail(&list_head, &obj->link);
	}
}

static void list_head_init2()
{
	obj_a_t *obj;

	ci_here();
	ci_loop(i, NR_OBJ_A) {
		obj = objs2 + i;
		obj->val = i + 1000;
		ci_list_add_head(&list_head_reverse, &obj->link);
	}

	ci_list_each(&list_head_reverse, obj, link)
		ci_printf("val=%d, ptr=%p\n", obj->val, obj);
}

static void list_traverse()
{
	ci_list_t *ent;

	ci_here();
	ci_list_each_ent_reverse(&list_head, ent) {
		obj_a_t *obj = ci_container_of(ent, obj_a_t, link);
		ci_printf("%d, %p, %p, \n", obj->val, ent, obj);
	}
}

static void list_traverse2()		/* easier way, you might use this */
{
	obj_a_t *obj;

	ci_here();
	ci_list_each(&list_head, obj, link)			
		ci_printf("val=%d, ptr=%p\n", obj->val, obj);

	ci_printf("list_count=%d\n", ci_list_count(&list_head));
}

static void list_traverse3()		/* reverse order */
{
	obj_a_t *obj;

	ci_here();
	ci_list_each_reverse(&list_head, obj, link)			
		ci_printf("val=%d, ptr=%p\n", obj->val, obj);

	ci_printf("list_count=%d\n", ci_list_count(&list_head));
}

static void list_del2()
{
	obj_a_t *obj;

	ci_here();
	ci_list_each_safe_with_index(&list_head, obj, link, iii, {
		if (obj->val % 7 == 0)
			ci_list_del(&obj->link);
	});

	ci_here();
	ci_list_each_reverese_safe_with_index(&list_head, obj, link, idx, {
		if (obj->val % 9 == 0)
			ci_list_del(&obj->link);
	});

	ci_list_each_reverese_safe_with_index(&list_head, obj, link, jjj, {
		ci_printf("i=%d, val=%d, ptr=%p\n", jjj, obj->val, obj);
	});
}

static void list_del_ent()
{
	obj_a_t *obj;
	ci_list_t *ent;

	ci_here();
	ci_list_each_ent_safe(&list_head, ent) {	/* the "unsafe" version will crash */
		obj = ci_container_of(ent, obj_a_t, link);
		if (obj->val % 7 == 0)
			ci_list_del(ent);
	}

	ci_list_each_reverse_safe(&list_head, obj, link)
		if (obj->val % 5 == 0)
			ci_list_del(&obj->link);
}

static void list_traverse_with_index()
{
	obj_a_t *obj;
	ci_list_t *ent;

	ci_here();
	ci_list_each_ent_with_index(&list_head, ent, i) {
		obj = ci_container_of(ent, obj_a_t, link);
		ci_printf("idx=%d, var=%d\n", i, obj->val);
	}

	ci_here();
	ci_list_each_ent_safe_with_index(&list_head, ent, iii, {
		obj = ci_container_of(ent, obj_a_t, link);
		if (obj->val == 6)
			ci_list_del(ent);
		else
			ci_printf("idx=%d, var=%d\n", iii, obj->val);
	});

	ci_here();
	ci_list_each_ent_reverse_safe_with_index(&list_head, ent, iii, {
		obj = ci_container_of(ent, obj_a_t, link);
		if (obj->val == 5)
			ci_list_del(ent);
		else
			ci_printf("idx=%d, var=%d\n", iii, obj->val);
	});
}

void list_obj_get_test()
{
	obj_a_t *obj;


//	obj = ci_list_head_obj(&list_head, obj_a_t, link);

	obj = ci_list_head_obj(&list_head, obj_a_t, link);
	ci_printf("first object=%p\n", obj);

	obj = ci_list_prev_obj(&list_head, obj, link);
	ci_printf("prev object=%p\n", obj);

	obj = ci_list_tail_obj(&list_head, obj_a_t, link);
	ci_printf("last object=%p\n", obj);

	obj = ci_list_prev_obj(&list_head, obj, link);
	ci_printf("prev object=%p\n", obj);

	obj = ci_list_next_obj(&list_head, obj, link);
	ci_printf("next object=%p\n", obj);

	obj = ci_list_next_obj(&list_head, obj, link);
	ci_printf("next object=%p\n", obj);

	while ((obj = ci_list_del_head_obj(&list_head, obj_a_t, link)))
		list_traverse2();
}

static void list_error_test()
{
	obj_a_t *obj;

	ci_here();
	list_head_init();				// put all the objects into the header

	obj = ci_list_head_obj(&list_head, ci_typeof(*obj), link);

	ci_list_del(&obj->link);
//	ci_list_del(&obj->link);		// double del test

	ci_list_add_tail(&list_head, &obj->link);
//	ci_list_add_tail(&list_head, &obj->link);		// double add test

	ci_list_t *ent = (ci_list_t *)pal_malloc(ci_sizeof(ci_list_t));
	ci_printfln("1. ent->prev=%p, ent->next=%p", ent->prev, ent->next);
	ci_list_add_head(&list_head, ent);				// add CCCCCCC
//	ci_list_add_head(&list_head, ent);				// add CCCCCCC
	ci_list_del(ent);
//	ci_list_del(ent);

	ent = (ci_list_t *)pal_aligned_malloc(ci_sizeof(ci_list_t), 8);
	ci_printfln("2. ent->prev=%p, ent->next=%p", ent->prev, ent->next);
	ci_list_add_head(&list_head, ent);				// add CCCCCCC
//	ci_list_add_head(&list_head, ent);				// add CCCCCCC

	ent = (ci_list_t *)pal_numa_alloc(0, ci_sizeof(ci_list_t));
	ci_printfln("3. ent->prev=%p, ent->next=%p", ent->prev, ent->next);
	ci_list_add_head(&list_head, ent);				// add CCCCCCC

	ent = (ci_list_t *)pal_zalloc(ci_sizeof(ci_list_t));
	ci_printfln("4. ent->prev=%p, ent->next=%p", ent->prev, ent->next);
	ci_list_add_head(&list_head, ent);				// add 00000000

	static ci_list_t ent2;
	ci_printfln("5. ent2.prev=%p, ent2.next=%p", ent2.prev, ent2.next);
	ci_list_add_head(&list_head, &ent2);				// add 00000000

/*  ERROR!
	ci_list_t ent3;
	ci_printfln("6. ent3.prev=%p, ent3.next=%p", ent3.prev, ent3.next);
	ci_list_add_head(&list_head, &ent3);				// add 00000000
 */	
}

static void list_add_sort_test()
{
	ci_here();
	ci_memzero(&objs, ci_sizeof(objs));
	ci_list_init(&list_head);

	ci_loop(i, NR_OBJ_A) {
		obj_a_t *obj = objs + i;
		obj->val = ci_rand_shr(1000, 2000);

#define val_cmp(a, b)		((a)->val - (b)->val)		// or make a function
		ci_list_add_sort(&list_head, obj, link, val_cmp);
	}
}

static ci_list_t *get_rand_loc(ci_list_t *head)
{
	ci_list_t *ent;
	int counter = 0;
	int expect = ci_rand_shr(30, 50);

	ci_list_each_ent(head, ent) {
		if (counter++ == expect)
			return ent;
	}

	return ci_list_head(head);
}

static void stress_with_op(ci_list_t *ping, ci_list_t *pong, int op)
{
	ci_list_t *ent;
	obj_a_t *obj;

	switch (op) {
		case 0:
			ent = ci_list_del_head(ping);
			if (ent)
				ci_list_add_head(pong, ent);
			break;
		case 1:
			ent = ci_list_del_tail(ping);
			if (ent)
				ci_list_add_tail(pong, ent);
			break;
		case 2:
			ent = ci_list_del_head(ping);
			if (ent)
				ci_list_add_tail(pong, ent);
			break;
		case 3:
			ent = ci_list_del_tail(ping);
			if (ent)
				ci_list_add_head(pong, ent);
			break;
		case 4:
			ent = ci_list_del_head(pong);
			if (ent)
				ci_list_add_head(ping, ent);
			break;
		case 5:
			ent = ci_list_del_tail(pong);
			if (ent)
				ci_list_add_tail(ping, ent);
			break;
		case 6:
			ent = ci_list_del_head(pong);
			if (ent)
				ci_list_add_tail(ping, ent);
			break;
		case 7:
			ent = ci_list_del_tail(pong);
			if (ent)
				ci_list_add_head(ping, ent);
			break;


		case 8:
			obj = ci_list_del_head_obj(ping, obj_a_t, link);
			if (obj)
				ci_list_add_head(pong, &obj->link);
			break;
		case 9:
			obj = ci_list_del_tail_obj(ping, obj_a_t, link);
			if (obj)
				ci_list_add_tail(pong, &obj->link);
			break;
		case 10:
			obj = ci_list_del_head_obj(ping, obj_a_t, link);
			if (obj)
				ci_list_add_tail(pong, &obj->link);
			break;
		case 11:
			obj = ci_list_del_tail_obj(ping, obj_a_t, link);
			if (obj)
				ci_list_add_head(pong, &obj->link);
			break;
		case 12:
			obj = ci_list_del_head_obj(pong, obj_a_t, link);
			if (obj)
				ci_list_add_head(ping, &obj->link);
			break;
		case 13:
			obj = ci_list_del_tail_obj(pong, obj_a_t, link);
			if (obj)
				ci_list_add_tail(ping, &obj->link);
			break;
		case 14:
			obj = ci_list_del_head_obj(pong, obj_a_t, link);
			if (obj)
				ci_list_add_tail(ping, &obj->link);
			break;
		case 15:
			obj = ci_list_del_tail_obj(pong, obj_a_t, link);
			if (obj)
				ci_list_add_head(ping, &obj->link);
			break;

		case 16:
			ent = get_rand_loc(ping);
			if (ent) {
				ci_list_del(ent);
				ci_list_add_head(pong, ent);
			}
			break;
		case 17:
			ent = get_rand_loc(pong);
			if (ent) {
				ci_list_del(ent);
				ci_list_add_head(ping, ent);
			}
			break;

		case 18:
		case 19:
			ent = get_rand_loc(pong);
			if (ent) {
				ci_list_t *e = ci_list_del_head(ping);
				if (e) {
					if (op & 0x01)
						ci_list_add_after(ent, e);
					else
						ci_list_add_before(ent, e);
				}
			}
			break;
		case 20:
		case 21:
			ent = get_rand_loc(ping);
			if (ent) {
				ci_list_t *e = ci_list_del_head(pong);
				if (e) {
					if (op & 0x01)
						ci_list_add_after(ent, e);
					else
						ci_list_add_before(ent, e);
				}
			}
			break;

		case 50:
			if (ci_rand_shr(0, 100000) == 0)
				ci_list_merge_head(ping, pong);
			break;
		case 51:
			if (ci_rand_shr(0, 100000) == 0)
				ci_list_merge_tail(pong, ping);
			break;

		default:
			break;
	}
}

static void list_stress_test()
{
	u64 cnt = 0;
	obj_a_t *obj, *pool;
	ci_list_def(ping);
	ci_list_def(pong);

	pool = (obj_a_t *)pal_malloc(STRESS_TEST_NR_OBJ_A * ci_sizeof(obj_a_t));
	ci_printf("pool=%p, alloc_size=%d KiB\n", pool, ci_to_kib(STRESS_TEST_NR_OBJ_A * ci_sizeof(obj_a_t)));

	ci_loop(i, STRESS_TEST_NR_OBJ_A) {
		obj = pool + i;
		ci_memzero(obj, ci_sizeof(*obj));
		obj->val = ci_rand_shr(0, 10000);
		ci_list_add_sort(&ping, obj, link, val_cmp);
	}

	for (int big_loop = 0;; cnt++) {
		if (cnt % 10000000 == 0) {
			int ping_count = ci_list_count(&ping);
			int pong_count = ci_list_count(&pong);
			ci_printf("list[%d]: ping_count=%04d, pong_count=%04d, total=%04d\n", big_loop++, ping_count, pong_count, ping_count + pong_count);
		}

		stress_with_op(&ping, &pong, ci_rand_shr(0, 100));
	}
}

static void do_list_test()
{
	list_head_init();				// put all the objects into the header
	list_head_init2();				// another way to initialize the header

//	list_traverse();				// traverse demo 
//	list_traverse2();				// traverse demo 2, most frequently used
//	list_traverse_with_index();		// traverse with index, also delete element 5
//	list_del_ent();					// deleting while iterating demo
	list_del2();					// deleting while iterating, reverse order, with index
//	list_obj_get_test();

//	ci_list_merge_tail(&list_head, &list_head_reverse);		// merge to tail
//	ci_list_merge_head(&list_head, &list_head_reverse);		// merge to head
//	ci_list_move(&list_head, &list_head_reverse);	// move all elements in tgt into src

//	list_add_sort_test();

	list_traverse2();				// show result
//	list_traverse3();
}


ci_clist_def(clist_head);

static void clist_head_init()
{
	ci_here();
//	ci_clist_init(&clist_head);

	ci_loop(i, NR_OBJ_A) {
		obj_a_t *obj = objs + i;
		obj->val = i;
		ci_clist_add_tail(&clist_head, &obj->clink);
		ci_printf("[%03d] obj=%p, val=%03d, clink=%p\n", i, obj, obj->val, &obj->clink);
	}
}

static void clist_show()
{
	obj_a_t *obj;

	ci_printf("\nDumping CList: %p\n", &clist_head);
	ci_clist_each(&clist_head, obj, clink)
		ci_printf("obj=%p, val=%d\n", obj, obj->val);

//	ci_clist_each_reverse(&clist_head, obj, clink) ci_printf("obj=%p, val=%d\n", obj, obj->val);
}

static void clist_chores_test()
{
	obj_a_t *head_obj, *tail_obj;
	ci_clist_t *head, *tail;

	ci_printf("ci_clist_empty()=%d\n", ci_clist_empty(&clist_head));
//	ci_clist_init(&clist_head);	ci_printf("after init, ci_clist_empty()=%d\n", ci_clist_empty(&clist_head));

	ci_printf("ci_clist_head:%p\n", head = ci_clist_head(&clist_head));
	ci_printf("ci_clist_head_obj:%p\n", head_obj = ci_clist_head_obj(&clist_head, obj_a_t, clink));
	ci_printf("ci_clist_tail:%p\n", tail = ci_clist_tail(&clist_head));
	ci_printf("ci_clist_tail_obj:%p\n", tail_obj = ci_clist_tail_obj(&clist_head, obj_a_t, clink));
	ci_printf("\n");

	ci_printf("1. ci_clist_next:%p\n", ci_clist_next(&clist_head, head));
	ci_printf("2. ci_clist_next:%p\n", ci_clist_next(&clist_head, ci_clist_head(&clist_head)));
	ci_printf("1. ci_clist_next_obj:%p\n", ci_clist_next_obj(&clist_head, head_obj, clink));
	ci_printf("2. ci_clist_next_obj:%p\n", ci_clist_next_obj(&clist_head, ci_clist_head_obj(&clist_head, obj_a_t, clink), clink));
	ci_printf("1. ci_clist_prev:%p\n", ci_clist_prev(&clist_head, head));
	ci_printf("2. ci_clist_prev:%p\n", ci_clist_prev(&clist_head, ci_clist_head(&clist_head)));
	ci_printf("1. ci_clist_prev_obj:%p\n", ci_clist_prev_obj(&clist_head, head_obj, clink));
	ci_printf("2. ci_clist_prev_obj:%p\n", ci_clist_prev_obj(&clist_head, ci_clist_head_obj(&clist_head, obj_a_t, clink), clink));
	ci_printf("\n");

	ci_printf("1. ci_clist_next:%p\n", ci_clist_next(&clist_head, tail));
	ci_printf("2. ci_clist_next:%p\n", ci_clist_next(&clist_head, ci_clist_tail(&clist_head)));
	ci_printf("1. ci_clist_next_obj:%p\n", ci_clist_next_obj(&clist_head, tail_obj, clink));
	ci_printf("2. ci_clist_next_obj:%p\n", ci_clist_next_obj(&clist_head, ci_clist_tail_obj(&clist_head, obj_a_t, clink), clink));
	ci_printf("1. ci_clist_prev:%p\n", ci_clist_prev(&clist_head, tail));
	ci_printf("2. ci_clist_prev:%p\n", ci_clist_prev(&clist_head, ci_clist_tail(&clist_head)));
	ci_printf("1. ci_clist_prev_obj:%p\n", ci_clist_prev_obj(&clist_head, tail_obj, clink));
	ci_printf("2. ci_clist_prev_obj:%p\n", ci_clist_prev_obj(&clist_head, ci_clist_tail_obj(&clist_head, obj_a_t, clink), clink));
	ci_printf("\n");


}

static void clist_traverse()
{
	obj_a_t *obj;
	ci_clist_t *e;

	ci_printf("\nci_clist_each_ent ...\n");
	ci_clist_each_ent(&clist_head, e) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d\n", e, obj, obj->val);
	}

	ci_printf("\nci_clist_each_ent_reversee ...\n");
	ci_clist_each_ent_reverse(&clist_head, e) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d\n", e, obj, obj->val);
	}

	ci_printf("\nci_clist_each_ent_safe ...\n");
	ci_clist_each_ent_safe(&clist_head, e) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d\n", e, obj, obj->val);
		if (obj->val % 9 == 0)
			ci_clist_del(&obj->clink);
	}

	ci_clist_del(ci_clist_head(&clist_head));
	ci_clist_del(ci_clist_tail(&clist_head));

	ci_printf("\nci_clist_each_ent_with_index ...\n");
	ci_clist_each_ent_with_index(&clist_head, e, idx) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d, idx=%d\n", e, obj, obj->val, idx);
	}

	ci_printf("\nci_clist_each_ent_reverse_safe ...\n");
	ci_clist_each_ent_reverse_safe(&clist_head, e) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d\n", e, obj, obj->val);
		if (obj->val % 5 == 0)
			ci_clist_del(&obj->clink);
	}

	ci_printf("\nci_clist_each_ent_reverse_with_index ...\n");
	ci_clist_each_ent_reverse_with_index(&clist_head, e, idx) {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d, idx=%d\n", e, obj, obj->val, idx);
	}

	ci_printf("\nci_clist_each_ent_safe_with_index\n");
	ci_clist_each_ent_safe_with_index(&clist_head, e, idx, {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d, idx=%d\n", e, obj, obj->val, idx);
		if (obj->val % 11 == 0)
			ci_clist_del(&obj->clink);
	});

	ci_printf("\nci_clist_each_ent_reverse_safe_with_index\n");
	ci_clist_each_ent_reverse_safe_with_index(&clist_head, e, iii, {
		obj = ci_container_of(e, obj_a_t, clink);
		ci_printf("e=%p, obj=%p, val=%d, idx=%d\n", e, obj, obj->val, iii);
		if (obj->val % 13 == 0)
			ci_clist_del(&obj->clink);
	});
	
	clist_show();
}

static void clist_traverse2()
{
	obj_a_t *obj;

	ci_printf("\nci_clist_each ...\n");
	ci_clist_each(&clist_head, obj, clink) 
		ci_printf("obj=%p, val=%d\n", obj, obj->val);

	ci_printf("\nci_clist_each_reverse ...\n");
	ci_clist_each_reverse(&clist_head, obj, clink) 
		ci_printf("obj=%p, val=%d\n", obj, obj->val);

	ci_printf("\nci_clist_each_safe ...\n");
	ci_clist_each_safe(&clist_head, obj, clink) {
		ci_printf("obj=%p, val=%d\n", obj, obj->val);
		if (obj->val % 9 == 0)
			ci_clist_del(&obj->clink);
	}

	ci_clist_del(ci_clist_head(&clist_head));
	ci_clist_del(ci_clist_tail(&clist_head));

	ci_printf("\nci_clist_each_with_index ...\n");
	ci_clist_each_with_index(&clist_head, obj, clink, idx) 
		ci_printf("obj=%p, val=%d, idx=%d\n", obj, obj->val, idx);

	ci_printf("\nci_clist_each_reverse_safe ...\n");
	ci_clist_each_reverse_safe(&clist_head, obj, clink) {
		ci_printf("obj=%p, val=%d\n", obj, obj->val);
		if (obj->val % 5 == 0)
			ci_clist_del(&obj->clink);
	}

	ci_printf("\nci_clist_each_reverse_with_index ...\n");
	ci_clist_each_reverse_with_index(&clist_head, obj, clink, idx) 
		ci_printf("obj=%p, val=%d, idx=%d\n", obj, obj->val, idx);

	ci_printf("\nci_clist_each_safe_with_index\n");
	ci_clist_each_safe_with_index(&clist_head, obj, clink, idx, {
		ci_printf("obj=%p, val=%d, idx=%d\n", obj, obj->val, idx);
		if (obj->val % 11 == 0)
			ci_clist_del(&obj->clink);
	});

	ci_printf("\nci_clist_each_reverse_safe_with_index\n");
	ci_clist_each_reverse_safe_with_index(&clist_head, obj, clink, idx, {
		ci_printf("obj=%p, val=%d, idx=%d\n", obj, obj->val, idx);
		if (obj->val % 13 == 0)
			ci_clist_del(&obj->clink);
	});
	
	clist_show();
}

static void do_clist_test()
{
	ci_here();
	clist_head_init();

//	clist_chores_test();
//	clist_traverse();
	clist_traverse2();
//	clist_show();
}

static ci_clist_t *cget_rand_loc(ci_clist_t *head)
{
	ci_clist_t *ent;

	if (!head->count)
		return ci_clist_head(head);

	int counter = 0;
	int expect = ci_rand_shr(0, head->count);

	ci_clist_each_ent(head, ent) {
		if (counter++ == expect)
			return ent;
	}

	return ci_clist_head(head);
}

static void c_stress_with_op(ci_clist_t *ping, ci_clist_t *pong, int op)
{
	ci_clist_t *ent;
	obj_a_t *obj;

	switch (op) {
		case 0:
			ent = ci_clist_del_head(ping);
			if (ent)
				ci_clist_add_head(pong, ent);
			break;
		case 1:
			ent = ci_clist_del_tail(ping);
			if (ent)
				ci_clist_add_tail(pong, ent);
			break;
		case 2:
			ent = ci_clist_del_head(ping);
			if (ent)
				ci_clist_add_tail(pong, ent);
			break;
		case 3:
			ent = ci_clist_del_tail(ping);
			if (ent)
				ci_clist_add_head(pong, ent);
			break;
		case 4:
			ent = ci_clist_del_head(pong);
			if (ent)
				ci_clist_add_head(ping, ent);
			break;
		case 5:
			ent = ci_clist_del_tail(pong);
			if (ent)
				ci_clist_add_tail(ping, ent);
			break;
		case 6:
			ent = ci_clist_del_head(pong);
			if (ent)
				ci_clist_add_tail(ping, ent);
			break;
		case 7:
			ent = ci_clist_del_tail(pong);
			if (ent)
				ci_clist_add_head(ping, ent);
			break;

		case 8:
			obj = ci_clist_del_head_obj(ping, obj_a_t, clink);
			if (obj)
				ci_clist_add_head(pong, &obj->clink);
			break;
		case 9:
			obj = ci_clist_del_tail_obj(ping, obj_a_t, clink);
			if (obj)
				ci_clist_add_tail(pong, &obj->clink);
			break;
		case 10:
			obj = ci_clist_del_head_obj(ping, obj_a_t, clink);
			if (obj)
				ci_clist_add_tail(pong, &obj->clink);
			break;
		case 11:
			obj = ci_clist_del_tail_obj(ping, obj_a_t, clink);
			if (obj)
				ci_clist_add_head(pong, &obj->clink);
			break;
		case 12:
			obj = ci_clist_del_head_obj(pong, obj_a_t, clink);
			if (obj)
				ci_clist_add_head(ping, &obj->clink);
			break;
		case 13:
			obj = ci_clist_del_tail_obj(pong, obj_a_t, clink);
			if (obj)
				ci_clist_add_tail(ping, &obj->clink);
			break;
		case 14:
			obj = ci_clist_del_head_obj(pong, obj_a_t, clink);
			if (obj)
				ci_clist_add_tail(ping, &obj->clink);
			break;
		case 15:
			obj = ci_clist_del_tail_obj(pong, obj_a_t, clink);
			if (obj)
				ci_clist_add_head(ping, &obj->clink);
			break;

		case 16:
			ent = cget_rand_loc(ping);
			if (ent) {
				ci_clist_del(ent);
				ci_clist_add_head(pong, ent);
			}
			break;
		case 17:
			ent = cget_rand_loc(pong);
			if (ent) {
				ci_clist_del(ent);
				ci_clist_add_head(ping, ent);
			}
			break;

		case 18:
		case 19:
			ent = cget_rand_loc(pong);
			if (ent) {
				ci_clist_t *e = ci_clist_del_head(ping);
				if (e) {
					if (op & 0x01)
						ci_clist_add_after(ent, e);
					else
						ci_clist_add_before(ent, e);
				}
			}
			break;
		case 20:
		case 21:
			ent = cget_rand_loc(ping);
			if (ent) {
				ci_clist_t *e = ci_clist_del_head(pong);
				if (e) {
					if (op & 0x01)
						ci_clist_add_after(ent, e);
					else
						ci_clist_add_before(ent, e);
				}
			}
			break;


		case 50:
			if (ci_rand_shr(0, 100000) == 0)
				ci_clist_merge_head(ping, pong);
			break;
		case 51:
			if (ci_rand_shr(0, 100000) == 0)
				ci_clist_merge_tail(pong, ping);
			break;


		default:
			break;
	}
}

static void clist_stress_test()
{
	u64 cnt = 0;
	obj_a_t *obj, *pool;
	ci_clist_def(ping);
	ci_clist_def(pong);

	pool = (obj_a_t *)pal_malloc(STRESS_TEST_NR_OBJ_A * ci_sizeof(obj_a_t));
	ci_printf("CLIST, pool=%p, alloc_size=%d KiB\n", pool, ci_to_kib(STRESS_TEST_NR_OBJ_A * ci_sizeof(obj_a_t)));

	ci_loop(i, STRESS_TEST_NR_OBJ_A) {
		obj = pool + i;
		ci_memzero(obj, ci_sizeof(*obj));
		obj->val = ci_rand_shr(0, 10000);
		ci_clist_add_sort(&ping, obj, clink, val_cmp);
	}

	for (int big_loop = 0; ; cnt++) {
		if (cnt % 10000000 == 0) {
			int ping_count = ping.count;
			int pong_count = ci_clist_count(&pong);
			ci_printf("clist[%d]: ping_count=%04d, pong_count=%04d, total=%04d\n", big_loop++, ping_count, pong_count, ping_count + pong_count);
			ci_assert(ping_count == ci_list_count(&ping.list));
			ci_assert(pong_count == ci_list_count(&pong.list));
		}

		c_stress_with_op(&ping, &pong, ci_rand_shr(0, 100));
	}
}

void test_list()
{
	ci_here();

//	list_error_test();

//	do_list_test();
	list_stress_test();

//	do_clist_test();
//	clist_stress_test();
}

