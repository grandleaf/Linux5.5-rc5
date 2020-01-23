/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci.h					Common Infrastructure
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#include "ci_balloc.h"
#include "ci_bitops.h"
#include "ci_bmp.h"
#include "ci_const.h"
#include "ci_dbg.h"
#include "ci_sched_disp.h"
#include "ci_errno.h"
#include "ci_halloc.h"
#include "ci_info.h"
#include "ci_jcmd.h"
#include "ci_json.h"
#include "ci_list.h"
#include "ci_macro.h"
#include "ci_macro_inc.h"
#include "ci_mod.h"
#include "ci_mod_jcmd.h"
#include "ci_node.h"
#include "ci_paver.h"
#include "ci_printf.h"
#include "ci_printf_def.h"
#include "ci_sched.h"
#include "ci_sta.h"
#include "ci_str.h"
#include "ci_timer.h"
#include "ci_trace.h"
#include "ci_type.h"
#include "ci_util.h"


int ci_init();
int ci_init_done();
int ci_finz();
int ci_finz_done();


