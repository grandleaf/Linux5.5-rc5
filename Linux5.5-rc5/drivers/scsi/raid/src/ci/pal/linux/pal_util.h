/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_util.h				PAL macros
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"

#define PAL_CPU_CACHE_LINE_MASK						(~PAL_CPU_CACHE_LINE_OFFSET)
#define PAL_CPU_CACHE_LINE_OFFSET					((uintptr_t)PAL_CPU_CACHE_LINE_SIZE - 1)

#define ci_type_is_signed(x)						(((ci_typeof(x))-1) < 0)
#define ci_type_is_unsigned(x)						(((ci_typeof(x))-1) >= 0)


#define ci_pragma_no_warning(...)					ci_m_each(__ci_pragma_no_warning, __VA_ARGS__)
#define __ci_pragma_no_warning(x)					gcc_pragma(GCC diagnostic ignored x)
#define ci_pragma_push()							gcc_pragma_push()
#define ci_pragma_pop()								gcc_pragma_pop()
#define ci_pragma_error(x)							gcc_pragma(GCC diagnostic error x)

#define gcc_pragma(...) 							_Pragma(#__VA_ARGS__)
#define gcc_pragma_push()							gcc_pragma(GCC diagnostic push)
#define gcc_pragma_pop()							gcc_pragma(GCC diagnostic pop)

#define CI_WARN_UNUSED_FUNCTION						"-Wunused-function"
#define CI_WARN_UNUSED_VARIABLE						"-Wunused-variable"
#define CI_WARN_UNUSED_LOCAL_TYPEDEFS				"-Wunused-local-typedefs"
#define CI_WARN_FORMAT								"-Wformat"

#define ci_static_assert(cond)						__ci_static_assert2(__COUNTER__, cond) 
#define __ci_static_assert2(cnt, cond)				__ci_static_assert3(cnt, cond)
#define __ci_static_assert3(cnt, cond)				typedef char __ci_sassert_ ## cnt [1 - 2 * !!!(cond)]		/* dummy */


#define ci_type_size_check(type, size)				ci_static_assert(ci_sizeof(type) == (size))

//		_Pragma(GCC diagnostic push) 




#ifdef NDEBUG
#define pal_assert(x, ...)							ci_nop()
#else
/*
#define pal_assert(x, ...)			\
	({	\
		if (!(x)) {			\
			ci_printf("\nASSERTION FAILED: \"%s\"\n", #x);		\
			ci_print_exp_loc();		\
			ci_m_if(ci_m_has_args(__VA_ARGS__)) ( ci_printf(__VA_ARGS__); ci_printf("\n") );		\
			ci_printf("\n");	\
			raise(SIGSEGV);		\
		}		\
		\
		0;	\
	})	
*/

#define pal_assert(x, ...)			\
	({	\
		if (!(x)) {			\
			printf("\nASSERTION FAILED: \"%s\"\n", #x);		\
			printf("Exception at:\n    file : %s\n    func : %s()\n    line : %d\n", \
				   __FILE__, __FUNCTION__, __LINE__);	\
			ci_m_if(ci_m_has_args(__VA_ARGS__)) ( __ci_printf_check(__VA_ARGS__); printf(__VA_ARGS__); printf("\n") );		\
			printf("\n");	\
			raise(SIGSEGV);		\
		}		\
		\
		0;	\
	})	
	
#endif

#define pal_panic(...)		\
	({	\
		ci_print_exp_loc();		\
		ci_m_if(ci_m_has_args(__VA_ARGS__)) ( ci_printf(__VA_ARGS__); ci_printf("\n") );		\
		ci_printf("\nPANIC TRIGGERED!\n");		\
		raise(SIGSEGV); 	\
		\
		0;	\
	})

#define pal_panic_if(x, ...)		\
	({	\
		if (x) {			\
			ci_printf("\nPANIC_IF: \"" #x "\"\n");		\
			pal_panic(__VA_ARGS__);		\
		}		\
		\
		0;	\
	})

#define ci_numa_mem_loc_check(ptr, numa_id)		\
	({	\
		extern int pal_numa_id_by_ptr(void *);		\
		extern int PAL_NUMA_NODE_0_MEM_ALLOC_ONLY();		\
		\
		int __numa_id__ = pal_numa_id_by_ptr(ptr);		\
		if (PAL_NUMA_NODE_0_MEM_ALLOC_ONLY())	\
			pal_assert(__numa_id__ == 0);	\
		else	\
			pal_assert(__numa_id__ == numa_id);	\
	})


#define ci_m_vc_expand(...)							__VA_ARGS__
#define __MSVC_COMMA__				

//#define pal_rand(l, u)								(rand() % ((u) - (l)) + (l))


#if 0
#define pal_rand_mask(bit)		((1ull << (bit)) - 1)

#define pal_rand_shr(l, u)		\
	({	\
		assert(RAND_MAX == pal_rand_mask(31));		\
		assert(RAND_MAX == 0x7FFFFFFF);	\
		\
		u64 __rnd_val__, __rnd_len__ = u;		\
		\
		if (__rnd_len__ <= pal_rand_mask(31))	\
			__rnd_val__ = rand();	\
		else if (__rnd_len__ <= pal_rand_mask(62))		\
			__rnd_val__ = (rand() << 31ull) | rand();	\
		else	\
			__rnd_val__ = (((rand() << 31ull) | rand()) << 31ull) | rand();	\
		\
		(__rnd_val__ % ((u) - (l)) + (l)) ;		\
	})
#else
#define pal_rand_shr(l, u)		\
	({	\
		u64 __rand_u__ = (u64)(u), __rand_l__ = (u64)(l);	\
		u64 __rand_len__ = (__rand_u__) - (__rand_l__);	\
		u64 __rand_val__ = __rand_len__ <= CI_U32_MAX ? pal_rand32_shr() : pal_rand64_shr();	\
		(ci_typeof(u))(__rand_val__ % ((__rand_u__) - (__rand_l__)) + (__rand_l__)) ;		\
	})
#define pal_rand(ctx, l, u)		\
	({	\
		pal_rand_ctx_t *__pal_rand_ctx__ = pal_rand_ctx_by_ctx(ctx);	 	\
		u64 __rand_u__ = (u64)(u), __rand_l__ = (u64)(l);	\
		u64 __rand_len__ = (__rand_u__) - (__rand_l__);	\
		u64 __rand_val__ = __rand_len__ <= CI_U32_MAX ? pal_rand32(__pal_rand_ctx__) : pal_rand64(__pal_rand_ctx__);	\
		(ci_typeof(u))(__rand_val__ % ((__rand_u__) - (__rand_l__)) + (__rand_l__)) ;		\
	})	
#endif


#define ci_pause()									getchar()


#define pal_imp_printf(...)				pal_ntc_printf(CI_PR_NTC_IMP, 		__VA_ARGS__)	
#define pal_warn_printf(...)			pal_ntc_printf(CI_PR_NTC_WARN, 		__VA_ARGS__)	
#define pal_err_printf(...)				pal_ntc_printf(CI_PR_NTC_ERR, 		__VA_ARGS__)

#define pal_imp_printfln(...)			pal_ntc_printfln(CI_PR_NTC_IMP,		__VA_ARGS__)	
#define pal_warn_printfln(...)			pal_ntc_printfln(CI_PR_NTC_WARN,	__VA_ARGS__)	
#define pal_err_printfln(...)			pal_ntc_printfln(CI_PR_NTC_ERR, 	__VA_ARGS__)


#define __pal_printf_ntc(ntc, ...)	/* print with "notice" */	\
	({	\
		int __pal_printf_rv__;	\
		\
		__ci_printf_line_lock(); 	\
		__ci_printf_set_loc(__FILE__, __func__, __LINE__, "pal", ntc);	\
		__pal_printf_rv__ = __ci_raw_printf(__VA_ARGS__);	\
		__ci_printf_line_unlock();	\
		__pal_printf_rv__;		\
	})
#define pal_printf(...)		/* with format checking */	\
	({	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			__ci_printf_check(__VA_ARGS__);		\
			__pal_printf_ntc(' ', __VA_ARGS__);		\
		)(0;)	\
	})
#define pal_ntc_printf(ntc, ...)		/* ntc(notice): print with a * to indicate this message is important */	\
	({	\
		int __pal_ntc_printf_rv__;	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			__ci_printf_check(__VA_ARGS__);		\
			__pal_ntc_printf_rv__ = __pal_printf_ntc(ntc, __VA_ARGS__);	\
		)(	\
			__pal_ntc_printf_rv__ = __pal_printf_ntc(ntc, "%s", "");	\
		)	\
		__pal_ntc_printf_rv__;	\
	})

#define pal_printfln(fmt, ...)				pal_printf(fmt "\n" ci_m_if(ci_m_has_args(__VA_ARGS__))(, __VA_ARGS__))
#define pal_ntc_printfln(ntc, fmt, ...)		pal_ntc_printf(ntc, fmt "\n" ci_m_if(ci_m_has_args(__VA_ARGS__))(, __VA_ARGS__))

#if 0
#define pal_printfln(...)		\
	({		\
		int __pal_printfln__ = pal_printf(__VA_ARGS__);		\
		__pal_printfln__ += __ci_raw_printf("\n");		\
	})
#define pal_ntc_printfln(ntc, ...)		\
	({	\
		int __pal_ntc_printfln__ = pal_ntc_printf(ntc, __VA_ARGS__);		\
		__pal_ntc_printfln__ += __ci_raw_printf("\n");		\
	})
#endif	


#define pal_get_tid()								((u64)syscall(SYS_gettid))

#define pal_perf_prep()	\
	do {	\
		u32 __cycles_high__, __cycles_low__;	\
		\
		asm volatile (	\
			"cpuid\n\t"		\
			"rdtsc\n\t"		\
			"mov %%edx, %0\n\t"		\
			"mov %%eax, %1\n\t": "=r" (__cycles_high__), "=r" (__cycles_low__)::"%rax", "%rbx", "%rcx", "%rdx"	\
		);	\
		\
		asm volatile (	\
			"cpuid\n\t"		\
			"rdtsc\n\t"		\
			"cpuid\n\t"		\
			"rdtsc\n\t"		\
			"mov %%edx, %0\n\t"		\
			"mov %%eax, %1\n\t": "=r" (__cycles_high__), "=r" (__cycles_low__)::"%rax", "%rbx", "%rcx", "%rdx"	\
		);	\
		\
		asm volatile (		\
			"cpuid\n\t"		\
			"rdtsc\n\t"::: "%rax", "%rbx", "%rcx", "%rdx"	\
		);	\
	} while (0)
#define pal_perf_counter() \
	({	\
		u32 __cycles_high__, __cycles_low__;	\
		\
		asm volatile (	\
			"CPUID\n\t"		\
			"RDTSC\n\t"		\
			"mov %%edx, %0\n\t"		\
			"mov %%eax, %1\n\t": "=r" (__cycles_high__), "=r" (__cycles_low__):: "%rax", "%rbx", "%rcx", "%rdx"	\
		);	\
		\
		((u64)__cycles_high__ << 32) | __cycles_low__;	\
	})
	
/* this is a "relax" version, which not as precise as the non-relax version, but runs faster */	
#define pal_perf_counter_relax()	\
	({	\
		u32 __cycles_high__, __cycles_low__;	\
		\
		asm volatile (	\
			"RDTSC\n\t"		\
			"mov %%edx, %0\n\t"		\
			"mov %%eax, %1\n\t": "=r" (__cycles_high__), "=r" (__cycles_low__):: "%rax", "%rbx", "%rcx", "%rdx"	\
		);	\
		\
		((u64)__cycles_high__ << 32) | __cycles_low__;	\
	})



#define pal_timestamp()								__rdtsc()

u8 *pal_malloc(u64 size);








