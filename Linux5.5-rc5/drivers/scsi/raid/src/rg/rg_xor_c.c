/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * rg_xor_c.c  					C XOR/GF-XOR functions
 *                                                          hua.ye@Hua Ye.com
 */


#include "rg.h"

void rg_xor_c(void **buf, int nr, int size)
{
	int i, j;
	u64 *p[RG_MAX_XOR_ELM];

	for (i = 0; i < nr; i++)
		p[i] = (u64 *)buf[i];

	size /= ci_sizeof(u64);

	while (size--) {
		*p[0] = 0;

		for (j = 1; j < nr; j++)
			*p[0] ^= *p[j]++;

		p[0]++;
	}
}

