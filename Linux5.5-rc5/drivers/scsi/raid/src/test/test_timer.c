#include "ci.h"

static void timer_callback(ci_sched_ctx_t *ctx, void *data)
{
	u64 jiffie;
	ci_timer_t *timer = (ci_timer_t *)data;
	int diff = (int)(pal_jiffie - timer->jiffie_expire);
	
	if (diff)	
		ci_warn_printfln("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! diff = %d", diff);
	ci_printf("--- 	ctx=%p, jiffie=%llu, pal_jiffie=%llu, expire=%llu, diff=%d\n", 
				ctx, timer->jiffie, pal_jiffie, timer->jiffie_expire, diff);

//	ci_timer_worker_data_t *tm_data = ci_worker_data_by_ctx(ctx, CI_WDID_TIMER);
//	ci_timer_wheel_dump(tm_data);


//	jiffie = ci_rand_shr_i(0x01, 0xFF);
//	jiffie = ci_rand_shr_i(0x0100, 0xFFFF);
//	jiffie = ci_rand_shr_i(0x010000, 0xFFFFFF);
//	jiffie = ci_rand_shr_i(0x01000000, 0xFFFFFFFF);
//	jiffie = ci_rand_shr_i(0x01, 0xFFFFFFFF);
	jiffie = ci_rand_shr_i(0x01, 0xFFFFF);

	timer->msec = jiffie * PAL_MSEC_PER_JIFFIE;
	ci_timer_add(ctx, timer);	/* requeue */
}

static void simple_timer_test()
{
	ci_here();
	
	static ci_timer_t tm = {
		.msec 		= 1,
		.data 		= &tm,
		.callback 	= timer_callback,
	};

	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);
}

static void periodic_timer_callback(ci_sched_ctx_t *ctx, void *data)
{
	static int tick = 1;
	ci_timer_t *timer = (ci_timer_t *)data;

	ci_printf("tick %d\n", tick);

	if (tick++ == 60)
		ci_timer_del(timer);	/* must be invoked in the schedule context */
}

static void periodic_timer_test()
{
	static ci_timer_t tm = {
		.flag		= CI_TIMER_PERIODIC,
		.msec 		= 1000,
		.data 		= &tm,
		.callback 	= periodic_timer_callback,
	};

	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);
}

/* ~95M add/del measured in axnpsim */
static void performance_timer_callback(ci_sched_ctx_t *ctx, void *data)
{
#ifdef CI_DEBUG
#define TIMER_LOOP			10000000ull	
#else
#define TIMER_LOOP			1000000000ull
#endif

	ci_timer_t *timer = (ci_timer_t *)data;

	timer->msec = 30 * 1000;		/* io timeout value? */

	ci_perf_eval(TIMER_LOOP, 1, {
		ci_timer_add(ctx, timer);	
		ci_timer_del(timer);
	});
}

static void timer_performance_test()
{
	static ci_timer_t tm = {
		.msec 		= 1,
		.data 		= &tm,
		.callback 	= performance_timer_callback,
	};

	ci_timer_ext_add(ci_sched_ctx_by_id(0, 0), &tm);
}

void test_timer()
{
	ci_printfln("Test Timer");

//	simple_timer_test();
	periodic_timer_test();
//	timer_performance_test();
}

