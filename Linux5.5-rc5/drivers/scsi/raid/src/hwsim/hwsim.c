/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * hwsim.c			HW Simulator/External Interrupt/External Worker
 *                                                          hua.ye@Hua Ye.com
 */
#include "hwsim.h"

static hwsim_info_t *__info;

void *hwsim_worker_entry(void *data)
{
	hwsim_task_t *task;
	hwsim_worker_t *worker = (hwsim_worker_t *)data;
	
	__info = worker->info;
	sem_post(&__info->sem_thread_create);

	for (;;) {
		sem_wait(&__info->sem_task);
		
		ci_slk_protected(&__info->lock, {
			task = ci_list_del_head_obj(&__info->head, hwsim_task_t, link);
		});
		
		ci_assert(task && task->callback);
		if (task->pre_delay_ns)
			pal_nsleep(task->pre_delay_ns);

		if (task->do_work)
			task->do_work(task, task->data);

		if (task->post_delay_ns)
			pal_nsleep(task->post_delay_ns);

//		ci_printf("worker%d callback\n", worker->id);
		task->callback(task, task->data);
	}

	return data;
}

void hwsim_task_submit(hwsim_task_t *task)
{
	ci_slk_protected(&__info->lock, {
		ci_list_add_tail(&__info->head, &task->link);
	});

	sem_post(&__info->sem_task);
}


