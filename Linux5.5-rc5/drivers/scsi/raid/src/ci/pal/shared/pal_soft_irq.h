/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_soft_irq.h				Soft IRQ
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_type.h"
#include "semaphore.h"

typedef struct 
{
	volatile int 		triggered;
	sem_t				sem;
} pal_soft_irq_t;

#define pal_soft_irq_init(s)			\
	do {	\
		sem_init(&(s)->sem, 0, 0);		\
		(s)->triggered = 0;		\
	} while (0)

/*	
#define bsem_post(b)			\
	do {	\
		if (unlikely(__sync_bool_compare_and_swap(&(b)->triggered, 0, 1))) 		\
			sem_post(&(b)->sem);		\
	} while (0)
*/

#if 1
#define pal_soft_irq_post(s)			\
	do {	\
		ci_mem_barrier();	\
		if (ci_unlikely(!(s)->triggered)) { 		\
			(s)->triggered = 1;		\
			sem_post(&(s)->sem);		\
		}		\
	} while (0)

#define pal_soft_irq_wait(s)			\
	do {	\
		if (ci_unlikely(!(s)->triggered)) 	\
			sem_wait(&(s)->sem);			\
	} while (0)

#define pal_soft_irq_clear(s)		\
	do {	\
		(s)->triggered = 0;	\
		ci_mem_barrier();	\
	} while (0)
#else
#define pal_soft_irq_post(s)			\
	do {	\
		int __old_val__ = __sync_val_compare_and_swap(&(s)->triggered, 0, 1);	\
		if (ci_unlikely(__old_val__ == 0)) {	\
			sem_post(&(s)->sem);		\
		}	\
	} while (0)
#define pal_soft_irq_wait(s)			\
	do {	\
		int __old_val__ = __sync_val_compare_and_swap(&(s)->triggered, 1, 0);	\
		if (ci_unlikely(__old_val__ == 0)) 	\
			sem_wait(&(s)->sem);			\
	} while (0)
#define pal_soft_irq_clear(s)		\
	do {	\
	} while (0)
#endif

		
#define pal_soft_irq_finz(s)			sem_destroy(&(s)->sem)
#define pal_soft_irq_post_raw(s)		sem_post(&(s)->sem)
#define pal_soft_irq_wait_raw(s)		sem_wait(&(s)->sem)


