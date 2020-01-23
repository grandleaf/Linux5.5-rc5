/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_worker.c				PAL Workers
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"
#include "pal_numa.h"


static void pal_worker_loop(pal_worker_t *pal_worker)
{
	ci_worker_t *ci_worker = pal_worker->ci_worker;
	int (*do_work)(ci_worker_t *) = ci_worker->do_work;
	pal_soft_irq_t *sirq = &pal_worker->soft_irq;

	for (;;) {
		pal_soft_irq_wait(sirq);
		pal_soft_irq_clear(sirq);

		while (do_work(ci_worker))	/* loop is a must, else lost requests */
			;
	}
}

static void *pal_worker_exec(void *__worker)
{
	ci_worker_t *ci_worker;
	pal_worker_t *pal_worker = (pal_worker_t *)__worker;
	pal_soft_irq_t *sirq = &pal_worker->soft_irq;
	
//	u8 dummy[65536] = { 0 }; ci_memset(dummy, 0xCC, ci_sizeof(dummy)); ci_printf("dummy[55]=%d\n", dummy[55]);

	/* init pal worker */
	pal_worker->thread_id = pal_get_tid();
	pal_worker->stack_curr = (u8 *)&ci_worker;		/* check stack from here */
	ci_thread_set_affinity(pal_worker->thread, pal_worker->cpu_id);
	sem_post(&pal_numa_info.sem_sync);		/* unblock the caller to create the next worker thread */

	/* waiting for ci_worker ready */
	pal_soft_irq_wait_raw(sirq);		
	ci_assert(pal_worker->ci_worker && pal_worker->ci_worker->do_work);

	ci_worker = pal_worker->ci_worker;
	pthread_setspecific(pal_tls_key, &ci_worker->sched_tab.ctx);
	pal_worker->node_id = ci_worker->node_id;
	pal_worker->worker_id = ci_worker->worker_id;

	pal_printf("pal_worker=%p, tid=%05lld, numa/cpu=< %d, %02d >, node/worker=< %d, %02d >, stack=< %d, [ %p, %p ) >\n",
				pal_worker, pal_worker->thread_id, pal_worker->numa_id, pal_worker->cpu_id, pal_worker->node_id, pal_worker->worker_id,
				pal_numa_id_by_stack(), pal_worker->stack, pal_worker->stack + PAL_WORKER_STACK_SIZE);
	sem_post(&pal_numa_info.sem_sync);		/* unblock the caller to continue next ci_worker init */

	/*
	 *	worker's dead loop
	 */
	pal_worker_loop(pal_worker);

	return pal_worker;
}

#ifdef WIN_SIM
static void pal_win_worker_thread_hack(pal_worker_t *worker)
{
	u8 stack[PAL_WORKER_STACK_SIZE];
	ci_memset64(stack, PAL_STACK_INIT_PTN, PAL_WORKER_STACK_SIZE);
	worker->stack = (u8 *)stack;
	ci_exec_upto(1, ci_warn_printf("not optimal stack binding for WIN_SIM, ignore numa stack setting\n"));
}

static void *pal_win_worker_exec(void *__worker)
{
	pal_worker_t *worker = (pal_worker_t *)__worker;

	/* never returns */
	pal_win_worker_thread_hack(worker);
	pal_worker_exec(worker);
//	pal_jump((uintptr_t)pal_worker_exec, (uintptr_t)__worker, (uintptr_t)stack_upper_addr);

	return worker;	/* dummy */
}
#endif

int pal_worker_create(pal_worker_t *worker)
{
	int rv;

	pthread_attr_init(&worker->attr);
	pthread_attr_setdetachstate(&worker->attr, PTHREAD_CREATE_JOINABLE);	

	ci_assert(worker->stack);
	ci_align_check(PAL_WORKER_STACK_SIZE, ci_sizeof(u64));
	ci_memset64(worker->stack, PAL_STACK_INIT_PTN, PAL_WORKER_STACK_SIZE);

	pal_soft_irq_init(&worker->soft_irq);

#ifdef WIN_SIM
	pthread_attr_setstackaddr(&worker->attr, worker->stack);		/* dummy! not supported */
	rv = pthread_create(&worker->thread, &worker->attr, pal_win_worker_exec, worker); 
#else
	pthread_attr_setstack(&worker->attr, worker->stack, PAL_WORKER_STACK_SIZE);
	rv = pthread_create(&worker->thread, &worker->attr, pal_worker_exec, worker); 
#endif

	ci_panic_if(rv, "cannot create PAL Worker %p\n", worker);
	pthread_attr_destroy(&worker->attr);

	return 0;
}

int pal_worker_destroy(pal_worker_t *worker)
{
	pthread_cancel(worker->thread);
	pthread_join(worker->thread, NULL);
	pal_soft_irq_finz(&worker->soft_irq);

	return 0;
}

u8 *pal_worker_current_stack(pal_worker_t *worker)
{
	for (u64 *p = (u64 *)(worker->stack_curr); p >= (u64 *)worker->stack; p--) 
		if (ci_unlikely(*p == PAL_STACK_INIT_PTN)) {
			worker->stack_curr = (u8 *)p;
			break;
		}

	if (ci_unlikely(worker->stack_curr <= worker->stack))
		ci_err_printfln("! STACK OVERFLOW, node/worker=%d/%02d, numa/cpu=%d/%02d, [ %p, %p )\n", 
						worker->node_id, worker->worker_id, worker->numa_id, worker->cpu_id,
						worker->stack, worker->stack + PAL_WORKER_STACK_SIZE);

	return worker->stack_curr;
}

int pal_worker_stack_usage(pal_worker_t *worker)
{
	pal_worker_current_stack(worker);
	return PAL_WORKER_STACK_SIZE - (int)(worker->stack_curr - worker->stack);
}

const char *ci_worker_mod_name()
{
	ci_mod_t *mod= ci_worker_mod();
	return mod ? mod->name : NULL;
}

