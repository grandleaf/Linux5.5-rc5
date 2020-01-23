/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_halloc.h					Heap Allocator (without free functions)
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_list.h"

typedef struct
{
	ci_mem_anchor;
	const char					*name;
	ci_mem_range_t				 range;
	ci_list_t					 link;		/* tracking purpose */
} ci_halloc_rec_t;

typedef struct {
	int							 flag;
#define CI_HALLOC_MT				0x0001	/* multi-thread support, thread safe, default on */	
	
	const char					*name;
	ci_node_t					*node;
	ci_mem_range_ex_t			 range;
	ci_slk_t					 lock;
	ci_list_t					 head;		/* chain all the ci_halloc_rec_t */
} ci_halloc_t;

#define ci_halloc_check(ha)		\
	do {	\
		ci_mem_anchor_check(ha);	\
	} while (0)

int ci_halloc_init(ci_halloc_t *ha, const char *name, u8 *start, u8 *end);
void *ci_halloc(ci_halloc_t *ha, int size, int align, const char *name);
int ci_halloc_dump(ci_halloc_t *ha);



