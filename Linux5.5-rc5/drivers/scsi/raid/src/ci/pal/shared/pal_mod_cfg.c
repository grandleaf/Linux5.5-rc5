/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_mod_cfg.c				configure module's arguments
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

static pal_mod_cfg_t mod_cfg[] = {
	{ 
		.name 			= "cli",	
		.order_start 	= CI_MOD_ORDER_MIN,		/* first one to start */
		.order_stop		= CI_MOD_ORDER_MAX		/* last one to stop */
	},

	{ 
		.name 			= "testm",	
		.order_start 	= 200,
		.order_stop		= 222
	},
	
	CI_EOT
};

pal_mod_cfg_t *pal_mod_cfg(const char *name)
{
	pal_mod_cfg_t *cfg;

	for (cfg = mod_cfg; cfg->name; cfg++)
		if (ci_strequal(cfg->name, name)) 
			return cfg;

	return NULL;
}

 
