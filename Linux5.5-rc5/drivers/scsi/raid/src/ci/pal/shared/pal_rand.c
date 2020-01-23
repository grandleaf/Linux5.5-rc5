/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_rand.c			PRNG: Use the mersenne-twister algorithm
 * 					 	fast and suitable for non-cryptographic code.
 *
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

static const u32 prng_matrix[2] = {0, 0x9908b0df};

#define prng_m32(x) 				(0x80000000 & (x)) 	/* 32nd Most Significant Bit */
#define prng_l31(x) 				(0x7FFFFFFF & (x)) 	/* 31 Least Significant Bits */
#define prng_odd(x)					((x) & 1) 			/* Check if number is odd */

#define prng_unroll(ctx, expr) 	\
	do {	\
	  (ctx)->mix = prng_m32((ctx)->tab[(ctx)->curr]) | prng_l31((ctx)->tab[(ctx)->curr + 1]); 	\
	  (ctx)->tab[(ctx)->curr] = (ctx)->tab[expr] ^ ((ctx)->mix >> 1) ^ prng_matrix[prng_odd((ctx)->mix)]; \
	  (ctx)->curr++;	\
	} while (0)


static pal_rand_ctx_t rand_ctx_shr;
static ci_slk_t prng_lock;	


static inline void prng_reload(pal_rand_ctx_t *ctx)
{
	ctx->curr = 0;

	while (ctx->curr < PRNG_DIFF - 1) {
		prng_unroll(ctx, ctx->curr + PRNG_PERIOD);
		prng_unroll(ctx, ctx->curr + PRNG_PERIOD);
	}
	prng_unroll(ctx, (ctx->curr + PRNG_PERIOD) % PRNG_SIZE);		/* ctx->curr == 226 */

  	/* ctx->curr = [227 ... 622] */
	while (ctx->curr < PRNG_SIZE - 1) {
		/*
		 * 623-227 = 396 = 2*2*3*3*11, so we can unroll this loop in any number
		 * that evenly divides 396 (2, 4, 6, etc). Here we'll unroll 11 times.
		 */
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
		prng_unroll(ctx, ctx->curr - PRNG_DIFF);
	}

	/* ctx->curr = 623 */
	ctx->mix = prng_m32(ctx->tab[PRNG_SIZE - 1]) | prng_l31(ctx->tab[0]); 
	ctx->tab[PRNG_SIZE - 1] = ctx->tab[PRNG_PERIOD - 1] ^ (ctx->mix >> 1) ^ prng_matrix[prng_odd(ctx->mix)]; 
}

void prng_seed(pal_rand_ctx_t *ctx, u32 value)
{
	ci_obj_zero(ctx);
	ctx->tab[0] = value;

	ci_loop(i, 1, PRNG_SIZE)
		ctx->tab[i] = 0x6c078965 * (ctx->tab[i - 1] ^ ctx->tab[i - 1] >> 30) + i;
}

u32 prng_rand_u32(pal_rand_ctx_t *ctx)
{
	u32 val;
	
	if (!ctx->idx)
		prng_reload(ctx);

	val = ctx->tab[ctx->idx];

 	/* Tempering */
	val ^= val >> 11;
	val ^= (val << 7) & 0x9d2c5680;
	val ^= (val << 15) & 0xefc60000;
	val ^= val >> 18;

	if ( ++ctx->idx == PRNG_SIZE )
		ctx->idx = 0;

	return val;
}

int prng_rand_s32(pal_rand_ctx_t *ctx)
{
	return CI_S32_MAX & prng_rand_u32(ctx);
}

u64 prng_rand_u64(pal_rand_ctx_t *ctx)
{
	return ((u64)prng_rand_u32(ctx) << 32) | prng_rand_u32(ctx);
}

int pal_rand_init()
{
#ifdef CI_RAND_SEED
	u32 seed = CI_RAND_SEED;
#else
	u32 seed = (u32)time(NULL) + ci_process_current();
#endif
	pal_imp_printf("pal_rand_init, random seed: %#X\n", seed);


	/*
	 *	shared random function without context, slower
	 */
	ci_slk_init(&prng_lock);
	prng_seed(&rand_ctx_shr, seed++);


	/*
	 *	per worker random function with context, faster
	 */
	ci_node_worker_each(node, worker, {
		pal_rand_ctx_t *ctx = pal_rand_ctx_by_id(node->node_id, worker->worker_id);
		prng_seed(ctx, seed++);
	});

#if 0
	/* seed = 1, 1791095845, 4282876139, 3093770124, 4005303368, 491263, 550290313 */
	pal_rand_ctx_t *ctx = pal_rand_ctx_by_id(0, 0);
	ci_loop(10) {
		u32 val = prng_rand_u32(ctx);
		ci_printf("%u\n", val);
	}
	ci_printfln("------------------------------");
	ctx = pal_rand_ctx_by_id(0, 1);
	ci_loop(10) {
		u32 val = prng_rand_u32(ctx);
		ci_printf("%u\n", val);
	}
#endif	

	return 0;
}

u32 pal_rand32_shr()
{
	u32 val;
	
	ci_slk_protected(&prng_lock, {
		val = prng_rand_u32(&rand_ctx_shr);
	});

	return val;
}

u64 pal_rand64_shr()
{
	u64 val;
	
	ci_slk_protected(&prng_lock, {
		val = prng_rand_u64(&rand_ctx_shr);
	});

	return val;
}

