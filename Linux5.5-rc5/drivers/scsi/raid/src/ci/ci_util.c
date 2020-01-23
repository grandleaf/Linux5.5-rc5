/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_util.c					Utilities
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

u64 ci_factorial(int n)		/* n! */
{
	u64 product = 1;
	ci_loop_i(i, 1, n)
		product *= i;

	return product;
}

u64 ci_permutation(int n, int k)		/* P(n, k) */
{
	u64 product = 1;

	ci_loop_i(i, n - k + 1, n)
		product *= i;

	return product;
}

u64 ci_combination(int n, int k)		/* C(n, k) */
{
	if (n - k < k)
		k = n - k;
	return ci_permutation(n, k) / ci_factorial(k);
}

void ci_memdump(void *addr, int len, char *name) 
{
	int i;
    u8 buff[17], *pc = addr;

	ci_printf("{ addr:%p, len:%d(%#X), name:\"%s\" }\n", addr, len, len, ci_str_na_if_null(name));
    if (len <= 0) {
        ci_printf("  ZERO/NEGATIVE LENGTH: %d\n", len);
        return;
    }

//	ci_print_hlineln(75);
	ci_printf("      ");
	ci_loop(j, 16)
		ci_printf(" %02X", j);
	ci_printf("\n");

	ci_print_hlineln(74);

	for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                ci_printf ("    %s\n", buff);

            ci_printf ("%05X ", i);
        }

        ci_printf (" %02X", pc[i]);
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        ci_printf ("   ");
        i++;
    }

    ci_printf ("    %s\n", buff);
}

const char *ci_int_to_name(ci_int_to_name_t *i2n, int code)
{
	for (; i2n->name; i2n++)
		if (i2n->code == code)
			return i2n->name;
	return CI_STR_UNDEFINED_VALUE;
}

int ci_gcd(int a, int b)		 /* greatest common divisor */
{
    int c;

    c = a % b;
    while (c != 0) {
        a = b;
        b = c;
        c = a % b;
    }

    return b;
}

static int ci_quick_sort_partition(void **a, int l, int r, int (*compare)(void *, void *))
{
    int i, j;
    i = l; j = r;
    void *t;

    for (;;)
    {
        while ((i <= r) && (compare(a[i], a[l]) <= 0))
            i++;
        while ((j > l) && (compare(a[j], a[l]) > 0))
            j--;
        if (i >= j)
            break;
        t = a[i]; a[i] = a[j]; a[j] = t;
    }

    t = a[l]; a[l] = a[j]; a[j] = t;
    return j;
}

void ci_quick_sort(void *__a, int l, int r, int (*compare)(void *, void *))
{
    int j;
	void **a = (void **)__a;

    if (l < r)
    {
        /* divide and conquer */
        j = ci_quick_sort_partition(a, l, r, compare);
        ci_quick_sort(a, l, j - 1, compare);
        ci_quick_sort(a, j + 1, r, compare);
    }
}

int ci_get_argc(const char **argv)
{
	int count;
	const char **p;

	if (!argv)
		return 0;
	
	for (p = argv, count = 0; *p; p++, count++)
		;

	return count;	
}

