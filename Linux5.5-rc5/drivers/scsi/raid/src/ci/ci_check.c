/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_check.c						Checking ...
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

ci_type_size_check(s64, 8);
ci_type_size_check(u64, 8);
ci_static_assert((1 << CI_CPU_WORD_SHIFT) == CI_CPU_WORD_WIDTH);
ci_static_assert(ci_m_has_args() == 0);
ci_static_assert(ci_m_argc() == 0);


