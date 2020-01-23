#include "ci.h"
//#include "pal_numa.h"

void pal_time_str(char *buf, int size, int local)
{
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	int milli = cur_time.tv_usec / 1000;
	
	char date_time[80];
	strftime(date_time, sizeof(date_time), "%Y-%m-%d_%H:%M:%S", 
			 local ? localtime(&cur_time.tv_sec) : gmtime(&cur_time.tv_sec));
	
	buf[0] = 0;
	snprintf(buf, size, "%s_%03d.%03d", date_time, milli, (int)(cur_time.tv_usec - 1000 * milli));
}

void pal_utc_time_str(char *buf, int size)
{
	pal_time_str(buf, size, 0);
}

void pal_local_time_str(char *buf, int size)
{
	pal_time_str(buf, size, 1);
}

void pal_nsleep_no_ctx(long nsec)
{
    time_t sec = (time_t)(nsec / 1000000000);
    nsec -= sec * 1000000000;
    struct timespec ts = { .tv_sec = sec, .tv_nsec = nsec };

    while (nanosleep(&ts, &ts))
		;
}

void pal_nsleep(long nsec)
{
	ci_assert(!ci_sched_ctx(), "sleep in the scheduler's context is not allowed!");
	pal_nsleep_no_ctx(nsec);
}

void pal_usleep(long usec)
{
    time_t sec = (time_t)(usec / 1000000);
    long nsec = (usec - sec * 1000000) * 1000;
    struct timespec ts = { .tv_sec = sec, .tv_nsec = nsec };

	ci_assert(!ci_sched_ctx(), "sleep in the scheduler's context is not allowed!");
	
    while (nanosleep(&ts, &ts))
		;
}

