/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_type.h				PAL types define
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once


#include "pal_cfg.h"
#include "pal_wrap.h"


typedef uint8_t				u8;
typedef uint16_t			u16;
typedef uint32_t			u32;
typedef unsigned long long	u64;
typedef int8_t				s8;
typedef int16_t				s16;
typedef int32_t				s32;
typedef long long			s64;

/* informative: for little endian and big endian */
typedef uint8_t				le8;
typedef uint16_t			le16;
typedef uint32_t			le32;
typedef uint64_t			le64;
typedef uint8_t				be8;
typedef uint16_t			be16;
typedef uint32_t			be32;
typedef uint64_t			be64;


typedef u64                 ci_cpu_word_t;

#define CI_CPU_WORD_WIDTH	64
#define CI_CPU_WORD_SHIFT	6






