/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_info.h					A Container
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_list.h"
#include "ci_printf.h"
#include "ci_node.h"
#include "ci_mod.h"
#include "ci_sched.h"
#include "ci_paver.h"

#define ci_mod_info				(ci_info.mod_info)
#define ci_node_info 			(ci_info.node_info)
#define ci_paver_info			(ci_info.paver_info)
#define ci_printf_info			(ci_info.printf_info)
#define ci_sched_info			(ci_info.sched_info)

typedef struct {
	char						*build_nr;
	ci_mod_info_t				*mod_info;
	ci_node_info_t				*node_info;
	ci_paver_info_t				*paver_info;
	ci_printf_info_t			*printf_info;
	ci_sched_info_t				*sched_info;
} ci_info_t;

extern ci_info_t ci_info;


int ci_info_pre_init();

