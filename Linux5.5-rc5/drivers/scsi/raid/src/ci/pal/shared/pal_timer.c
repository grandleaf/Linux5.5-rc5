/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_timer.c				Timer Service without skew
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

static pthread_t pal_timer_thread;
u64 pal_jiffie, pal_jiffie_prev;
static ci_list_def(intr_handler_head);

#define ci_handler_each(head, obj, field)		\
	for ((obj) = ci_container_of((head)->next, ci_typeof(*(obj)), field);	\
		 (ci_list_t *)&(obj)->field != (head);	\
		 (obj) = ci_container_of(((ci_list_t *)&(obj)->field)->next, ci_typeof(*(obj)), field))
		 

static void *pal_timer_thread_exec(void *data)
{
	u64 us_start, us_now;

	us_start = pal_clock_get_us();
	__ci_printf_block_lock();
	pal_imp_printf("pal timer thread started, tid=%lld, us=%llu", pal_get_tid(), us_start);
	pal_printfln(", PAL_JIFFIE_PER_SEC=%d, PAL_USEC_PER_JIFFIE=%d", PAL_JIFFIE_PER_SEC, PAL_USEC_PER_JIFFIE);
	__ci_printf_block_unlock();
	
	for (;;) {
#if 0	/* test purpose only */		
		pal_jiffie++;
		pal_usleep(PAL_USEC_PER_JIFFIE);
		ci_handler_each(&intr_handler_head, handler, link_buf)
			handler->callback(1, handler->data);
		break;
#else		
		pal_usleep(PAL_TIMER_INTERVAL);

		us_now = pal_clock_get_us();
		pal_jiffie = (us_now - us_start) / PAL_USEC_PER_JIFFIE;
#ifndef PAL_DISABLE_TIMER
		if (pal_jiffie != pal_jiffie_prev) {
				pal_timer_intr_handler_t *handler;
				ci_handler_each(&intr_handler_head, handler, link_buf)
					handler->callback((int)(pal_jiffie - pal_jiffie_prev), handler->data);

			pal_jiffie_prev = pal_jiffie;
		}
#endif
#endif		
	}

	return data;
}

int pal_timer_init()
{
	int rv = pthread_create(&pal_timer_thread, NULL, pal_timer_thread_exec, NULL); 
	ci_panic_if(rv, "pal_timer_init");

	ci_panic_if(ci_member_size(pal_timer_intr_handler_t, link_buf) != ci_sizeof(ci_list_t), 
				"ci_sizeof(link_buf)=%d, ci_sizeof(ci_list_t)=%d", 
				ci_member_size(pal_timer_intr_handler_t, link_buf),
				ci_sizeof(ci_list_t));

	return 0;
}

int pal_timer_finz()
{
	pthread_cancel(pal_timer_thread);
	return 0;
}

void pal_timer_register(pal_timer_intr_handler_t *handler)
{
	ci_assert(handler->callback);
	ci_list_add_tail(&intr_handler_head, (ci_list_t *)handler->link_buf);
}

