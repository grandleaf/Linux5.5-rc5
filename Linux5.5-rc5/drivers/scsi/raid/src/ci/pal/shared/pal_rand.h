/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_rand.h			PRNG: Use the mersenne-twister algorithm
 * 					 	fast and suitable for non-cryptographic code.
 *
 * In a scheduler's benchmark, I saw >97% CPU cycles is eaten by rand() with
 * 72 workers, so let's use a faster alternative (with context).
 *
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "pal_cfg.h"
#include "pal_type.h"

#define pal_rand_ctx_by_id(node_id, worker_id)		\
	(&ci_worker_by_id(node_id, worker_id)->pal_worker->rand_ctx)
#define pal_rand_ctx_by_ctx(ctx)	\
	pal_rand_ctx_by_id((ctx)->worker->node_id, (ctx)->worker->worker_id)
	
#define pal_rand32(ctx)			prng_rand_u32(ctx)		/* fast */
#define pal_rand64(ctx)			prng_rand_u64(ctx)		/* fast */



/*
 * We have an array of 624 32-bit values, and there are
 * 31 unused bits, so we have a seed value of
 * 624*32-31 = 19937 bits.
 */
#define PRNG_SIZE 					624
#define PRNG_PERIOD  				397
#define PRNG_DIFF   				(PRNG_SIZE - PRNG_PERIOD)

typedef struct {
	u32 idx;
	u32 curr;
	u32 mix;
	u32 tab[PRNG_SIZE];
} pal_rand_ctx_t;

int pal_rand_init();
u32 pal_rand32_shr();
u64 pal_rand64_shr();

/* internal use only, please use pal_rand32() or pal_rand64() */
u32 prng_rand_u32(pal_rand_ctx_t *ctx);
u64 prng_rand_u64(pal_rand_ctx_t *ctx);


