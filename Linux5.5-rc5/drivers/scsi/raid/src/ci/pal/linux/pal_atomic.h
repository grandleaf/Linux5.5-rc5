/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_atomic.h			PAL atomic operations
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal_cfg.h"


#define ci_atomic_inc(int_addr)					do { __atomic_add_fetch(int_addr, 1, __ATOMIC_ACQ_REL); } while (0)
#define ci_atomic_dec(int_addr)					do { __atomic_sub_fetch(int_addr, 1, __ATOMIC_ACQ_REL); } while (0)

#define ci_atomic_inc_fetch(int_addr)			__atomic_add_fetch(int_addr, 1, __ATOMIC_ACQ_REL)
#define ci_atomic_dec_fetch(int_addr)			__atomic_sub_fetch(int_addr, 1, __ATOMIC_ACQ_REL)
	
#define ci_atomic_fetch_inc(int_addr)			__atomic_fetch_add(int_addr, 1, __ATOMIC_ACQ_REL)
#define ci_atomic_fetch_dec(int_addr)			__atomic_fetch_sub(int_addr, 1, __ATOMIC_ACQ_REL)

#define ci_atomic_add_fetch(int_addr, val)		__atomic_add_fetch(int_addr, val, __ATOMIC_ACQ_REL)
#define ci_atomic_sub_fetch(int_addr, val)		__atomic_sub_fetch(int_addr, val, __ATOMIC_ACQ_REL)

#define ci_atomic_fetch_add(int_addr, val)		__atomic_fetch_add(int_addr, val, __ATOMIC_ACQ_REL)
#define ci_atomic_fetch_sub(int_addr, val)		__atomic_fetch_sub(int_addr, val, __ATOMIC_ACQ_REL)

#define ci_atomic_fetch_or(int_addr, val)		__sync_fetch_and_or(int_addr, val) 

