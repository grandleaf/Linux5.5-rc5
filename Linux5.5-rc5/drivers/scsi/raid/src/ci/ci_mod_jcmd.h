/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_mod_jcmd.h				CI Module for CLI
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"


void ci_jcmd_get_mod_ary(ci_mod_t ***ary, int *count);	/* get pointers[] to all modules (sorted) */
void ci_jcmd_put_mod_ary(ci_mod_t ***ary);				/* free */
void ci_jcmd_get_mod_jcmd_ary(ci_mod_t *mod, ci_jcmd_t ***ary, int *count);	/* get pointers[] to all jcmds for the module (sorted) */
void ci_jcmd_put_mod_jcmd_ary(ci_jcmd_t ***ary);
void ci_jcmd_get_all_mod_jcmd_ary(ci_jcmd_t ***ary, int *count);	/* get pointers[] to all jcmds for all the module (sorted) */
void ci_jcmd_put_all_mod_jcmd_ary(ci_jcmd_t ***ary);


