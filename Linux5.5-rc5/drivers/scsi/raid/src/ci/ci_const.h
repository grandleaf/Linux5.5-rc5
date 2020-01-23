/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_const.h				CI Constants Definition
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#define CI_U8_MAX					((u8)-1)
#define CI_U16_MAX					((u16)-1)
#define CI_U32_MAX					((u32)-1)
#define CI_U64_MAX					((u64)-1ULL)

#define CI_S8_MAX					((s8)(CI_U8_MAX >> 1))
#define CI_S16_MAX					((s8)(CI_U16_MAX >> 1))
#define CI_S32_MAX					((s8)(CI_U32_MAX >> 1))
#define CI_S64_MAX					((s8)(CI_U64_MAX >> 1))

#define CI_INT_MAX					((int)((unsigned int)-1 >> 1))


#define CI_STR_UNDEFINED_VALUE		"n/a"

