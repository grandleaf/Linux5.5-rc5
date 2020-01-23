/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * hwsim.h			HW Simulator/External Interrupt/External Worker
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

#define HWSIM_WORKER_NR				16

#define hwsim_info(mod)									((hwsim_info_t *)ci_mod_mem_shr(mod))

typedef struct __hwsim_task_t hwsim_task_t;
typedef struct __hwsim_info_t hwsim_info_t;

struct __hwsim_task_t {
	int						 pre_delay_ns;				/* nanosleep, sleep before do_work() */
	int						 post_delay_ns;				/* nanosleep, sleep after do_work() */
	
	void					*data;						/* for user */
	void (*do_work)(hwsim_task_t *, void *);	/* routine to simulator hardware operations */
	void (*callback)(hwsim_task_t *, void *);	/* callback when done */
	
	ci_list_t				 link;
};

typedef struct {
	int						 id;
	pthread_t				 handle;
	hwsim_info_t			*info;
} hwsim_worker_t;

struct __hwsim_info_t {
	hwsim_worker_t			 hw_worker[HWSIM_WORKER_NR];
	ci_slk_t				 lock;
	ci_list_t				 head;		/* request head */
	sem_t					 sem_task;
	sem_t					 sem_thread_create;
};


void *hwsim_worker_entry(void *data);
void hwsim_task_submit(hwsim_task_t *task);

