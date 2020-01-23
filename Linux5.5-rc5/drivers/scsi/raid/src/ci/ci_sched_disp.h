/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_sched_disp.h					Dispatch tasks to schedulers
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#include "ci_type.h"
#include "ci_list.h"
#include "ci_bmp.h"

/* input old_lvl, nr_busy, get new_lvl */
#define ci_sched_busy_inc_get_lvl(old_lvl, nr_busy)	\
	({	\
		int __high__ = 1 << ((old_lvl) + 1);	\
		__high__ += __high__ >> 1;		\
		ci_unlikely((nr_busy) >= __high__) ? (old_lvl) + 1 : (old_lvl);	\
	})
#define ci_sched_busy_dec_get_lvl(old_lvl, nr_busy)	\
	({	\
		int __low__ = 1 << ((old_lvl) + 1);	\
		__low__ -= __low__ >> 1;		\
		ci_unlikely((nr_busy) <= __low__) ? old_lvl ? (old_lvl) - 1 : (old_lvl) : (old_lvl);	\
	})





