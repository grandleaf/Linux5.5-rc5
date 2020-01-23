/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_mod_cfg.h				configure module's arguments
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_type.h"

typedef struct {
	const char			*name;
	int					 order_start;
	int					 order_stop;
} pal_mod_cfg_t;

pal_mod_cfg_t *pal_mod_cfg(const char *name);
