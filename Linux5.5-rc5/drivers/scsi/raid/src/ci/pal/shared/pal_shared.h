/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_shared.h				PAL shared utilities
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_perf.h"
#include "pal_soft_irq.h"
#include "pal_worker.h"
#include "pal_sock_srv.h"
#include "pal_mod_cfg.h"
#include "pal_timer.h"
#include "pal_rand.h"
//#include "pal_numa.h"

extern u64 PAL_CYCLE_PER_SEC;
extern u64 PAL_CYCLE_OVERHEAD;

#define ci_const_expr(x)			__builtin_constant_p(x)

/*
 *	spinlock things
 */
#ifdef WIN_SIM 
#define ci_mem_barrier()			do {} while (0)
typedef pthread_spinlock_t			__ci_slk_t;

#define __ci_slk_init(l)		\
	({	\
		int __rv__ __attribute__((unused)) = pthread_spin_init(l, PTHREAD_PROCESS_PRIVATE);		\
		ci_assert(__rv__ == 0);	\
		l;	\
	})
#define __ci_slk_lock(l)		\
	({	\
		int __rv__ __attribute__((unused)) = pthread_spin_lock(l);		\
		ci_assert(__rv__ == 0);	\
		l;	\
	})
#define __ci_slk_unlock(l)	\
	({	\
		int __rv__ __attribute__((unused)) = pthread_spin_unlock(l);	\
		ci_assert(__rv__ == 0);	\
		l;	\
	}) 
#define __ci_slk_try_lock(l)	/* 1 means success */	\
	({	\
		int __rv__ = pthread_spin_trylock(l);	\
		!__rv__;	\
	}) 
#define __ci_slk_try_lock_timeout(l, ns)	\
	({	\
		int __ci_slk_try_lock_timeout_val__;	\
		u64 __ci_slk_try_lock_expire__ = pal_clock_get_ns() + (ns);	\
		while((__ci_slk_try_lock_timeout_val__ = pthread_spin_trylock(l))) {		\
			if (pal_clock_get_ns() > __ci_slk_try_lock_expire__)	\
				break;	\
			_mm_pause();	\
		}	\
		!__ci_slk_try_lock_timeout_val__;	\
	})
#else
#define ci_mem_barrier()			asm volatile ("")
typedef int __ci_slk_t;

#define __ci_slk_init(l)			(*(l) = 0)
#define __ci_slk_lock(l)		\
	do {	\
		while(!__sync_bool_compare_and_swap((l), 0, 1))		\
			while(ci_unlikely(*(l))) _mm_pause();	\
	} while (0)
#define __ci_slk_unlock(l)	\
	do {	\
		ci_mem_barrier();	/* memory barrier */	\
		*(l) = 0;		\
	} while (0)
#define __ci_slk_try_lock(l)	/* 1 means success */	\
		__sync_bool_compare_and_swap((l), 0, 1)
#define __ci_slk_try_lock_timeout(l, ns)	\
	({	\
		int __ci_slk_try_lock_timeout_val__;	\
		u64 __ci_slk_try_lock_expire__ = pal_clock_get_ns() + (ns);	\
		while(!(__ci_slk_try_lock_timeout_val__ = __sync_bool_compare_and_swap((l), 0, 1))) {		\
			if (pal_clock_get_ns() > __ci_slk_try_lock_expire__)	\
				break;	\
			_mm_pause();	\
		}	\
		__ci_slk_try_lock_timeout_val__;	\
	})
#endif


#ifdef CI_DEBUG

#define CI_SLK_MASK					 0x7878787800000000ull
#define CI_SLK_UNLOCKED				 (CI_SLK_MASK + 1)
#define CI_SLK_LOCKED				 (CI_SLK_MASK + 2)

typedef struct {
	u64								 tag;
	__ci_slk_t						 lock;
} ci_slk_t;

#define ci_slk_init(l)		\
	do {	\
		ci_obj_zero(l);		\
		(l)->tag = CI_SLK_UNLOCKED;		\
		__ci_slk_init(&(l)->lock);	\
	} while (0)
#define ci_slk_lock(l)		\
	do {	\
		ci_assert(((l)->tag & CI_SLK_MASK) == CI_SLK_MASK, "use \"ci_slk_t\" without init");	\
		__ci_slk_lock(&(l)->lock);	\
		(l)->tag = CI_SLK_LOCKED;	\
	} while (0)
#define ci_slk_unlock(l)	\
	do {	\
		ci_assert(((l)->tag & CI_SLK_MASK) == CI_SLK_MASK, "use \"ci_slk_t\" without init");	\
		ci_assert((l)->tag == CI_SLK_LOCKED, "double \"ci_slk_t\" unlock or unlock without lock");		\
		(l)->tag = CI_SLK_UNLOCKED;	\
		__ci_slk_unlock(&(l)->lock);	\
	} while (0)
#define ci_slk_try_lock(l)	\
	({	\
		int __ci_slk_try_lock_val__;	\
		\
		ci_assert(((l)->tag & CI_SLK_MASK) == CI_SLK_MASK, "use \"ci_slk_t\" without init");	\
		if ((__ci_slk_try_lock_val__ = __ci_slk_try_lock(&(l)->lock)))	\
			(l)->tag = CI_SLK_LOCKED;	\
		__ci_slk_try_lock_val__;	\
	})
#define ci_slk_try_lock_timeout(l, ns)	\
	({	\
		int __ci_slk_try_lock_val__;	\
		\
		ci_assert(((l)->tag & CI_SLK_MASK) == CI_SLK_MASK, "use \"ci_slk_t\" without init");	\
		if ((__ci_slk_try_lock_val__ = __ci_slk_try_lock_timeout(&(l)->lock, ns)))	\
			(l)->tag = CI_SLK_LOCKED;	\
		__ci_slk_try_lock_val__;	\
	})
#else
typedef __ci_slk_t						ci_slk_t;
#define ci_slk_init(l)					__ci_slk_init(l)
#define ci_slk_lock(l)					__ci_slk_lock(l)
#define ci_slk_unlock(l)				__ci_slk_unlock(l)
#define ci_slk_try_lock(l)				__ci_slk_try_lock(l)
#define ci_slk_try_lock_timeout(l, ns)	__ci_slk_try_lock_timeout(l, ns)		/* nano second */
#endif


#define ci_slk_protected(l, ...)		\
	({	\
		ci_slk_lock(l);		\
		__VA_ARGS__;		\
		ci_slk_unlock(l);	\
	})




#ifdef WIN_SIM
extern int clock_gettime(int dummy, struct timespec *ct);
#define pal_clock_gettime(clock_id, time_spec)			clock_gettime(3, time_spec)
#else
#define pal_clock_gettime(clock_id, time_spec)			clock_gettime(clock_id, time_spec)
#endif

/* in microsecond */
#define pal_clock_get_us()		\
({	\
	struct timespec __ts__;		\
	\
	pal_clock_gettime(CLOCK_MONOTONIC, &__ts__);	\
	(u64)(((__ts__.tv_sec * 1000000000) + __ts__.tv_nsec) / 1000);	\
})

#define pal_clock_get_ns()		\
({	\
	struct timespec __ts__;		\
	\
	pal_clock_gettime(CLOCK_MONOTONIC, &__ts__);	\
	(u64)((__ts__.tv_sec * 1000000000) + __ts__.tv_nsec);	\
})


/* checking functions */
#ifdef WIN_SIM
static inline void __ci_printf_check(const char *format, ...) {}
static inline void __ci_sprintf_check(char *buf, int size, const char *fmt, ...) {}
static inline void __ci_vsprintf_check(char *buf, int size, const char *fmt, va_list args) {}

static inline void __ci_sscanf_check(const char *buf, const char *fmt, ...) {}
static inline void __ci_vsscanf_check(const char *buf, const char *fmt, va_list args) {}
#elif defined(__GNUC__)
__attribute__((format(printf, 1, 2))) static inline void __ci_printf_check(const char *format, ...) {}
__attribute__((format(printf, 3, 4))) static inline void __ci_sprintf_check(char *buf, int size, const char *fmt, ...) {}
__attribute__((format(printf, 3, 0))) static inline void __ci_vsprintf_check(char *buf, int size, const char *fmt, va_list args) {}

__attribute__((format(printf, 2, 3))) static inline void __ci_sscanf_check(const char *buf, const char *fmt, ...) {}
__attribute__((format(printf, 2, 0))) static inline void __ci_vsscanf_check(const char *buf, const char *fmt, va_list args) {}
#endif

#ifdef WIN_SIM
	#pragma section(".mod$a", read, write)
	#pragma section(".mod$z", read, write)
	#define CI_MODULE_SECTION_NAME					".mod$m"
#else
	#define CI_MODULE_SECTION_NAME					"ci_mod_section"
#endif

#define ci_mod_def(x, ...)		\
	static ci_mod_t x = __VA_ARGS__;		\
    static ci_mod_t *__##x __attribute__((section(CI_MODULE_SECTION_NAME), used)) = &x




