/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_trace.h				Trace Utilities
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

/*
 *	XXX, Temporary Configurations
 */
#define CI_TRACE_FRAME_META_SIZE		16				/* utility use this */
#define CI_TRACE_CHUNK_SIZE				ci_kib(128)		/* configurable */
#define CI_TRACE_BUF_SIZE				ci_mib(32)		/* configurable */
#define ci_trace_const_str_cast			u32				/* saving space, change to u64 in case of ... */



#define CI_NR_TRACE_CHUNK				(CI_TRACE_BUF_SIZE / CI_TRACE_CHUNK_SIZE)				

enum {
	CI_TRACE_TYPE_CONST_STR,		/* 0 */
	CI_TRACE_TYPE_U8,				/* 1 */
	CI_TRACE_TYPE_U16,				/* 2 */
	CI_TRACE_TYPE_U32,				/* 3 */
	CI_TRACE_TYPE_U64,				/* 4 */
	CI_TRACE_NR_FIXED_TYPE,			/* 5 */
	CI_TRACE_MAX_NON_CONST_STR_LEN = 0xFF - CI_TRACE_NR_FIXED_TYPE
};

typedef struct {
	u64				ts_walk;		/* walk timestamp */
	u64				ci_rsvd;
} ci_trace_chunk_meta_t; 

ci_type_size_check(ci_trace_chunk_meta_t, CI_TRACE_FRAME_META_SIZE);


#define __ci_trace_store_argument(val)		\
	do {	\
		if (ci_type_is_non_const_str(val)) {		\
			int __str_len__ = ci_strnlen((char *)(uintptr_t)(val), CI_TRACE_MAX_NON_CONST_STR_LEN);	\
			ci_memcpy(__ci_trace_buf_cur__, (char *)(uintptr_t)(val), __str_len__);	\
			__ci_trace_buf_cur__ += __str_len__;	\
		} else if(ci_type_is_const_str(val)) {		\
			/* '# CON' in order to work around a GCC optimization issue which cause symbol missing */		\
			*(uintptr_t *)__ci_trace_buf_cur__ = (ci_trace_const_str_cast)(uintptr_t)(#val "CON");	\
			__ci_trace_buf_cur__ += ci_sizeof(ci_trace_const_str_cast);	\
		} else {	\
			if (ci_sizeof(val) == 1) 	\
				*(u8 *)__ci_trace_buf_cur__ = (u8)(uintptr_t)(val);	\
			else if (ci_sizeof(val) == 2) 	\
				*(u16 *)__ci_trace_buf_cur__ = (u16)(uintptr_t)(val);	\
			else if (ci_sizeof(val) == 4) 	\
				*(u32 *)__ci_trace_buf_cur__ = (u32)(uintptr_t)(val);	\
			else if (ci_sizeof(val) == 8) 	\
				*(u64 *)__ci_trace_buf_cur__ = (u64)(uintptr_t)(val);	\
			else	\
				ci_panic();	\
			__ci_trace_buf_cur__ += ci_sizeof(val);	\
		}	\
	} while (0);
	
#define __ci_trace_arg_type(val)		\
	({	\
		int __arg_type__;	/* also server as size of non const string */	\
		\
		if (ci_type_is_non_const_str(val))		\
			__arg_type__ = ci_strnlen((char *)(uintptr_t)(val), CI_TRACE_MAX_NON_CONST_STR_LEN) + CI_TRACE_NR_FIXED_TYPE;		\
		else if (ci_type_is_const_str(val))		\
			__arg_type__ = CI_TRACE_TYPE_CONST_STR;	\
		else if (ci_sizeof(val) == ci_sizeof(u8))	\
			__arg_type__ = CI_TRACE_TYPE_U8;	\
		else if (ci_sizeof(val) == ci_sizeof(u16))	\
			__arg_type__ = CI_TRACE_TYPE_U16; \
		else if (ci_sizeof(val) == ci_sizeof(u32))	\
			__arg_type__ = CI_TRACE_TYPE_U32;	 \
		else if (ci_sizeof(val) == ci_sizeof(u64))	\
			__arg_type__ = CI_TRACE_TYPE_U64;	 \
		else {	\
			__arg_type__ = 0xFF;	\
			ci_panic("__ci_trace_arg_type(), unknown type, val=\"%s\"", #val);	\
		}	\
		\
		__arg_type__;	\
	})

#define __ci_trace_arg_size(val)		\
	({	\
		int __arg_size__;	/* also server as size of non const string */	\
		\
		if (ci_type_is_non_const_str(val))		\
			__arg_size__ = ci_strnlen((char *)(uintptr_t)(val), CI_TRACE_MAX_NON_CONST_STR_LEN);		\
		else if (ci_type_is_const_str(val))		\
			__arg_size__ = ci_sizeof(ci_trace_const_str_cast);	\
		else	\
			__arg_size__ = ci_sizeof(val);	\
		\
		__arg_size__;	\
	})		

#define __ci_trace_store_size(val)			*__ci_trace_buf_cur__++ = __ci_trace_arg_type(val);	
#define __ci_trace_add_size(val)			__total_args_size__ += __ci_trace_arg_size(val);
#define __ci_trace_frame_size(...)			(CI_TRACE_META_SIZE + __ci_trace_args_frame_size(__VA_ARGS__))

#define __ci_trace_total_args_size(...)	\
	({	\
		int __total_args_size__ = 0;		\
		ci_m_each(__ci_trace_add_size, __VA_ARGS__);	\
		__total_args_size__;		\
	})

#define __ci_trace_args_frame_size(...)	\
	(__ci_trace_total_args_size(__VA_ARGS__) + ci_m_argc(__VA_ARGS__) + 1)


#define __ci_trace(buf_s, buf_e, u16_meta, ...)		\
	({	\
		int __trace_frame_size__ = __ci_trace_frame_size(__VA_ARGS__);	\
		u8 *__ci_trace_buf_cur__ = (u8 *)(buf_s);		\
		\
		if (ci_unlikely(__ci_trace_buf_cur__ + __trace_frame_size__ + 1 > buf_e))	/* store \0 */	\
			__trace_frame_size__ = 0;	\
		else {	\
			*__ci_trace_buf_cur__++ = ci_m_argc(__VA_ARGS__);	\
			ci_m_each(__ci_trace_store_size, __VA_ARGS__);	\
			ci_m_each(__ci_trace_store_argument, __VA_ARGS__); \
			__ci_trace_meta(__ci_trace_buf_cur__, u16_meta);	\
			*(__ci_trace_buf_cur__ + CI_TRACE_META_SIZE) = 0;	/* end */ \
		}	\
		\
		__trace_frame_size__;	\
	})

/* timestamp, file, line, u16_meta */
#define CI_TRACE_META_SIZE					(ci_sizeof(u64) + ci_sizeof(ci_trace_const_str_cast) + ci_sizeof(u32))		
#define __ci_trace_meta(ptr, u16_meta)		\
	({	\
		*(u64 *)(ptr) = pal_timestamp();		\
		*(ci_trace_const_str_cast *)((ptr) + ci_sizeof(u64)) = (ci_trace_const_str_cast)(uintptr_t)__FILE__;	\
		*(u32 *)((ptr) + ci_sizeof(u64) + ci_sizeof(ci_trace_const_str_cast)) = ((u16)(u16_meta) << 16) | (u16)__LINE__;	\
	})
#define __ci_trace_format_check(...)	\
	({	\
		ci_pragma_push()	\
		ci_pragma_error(CI_WARN_FORMAT)	\
		__ci_printf_check(__VA_ARGS__);		\
		ci_pragma_pop()		\
		0;	\
	})
#define ci_trace_ex(mgr, tc, meta, ...)		\
	({	\
		int __bytes_write__;		\
		\
		__ci_trace_format_check(__VA_ARGS__);	\
		do {	\
			__bytes_write__ = __ci_trace((tc)->curr, (tc)->end, meta, __VA_ARGS__);	\
			if (ci_unlikely(!__bytes_write__)) 	\
				(tc) = ci_trace_mgr_get_chunk(mgr, (tc));		\
		} while (ci_unlikely(!__bytes_write__));	\
		\
		(tc)->curr += __bytes_write__;	\
	})

typedef struct __ci_trace_chunk_t ci_trace_chunk_t;

typedef struct {
	ci_trace_chunk_t		*first;
	ci_trace_chunk_t		*curr;	
	u64						 pass;		/* each loop pass + 1 */
	ci_slk_t				 lock;		/* spinlock */
} ci_trace_mgr_t;

/* chunk control block */		
struct __ci_trace_chunk_t {
	int						 flag;
#define CI_TRACE_CHUNK_BUSY			0x0001	

	ci_trace_chunk_meta_t	*meta;
	u8 						*curr;
	u8 						*end;		/* not included */
	u64						 pass;
	ci_trace_mgr_t			*mgr;
	ci_list_t				 link;
};


/*
 *	XXX, temporary
 */
extern ci_trace_mgr_t		 trace_mgr;
//#define ci_trace(...)		 ci_trace_ex(&trace_mgr, trace_mgr.curr, 0xABCD, __VA_ARGS__)		



int ci_trace_mgr_init(ci_trace_mgr_t *mgr);
ci_trace_chunk_t *ci_trace_mgr_get_chunk(ci_trace_mgr_t *mgr, ci_trace_chunk_t *old);



