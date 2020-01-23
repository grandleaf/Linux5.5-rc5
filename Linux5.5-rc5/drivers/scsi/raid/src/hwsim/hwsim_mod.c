/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * hwsim_mod.c			HW Simulator/External Interrupt/External Worker
 *                                                          hua.ye@Hua Ye.com
 */
#include "hwsim.h"

static void hwsim_init(ci_mod_t *mod, ci_json_t *json)
{
	hwsim_info_t *info = hwsim_info(mod);

	ci_obj_zero(info);
	ci_slk_init(&info->lock);
	ci_list_init(&info->head);
	sem_init(&info->sem_thread_create, 0, -1);
	sem_init(&info->sem_task, 0, 0);

	ci_loop(idx, HWSIM_WORKER_NR) {
		hwsim_worker_t *worker = &info->hw_worker[idx];
		worker->id = idx;
		worker->info = info;
		pthread_create(&worker->handle, NULL, hwsim_worker_entry, worker);
	}

	ci_mod_init_done(mod, json);
}

static void hwsim_start(ci_mod_t *mod, ci_json_t *json)
{
	hwsim_info_t *info = hwsim_info(mod);

	sem_wait(&info->sem_thread_create);
	ci_printfln("%d hw_sim worker thread created", HWSIM_WORKER_NR);

	ci_mod_start_done(mod, json);
}

ci_mod_def(mod_hw_sim, {
	.name 			= "hwsim",
	.desc 			= "hardware simulator",
	.order_start 	= 10,


	.vect = {
		[CI_MODV_INIT] 		= hwsim_init,
		[CI_MODV_START] 	= hwsim_start,
	},

	.mem = {
		.size_shr			= ci_sizeof(hwsim_info_t),
	},		
});


