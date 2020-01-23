/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_bmp.c				Bitmap operations
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

#if 0
int __ci_bmp_dump(void *__bmp, int bits, int fmt)
{
	ci_bmp_t *bmp;
	int rv, cnt, skip, max_bmp, nr_bytes, rest, pr, byte, first = 1;

	rv = cnt = pr = 0;

	if (fmt & CI_BMP_DUMP_SHOW_EACH_ONLY)
		goto __show_each_only;
	
	max_bmp = ci_bits_to_bmp(bits) - 1;
	nr_bytes = ci_bits_to_byte(bits);
	rest = nr_bytes % 4;
	skip = (max_bmp + 1) * ci_sizeof(ci_bmp_t) - nr_bytes;
	bmp = (ci_bmp_t *)__bmp + max_bmp;

	ci_loop(i, max_bmp, -1, -1) {
		ci_bmp_t t = *bmp--;

		ci_loop(j, ci_sizeof(ci_bmp_t)) {
			if ((i == max_bmp) && (skip-- > 0)) 
				continue;

			byte = (t >> ((ci_sizeof(ci_bmp_t) - 1 - j) * 8)) & 0xFF;
			if (!(fmt & CI_BMP_DUMP_BIN))
				rv += ci_printf("%02X", byte);
			else {
				first ? first = 0 : (pr || (rv += ci_printf("_")));
				rv += __ci_printf("%08b", byte);	/* skip the print format check since %b is an extension */
			}

			pr = 0, cnt++;
			if (nr_bytes > 4) {
				if ((rest <= 0) && cnt && !(cnt % 4) &&  (i || (j < ci_sizeof(ci_bmp_t) - 1)))
					pr = 1;
				else if (cnt == rest) 
					pr = 1, rest = -1, cnt = 0;

				pr && (rv += ci_printf("."));
			}
		}
	}

__show_each_only:
	if (fmt & (CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_ONLY)) {
		first = 1;
		bmp = (ci_bmp_t *)__bmp;
		
		(fmt & CI_BMP_DUMP_SHOW_EACH) && (rv += ci_printf("  { "));
		
		ci_bmp_each_set(bmp, bits, i) {
			!first && (rv += ci_printf(fmt & CI_BMP_DUMP_SHOW_EACH_COMMA ? ", " : " "));
			rv += ci_printf("%d", i);
			first = 0;
		}
		
		(fmt & CI_BMP_DUMP_SHOW_EACH) && (rv += ci_printf(" }"));
	}

	return rv;
}
#endif

ci_bmp_t *ci_bmp_combo_init(void *__bmp, int bits, int sets)
{
	ci_bmp_t *bmp = (ci_bmp_t *)__bmp;

	ci_bmp_zero(bmp, bits);
	ci_loop(i, sets)
		ci_bmp_set_bit(bmp, bits, i);

	return bmp;
}

/*	0: No next combo available; > 0: Has next combo */
int ci_bmp_combo_next(void *__bmp, int bits)
{
	int last_set, last_clear, hole_set;
	ci_bmp_t *bmp = (ci_bmp_t *)__bmp;

	last_set = ci_bmp_last_set(bmp, bits);
	if ((last_set >= 0) && (last_set < bits - 1)) {
		ci_bmp_clear_bit(bmp, bits, last_set);
		ci_bmp_set_bit(bmp, bits, last_set + 1);
		return 1;
	}

	if ((last_clear = ci_bmp_last_clear(bmp, bits)) < 0) 
		return 0;
	if ((hole_set = ci_bmp_prev_set(bmp, bits, last_clear - 1)) < 0)
		return 0;
	
	ci_bmp_clear_bit(bmp, bits, hole_set);
	ci_bmp_set_bit(bmp, bits, hole_set + 1);

	ci_bmp_clear_range(bmp, bits, last_clear + 1, bits);
	ci_bmp_set_range(bmp, bits, hole_set + 2, hole_set + bits - last_clear + 1);

	return 2;
}

int ci_bmp_sn_dump(char *buf, int len, void *__bmp, int bits, int fmt)
{
	ci_bmp_t *bmp;
	char *e, *p = buf;
	int cnt, skip, max_bmp, nr_bytes, rest, pr, byte, first = 1;

	cnt = pr = 0;
	e = buf + (p ? len : CI_S32_MAX);

	if (fmt & CI_BMP_DUMP_SHOW_EACH_ONLY)
		goto __show_each_only;
	
	max_bmp = ci_bits_to_bmp(bits) - 1;
	nr_bytes = ci_bits_to_byte(bits);
	rest = nr_bytes % 4;
	skip = (max_bmp + 1) * ci_sizeof(ci_bmp_t) - nr_bytes;
	bmp = (ci_bmp_t *)__bmp + max_bmp;

	ci_loop(i, max_bmp, -1, -1) {
		ci_bmp_t t = *bmp--;

		ci_loop(j, ci_sizeof(ci_bmp_t)) {
			if ((i == max_bmp) && (skip-- > 0)) 
				continue;

			byte = (t >> ((ci_sizeof(ci_bmp_t) - 1 - j) * 8)) & 0xFF;
			if (!(fmt & CI_BMP_DUMP_BIN))
				p += ci_buf_snprintf(p, e - p, "%02X", byte);
			else {
				first ? first = 0 : (pr || (p += ci_buf_snprintf(p, e - p, "_")));
				p += __ci_buf_snprintf(p, e - p, "%08b", byte);	/* skip the print format check since %b is an extension */
			}

			pr = 0, cnt++;
			if (nr_bytes > 4) {
				if ((rest <= 0) && cnt && !(cnt % 4) &&  (i || (j < ci_sizeof(ci_bmp_t) - 1)))
					pr = 1;
				else if (cnt == rest) 
					pr = 1, rest = -1, cnt = 0;

				pr && (p += ci_buf_snprintf(p, e - p, "."));
			}
		}
	}

__show_each_only:
	if (fmt & (CI_BMP_DUMP_SHOW_EACH | CI_BMP_DUMP_SHOW_EACH_ONLY)) {
		first = 1;
		bmp = (ci_bmp_t *)__bmp;
		
		(fmt & CI_BMP_DUMP_SHOW_EACH) && (p += ci_buf_snprintf(p, e - p, "  { "));
		
		ci_bmp_each_set(bmp, bits, i) {
			!first && (p += ci_buf_snprintf(p, e - p, fmt & CI_BMP_DUMP_SHOW_EACH_COMMA ? ", " : " "));
			p += ci_buf_snprintf(p, e - p, "%d", i);
			first = 0;
		}
		
		(fmt & CI_BMP_DUMP_SHOW_EACH) && (p += ci_buf_snprintf(p, e - p, " }"));
	}

	return (int)(p - buf);
}

