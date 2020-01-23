/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_check.c								Checking definitions 
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

ci_static_assert(PAL_CPU_TOTAL == PAL_NUMA_NR * PAL_CPU_PER_NUMA);	
ci_static_assert(PAL_WORKER_STACK_SIZE % PAL_CPU_CACHE_LINE_SIZE == 0);	



