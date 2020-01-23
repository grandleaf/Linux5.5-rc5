/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_perf.h				PAL Performance Test
 *                                                          hua.ye@Hua Ye.com
 */
 
#include "ci.h"


static volatile int pal_perf_work_count_down;

static void *do_thread_work(void *__work)
{
	pal_perf_work_t *work = (pal_perf_work_t *)__work;

	if (work->setup_work)
		work->setup_work(work);

	ci_thread_set_affinity(*work->thread, work->id);
//	ci_printf("set affinity %d\n", work->id);

	ci_atomic_dec(&pal_perf_work_count_down);
	while (pal_perf_work_count_down)
		;

	ci_assert(work->start_work);
	work->start_work(work);

//	printf("Thread %d done!\n", work->id);
//	pthread_exit(work);
	return work;
}

void ci_perf_mt(int nr_thread, 
				int (*setup_work)(pal_perf_work_t *), 
				int (*start_work)(pal_perf_work_t *),
				int (*finish_work)(pal_perf_work_t *, int /* nr */, u64 ms /* microseconds */))
{
	int thread_idx, rc;
	void *status;
	pal_perf_work_t work[CI_PERF_NR_THREAD];
	pthread_t thread[CI_PERF_NR_THREAD];
	pthread_attr_t attr;

	ci_memzero(work, ci_sizeof(work));
	ci_memzero(thread, ci_sizeof(thread));
	ci_assert(nr_thread <= CI_PERF_NR_THREAD);

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	ci_printf("--> Starting %d threads ...\n", nr_thread);
	pal_perf_work_count_down = nr_thread;
	
	for(thread_idx = 0; thread_idx < nr_thread; thread_idx++) {
//		printf("    creating thread %d\n", thread_idx);
		work[thread_idx].thread		= &thread[thread_idx];
		work[thread_idx].id 		= thread_idx;
		work[thread_idx].setup_work = setup_work;
		work[thread_idx].start_work = start_work;
		
	  	rc = pthread_create(&thread[thread_idx], &attr, do_thread_work, &work[thread_idx]); 
	  	if (rc) {
		 	printf("ERROR; return code from pthread_create() is %d\n", rc);
		 	exit(-1);
		}
	}

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);

	while (pal_perf_work_count_down)
		;
	
	u64 begin_time = pal_clock_get_us();
	for(thread_idx = 0; thread_idx < nr_thread; thread_idx++) {
		rc = pthread_join(thread[thread_idx], &status);
	  	if (rc) {
		 	printf("ERROR; return code from pthread_join() is %d\n", rc);
		 	exit(-1);
		}
//	  	printf("Main: completed join with thread %d having a status of %d\n", thread_idx, ((pal_perf_work_t *)status)->rv);
	}

	u64 time_diff = pal_clock_get_us() - begin_time;
//	printf("Main: program completed. Exiting.\n");

	if (finish_work)
		finish_work(work, nr_thread, time_diff);
//	pthread_exit(NULL);
}

void ci_perf_eval_result(ci_perf_data_t *data)
{
	u64 time_diff = data->end_time - data->start_time;	
	ci_big_nr_parse_t tp = CI_TIME_US_PARSE;	

	tp.big_nr = time_diff;	
	ci_big_nr_parse(&tp);
	
	u64 mops = data->nr_io * 1000000 * (data->mul_factor) / time_diff;
	ci_big_nr_parse_t mops_p = { .flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_UNIT_VERBOSE, .big_nr = mops, .dec_len = 3, .unit = NULL };
	ci_big_nr_parse(&mops_p);		\
	ci_printf("--- %d.%03d %s (%lli) iops, ", mops_p.int_part, mops_p.dec_part, mops_p.unit, mops_p.big_nr);
	ci_printf(CI_PRN_FMT_TIME_SEC " Seconds (%lli usec)\n", CI_PRN_VAL_TIME_SEC(tp), tp.big_nr);		
}



