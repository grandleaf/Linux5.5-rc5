/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_wrap.h				PAL wrappers for functions
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal_cfg.h"
#include "pal_imp.h"

#define ci_sizeof(x)							((int)sizeof(x))
#define ci_memcpy(dst, src, n)					memcpy(dst, src, (size_t)(n))
#define ci_memset(ptr, val, n)					memset(ptr, val, (size_t)(n))
#define ci_memzero(ptr, n)						ci_memset(ptr, 0, n)
#define ci_memcmp(a, b, n)						memcmp(a, b, (size_t)(n))
#define ci_memmove(a, b, n)						memmove(a, b, (size_t)(n))
#define ci_memequal(a, b, n)					(!ci_memcmp(a, b, n))

#define ci_strlen(x)							((int)strlen(x))
#define ci_strequal(a, b)						(!strcmp(a, b))
#define ci_strnequal(a, b, n)					(!strncmp(a, b, n))
#define ci_strnlen(x, n)						((int)strnlen(x, n))
#define ci_strstr(a, b)							strstr(a, b)
#define ci_strcmp(a, b)							strcmp(a, b)
#define ci_strchrnul(a, b)						strchrnul(a, b)

#define ci_strlcpy(a, b, n)		/* safe version of strcpy() */ \
	({	\
		char *__a__ = (char *)(a), *__b__ = (char *)(b);		\
		int __n__ = (int)(n), __cpy_len__ = 0;		\
		ci_assert(__a__ && __b__ && ((__n__) > 0));		\
		\
		while ((__cpy_len__++ < __n__ - 1) && (*__a__++ = *__b__++))	\
			;	\
		\
		(__cpy_len__ >= __n__ - 1) && (*__a__ = 0);	\
		__cpy_len__;	\
	})

#ifndef LIB_FAST_RAID
#define __pal_printf(...)			\
	({		\
		int __pal_printf_rv__;		\
		if (!(ci_printf_info->flag & CI_PRF_CONSOLE)) {	\
			u8 *__printf_range_buf_curr__ = ci_printf_info->range_mem_buf.curr;	\
			__pal_printf_rv__ = (int)(ci_mem_printf(&ci_printf_info->range_mem_buf, __VA_ARGS__) - __printf_range_buf_curr__);	\
		} else {	\
			char __buf__[4096];	\
			__pal_printf_rv__ = ci_snprintf(__buf__, 4096, __VA_ARGS__);	\
			pal_sock_send_str(NULL, __buf__);		\
		}	\
		\
		if (!(ci_printf_info->flag & CI_PRF_CLI))	\
			__pal_printf_rv__ = printf(__VA_ARGS__);		\
		\
		__pal_printf_rv__;	\
	})
#endif

#define ci_alignas(n)							__attribute__ ((aligned(n))) //alignas(n)
#define ci_typeof(x)							__typeof(x)

#define ci_type_compatible(a, b)				__builtin_types_compatible_p(a, b)
#define ci_type_is_str(a)						ci_type_compatible(ci_typeof(a), char[])
#define ci_type_is_const_str(a)					(ci_type_is_str(a) && ci_const_expr(a))
#define ci_type_is_non_const_str(a)				(ci_type_is_str(a) && !ci_const_expr(a))
#define ci_type_is_str_ptr(a)					(ci_type_compatible(ci_typeof(a), char *) || ci_type_compatible(ci_typeof(a), const char *))

#define ci_likely(x)       						__builtin_expect((x), 1)
#define ci_unlikely(x)     						__builtin_expect((x), 0)
#define ci_unused								__attribute__((unused))


#define ci_process_current()					((u32)getpid())
#define ci_thread_current()						pthread_self()


int ci_thread_set_affinity(pthread_t thread, int core);



