/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_lib.h			Lib interface with libraid0.so
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once
#include "pal_cfg.h"
#include "pal_type.h"

typedef struct {
	u8							*start;				/* [start, end) */
	u8							*end;
	u8							*curr;				/* current allocation ptr */
} pal_mem_range_ex_t; 


/* NOTE: SYNC THIS WITH SIDRAT */
typedef struct {
	void 		 (*c_sys_printf)(char *);			/* first field */
	int			 size_check;						/* second field: for a simple checking purpose */
	int			 flag;
#define LIB_FAST_RAID_CFG_INITED			0x0001
#define LIB_FAST_RAID_CFG_UNLOAD			0X0002	
#define	LIB_FAST_RAID_CFG_NO_MEM			0x0004

#define LIB_FAST_RAID_CFG_ERR				(LIB_FAST_RAID_CFG_NO_MEM)
} lib_fast_raid_cfg_t;





extern lib_fast_raid_cfg_t *lib_fast_raid_cfg; 




#ifdef LIB_FAST_RAID
#define PAL_PRINTF_BUF_SIZE				4096

#define __pal_printf(...)		\
	({	\
		extern char __pal_printf_buf[PAL_PRINTF_BUF_SIZE];		\
		\
		int __rv = __ci_snprintf(__pal_printf_buf, ci_sizeof(__pal_printf_buf), __VA_ARGS__);		\
		lib_fast_raid_cfg->c_sys_printf(__pal_printf_buf);	\
		\
		if (!(ci_printf_info->flag & CI_PRF_CONSOLE))	\
			ci_mem_printf(&ci_printf_info->range_mem_buf, "%s", p);	\
		else {	\
			char __buf__[4096];	\
			ci_snprintf(__buf__, 4096, __VA_ARGS__);	\
			pal_sock_send_str(NULL, __buf__);		\
		}	\
		__rv;		\
	})


#endif /* !LIB_FAST_RAID */



