/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_timer.h				Timer Service
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"

#define PAL_USEC_PER_JIFFIE				(1000000 / PAL_JIFFIE_PER_SEC)
#define PAL_MSEC_PER_JIFFIE				(1000 / PAL_JIFFIE_PER_SEC)
#define PAL_TIMER_INTERVAL				(PAL_USEC_PER_JIFFIE / 2)


typedef struct {
	void			   (*callback)(int jiffie_inc, void *data);
	void				*data;

#ifdef NDEBUG
	u8					 link_buf[16];		/* to put ci_list_t */
#else
	u8					 link_buf[56];		
#endif
} pal_timer_intr_handler_t;


extern u64 pal_jiffie;

int pal_timer_init();
int pal_timer_finz();
void pal_timer_register(pal_timer_intr_handler_t *handler);



