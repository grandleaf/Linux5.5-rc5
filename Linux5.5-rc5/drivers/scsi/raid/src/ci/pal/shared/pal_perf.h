/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_perf.h				PAL Performance Test
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_type.h"

#define CI_PERF_NR_THREAD					128
#define	CI_PERF_NR_WORK_ARG					8



typedef struct __pal_perf_work_t pal_perf_work_t;

struct __pal_perf_work_t {
	pthread_t	*thread;
	int			 id;
	int			 rv;

	void		*arg[CI_PERF_NR_WORK_ARG];
	int (*setup_work)(pal_perf_work_t *);
	int (*start_work)(pal_perf_work_t *);
};


/*
 *	performance test wrapper
 */
typedef struct {
	int				mul_factor;		/* nr_io * mul_factor == total io */
	u64				nr_io;			/* number of IOs */
	u64				start_time;
	u64				end_time;
} ci_perf_data_t;

void ci_perf_eval_result(ci_perf_data_t *data);

#define ci_perf_eval_start(data, show)		\
	do {		\
		(data)->mul_factor || ((data)->mul_factor = 1);	\
		if (show)	\
			ci_printf("+++ Starting Performance Test...   loop=%lli, multiply=%d\n", (data)->nr_io, (data)->mul_factor); 	\
		(data)->start_time = pal_clock_get_us();		\
	} while (0)
#define ci_perf_eval_end(data, show)	\
	do {	\
		(data)->end_time = pal_clock_get_us();		\
		if (show)		\
			ci_perf_eval_result(data);		\
	} while (0)

#define ci_perf_eval(nr_loop, mul_factor, ...)	\
	do {	\
		ci_printf("+++ Starting Performance Test...   loop=%lli, multiply=%d\n", (u64)(nr_loop), mul_factor); \
		u64 __start_time__ = pal_clock_get_us();		\
		ci_loop(__loop_cnt__, nr_loop) {	\
			__VA_ARGS__;		\
		}		\
		u64 __end_time__ = pal_clock_get_us();		\
		u64 __time_diff__ = __end_time__ - __start_time__;	\
		\
		ci_big_nr_parse_t __tp__ = CI_TIME_US_PARSE;	\
		__tp__.big_nr = __time_diff__;	\
		ci_big_nr_parse(&__tp__);		\
		\
		u64 __mops__ = (u64)(nr_loop) * 1000000 * (mul_factor) / __time_diff__;		\
		ci_big_nr_parse_t mops_p = { .flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_UNIT_VERBOSE, \
									  .big_nr = __mops__, .dec_len = 3, .unit = NULL };	\
		ci_big_nr_parse(&mops_p);		\
		ci_printf("--- %d.%03d %s (%lli) iops, ", mops_p.int_part, mops_p.dec_part, mops_p.unit, mops_p.big_nr);	\
		ci_printf("--> " CI_PRN_FMT_TIME_SEC " Seconds (%lli usec)\n", CI_PRN_VAL_TIME_SEC(__tp__), __tp__.big_nr);		\
	} while (0)


void ci_perf_mt(int nr_thread, 
				int (*setup_work)(pal_perf_work_t *), 
				int (*start_work)(pal_perf_work_t *),
				int (*finish_work)(pal_perf_work_t *, int /* nr */, u64 ms /* microseconds */));


