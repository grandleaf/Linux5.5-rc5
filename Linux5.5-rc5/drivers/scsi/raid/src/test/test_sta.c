#include "ci.h"


typedef struct {
	u64							 total;
} test_sta_pending_t;

typedef struct {
	u64							 alloc;
	u64							 free;
} test_sta_acc_t;

static ci_sta_cfg_hist_t cfg_hist[] = {
	{ "1 second",			1000,				60 	},
	{ "5 seconds",			5000,				120 },
	{ "1 minute",			60000,				60 	},
	CI_EOT
};



		
static ci_sta_dump_dpt_t dump_dpt[] = ci_sta_dump_dpt_maker(test_sta_acc_t, 
	alloc, 
	free
);



static ci_sta_cfg_t cfg = {
	.name				= "test_sta",
	.node_id			= 0,
	.trip_name			= ci_str_ary("Trip A", "Trip B", "Trip C"),
	.hist				= cfg_hist,

	.pending_size		= ci_sizeof(test_sta_pending_t),
	.acc_size			= ci_sizeof(test_sta_acc_t)
};

/* hack, disable the context check */
#undef ci_sched_ctx_check
#define ci_sched_ctx_check(...)		 ci_nop()

#define test_sta_acc_add(sta, name, val)	ci_sta_acc_add(sta, ci_offset_of(test_sta_acc_t, name), val)
#define test_sta_acc_inc(sta, name, val)	test_sta_acc_add(sta, name, 1)

void *test_sta_thread(void *data)
{
	ci_sta_t *sta = (ci_sta_t *)data;
	ci_imp_printf("test_sta_thread started\n");

	for (;;) {
		test_sta_acc_add(sta, alloc, 1);
		pal_msleep(1);
		test_sta_acc_add(sta, free, 2);
	}

	return data;
}

void test_sta()
{
	ci_sta_t *sta;
	pthread_t thd;

	ci_printfln("test sta");
	sta = ci_sta_create(&cfg);

	pthread_create(&thd, NULL, test_sta_thread, sta);
	ci_sta_set_hist_dump(sta, "1 second", dump_dpt, 1, 0);
}



