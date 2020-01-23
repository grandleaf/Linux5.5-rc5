/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_printf.c				CI Print Functions
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

static const char *outline_symbol[CI_PR_OUTLINE_NR] = {
	CI_PR_OUTLINE_PREFIX	CI_PR_OUTLINE_STR_NONE			CI_PR_OUTLINE_SUFFIX,
	CI_PR_OUTLINE_PREFIX	CI_PR_OUTLINE_STR_INDIRECT		CI_PR_OUTLINE_SUFFIX,
	CI_PR_OUTLINE_PREFIX	CI_PR_OUTLINE_STR_DIRECT		CI_PR_OUTLINE_SUFFIX,
	CI_PR_OUTLINE_PREFIX	CI_PR_OUTLINE_STR_DIRECT_LAST	CI_PR_OUTLINE_SUFFIX
};

static const char *box_symbol[CI_PR_BOX_TYPE_NR][CI_PR_BOX_ID_NR] = {
	{ CI_PR_BOX_STR_TITLE_TOP_LEFT, CI_PR_BOX_STR_TITLE_TOP_HLINE, 	CI_PR_BOX_STR_TITLE_TOP_VLINE,	CI_PR_BOX_STR_TITLE_TOP_RIGHT },
	{ CI_PR_BOX_STR_TITLE_TXT_LEFT, CI_PR_BOX_STR_TITLE_TXT_HLINE, 	CI_PR_BOX_STR_TITLE_TXT_VLINE, 	CI_PR_BOX_STR_TITLE_TXT_RIGHT },
	{ CI_PR_BOX_STR_TITLE_BTM_LEFT, CI_PR_BOX_STR_TITLE_BTM_HLINE, 	CI_PR_BOX_STR_TITLE_BTM_VLINE, 	CI_PR_BOX_STR_TITLE_BTM_RIGHT },

	{ CI_PR_BOX_STR_BODY_TOP_LEFT, 	CI_PR_BOX_STR_BODY_TOP_HLINE, 	CI_PR_BOX_STR_BODY_TOP_VLINE, 	CI_PR_BOX_STR_BODY_TOP_RIGHT },
	{ CI_PR_BOX_STR_BODY_TXT_LEFT, 	CI_PR_BOX_STR_BODY_TXT_HLINE, 	CI_PR_BOX_STR_BODY_TXT_VLINE, 	CI_PR_BOX_STR_BODY_TXT_RIGHT },
	{ CI_PR_BOX_STR_BODY_BTM_LEFT, 	CI_PR_BOX_STR_BODY_BTM_HLINE, 	CI_PR_BOX_STR_BODY_BTM_VLINE, 	CI_PR_BOX_STR_BODY_BTM_RIGHT },
};



#define __ZEROPAD				0x0001		/* pad with zero */
#define __SIGN					0x0002		/* unsigned/signed long */
#define __PLUS					0x0004		/* show plus */
#define __SPACE					0x0008		/* space if plus */
#define __LEFT					0x0010		/* left justified */
#define __SPECIAL				0x0020		/* 0x */
#define __LARGE					0x0040		/* use 'ABCDEF' instead of 'abcdef' */
#define __COMMA					0x0080		/* e.g. 123,456,789 */


static ci_printf_info_t printf_info = { .flag = CI_PRF_CR, .meta_flag = CI_PRF_META_DFT, .seq_nr = 1 };

int ci_printf_pre_init()
{
	ci_info.printf_info = &printf_info;
	ci_mem_range_ex_init(&ci_printf_info->range_mem_buf, ci_printf_info->mem_buf, ci_printf_info->mem_buf + CI_PR_MEM_BUF_SIZE);
	return 0;
}

int ci_printf_init()
{
	ci_slk_init(&ci_printf_info->line_lock);
	ci_slk_init(&ci_printf_info->block_lock);
	ci_printf_info->flag |= CI_PRF_SLK;

	return 0;
}

static int str_nlen(const char *s, int precision)
{
	int len = ci_strlen(s);
	return (len > precision) && (precision >= 0) ? precision : len;
}


/*
 * Convert a string to an unsigned long
 * cp:		The start of the string
 * endp:	A pointer to the end of the parsed string will be placed here
 * base:	The number base to use
 */
static unsigned long simple_str_to_ul(const char *cp, char **endp, int base)
{
	int value;
	unsigned long result = 0;;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((ci_char_to_upper(*cp) == 'X') && ci_char_is_xdigit(cp[1])) {
				cp++;
				base = 16;
			} else if (ci_char_to_upper(*cp) == 'B') {
				cp++;
				base = 2;
			}
		}
	} else if (base == 16) {
		if (cp[0] == '0' && ci_char_to_upper(cp[1]) == 'X')
			cp += 2;
	}

	while (ci_char_is_xdigit(*cp) && (value = ci_char_is_digit(*cp) ? *cp - '0' : ci_char_to_upper(*cp) - 'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

/*
 * simple_str_to_l - convert a string to a signed long
 * cp:		The start of the string
 * endp:	A pointer to the end of the parsed string will be placed here
 * base:	The number base to use
 */
static long simple_str_to_l(const char *cp, char **endp, int base)
{
	if (*cp == '-')
		return -(long)simple_str_to_ul(cp + 1, endp, base);

	return (long)simple_str_to_ul(cp, endp, base);
}

/*
 * simple_str_to_ull - convert a string to an unsigned long long
 * cp:		The start of the string
 * endp:	A pointer to the end of the parsed string will be placed here
 * base:	The number base to use
 */
static unsigned long long simple_str_to_ull(const char *cp, char **endp, int base)
{
	int value;
	unsigned long long result = 0;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((ci_char_to_upper(*cp) == 'X') && ci_char_is_xdigit(cp[1])) {
				cp++;
				base = 16;
			} else if (ci_char_to_upper(*cp) == 'B') {
				cp++;
				base = 2;
			}
		}
	} else if (base == 16) {
		if (cp[0] == '0' && ci_char_to_upper(cp[1]) == 'X')
			cp += 2;
	}

	while (ci_char_is_xdigit(*cp) && (value = ci_char_is_digit(*cp) ? *cp - '0' : (ci_char_is_lower(*cp) ? ci_char_to_upper(*cp) : *cp) - 'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;
	return result;
}

/* a wrapper */
u64 ci_str_to_u64(char *cp, char **endp, int base)
{
	return simple_str_to_ull(cp, endp, base);
}

/*
 * simple_str_to_ll - convert a string to a signed long long
 * cp:		The start of the string
 * endp:	A pointer to the end of the parsed string will be placed here
 * base:	The number base to use
 */
static long long simple_str_to_ll(const char *cp, char **endp, int base)
{
	if (*cp == '-')
		return -(long)simple_str_to_ull(cp + 1, endp, base);

	return (long long)simple_str_to_ull(cp, endp, base);
}

/*
 * skip_atoi - convert a string to a int
 * It is basically a atoi(), just skip invalid characters
 */
static int skip_atoi(const char **s)
{
	int i = 0;

	while (ci_char_is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

/*
 * number - generate a number string according to base, size, precision, etc...
 */
static char *number(char *buf, char *end, unsigned long long nr, int base, int size, int precision, int type)
{
	int i, j;
	char c, sign, tmp[66];
	const char *digits;
	static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	digits = (type & __LARGE) ? large_digits : small_digits;

	if (type & __LEFT)
		type &= ~__ZEROPAD;
	if (base < 2 || base > 36)
		return NULL;

	c = (type & __ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & __SIGN) {
		if ((signed long long)nr < 0) {
			sign = '-';
			nr = (unsigned long long) - (signed long long)nr;
			size--;
		} else if (type & __PLUS) {
			sign = '+';
			size--;
		} else if (type & __SPACE) {
			sign = ' ';
			size--;
		}
	}

	if (type & __SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}

	i = j = 0;
	if (nr == 0) 
		tmp[i++] = '0';
	else
		while (nr != 0) {
			tmp[i++] = digits[nr % base];
			nr = nr / base;
			if ((type & __COMMA) && (nr != 0) && (++j % 3 == 0))
				tmp[i++] = ',';
		}

	if (i > precision)
		precision = i;
	size -= precision;

	if (!(type & (__ZEROPAD + __LEFT))) {
		while (size-- > 0) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}

	if (sign) {
		if (buf < end)
			*buf = sign;
		++buf;
	}

	if (type & __SPECIAL) {
		if (base == 8) {
			if (buf < end)
				*buf = '0';
			++buf;
		} else if (base == 16) {
			if (buf < end)
				*buf = '0';
			++buf;
			if (buf < end)
				*buf = 'x';
			//				*buf = digits[33];
			++buf;
		}
	}

	if (!(type & __LEFT)) 
		while (size-- > 0) {
			if (buf < end)
				*buf = c;
			++buf;
		}

	while (i < precision--) {
		if (buf < end)
			*buf = '0';
		++buf;
	}

	while (i-- > 0) {
		if (buf < end)
			*buf = tmp[i];
		++buf;
	}

	while (size-- > 0) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}

	return buf;
}

/*
 * __vsnprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * size:	The size of the buffer, including the trailing null space
 * fmt:		The format string to use
 * args:	Arguments for the format string
 *
 * The return value is the number of characters which would
 * be generated for the given input, excluding the trailing
 * '\0', as per ISO C99. If you want to have the exact
 * number of characters written into @buf as return value
 * (not including the trailing '\0'), use ci_vscnprintf. If the
 * return is greater than or equal to @size, the resulting
 * string is truncated.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want ci_snprintf instead.
 */
int __ci_vsnprintf(char *buf, int size, const char *fmt, va_list args)
{
	int len;
	unsigned long long nr;
	int i, base;
	char *str, *end, c;
	const char *s;

	int flags;			/* flags to number() */

	int field_width;	/* width of output field */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */
	int precision;		/* min. # of digits for integers; max
						   number of chars for from string */

	/* Reject out-of-range values early.  Large positive sizes are used for unknown buffer sizes. */
	if (size < 0) {
		/* There can be only one.. */
		/*		static int warn = 1;
				WARN_ON(warn);
				warn = 0;
		 */
		return 0;
	}

	str = buf;
	end = buf + size;
	/* Make sure end is always >= buf */
	if (end < buf) {
		end = (char *)-1;
		size = (int)(end - buf);
	}

	for (; *fmt; ++fmt) {

		if (*fmt != '%') {
			if (str < end)
				*str = *fmt;
			++str;
			continue;
		}

		/* process flags */
		flags = 0;

	repeat:
		++fmt; /* this also skips first '%' */
		switch (*fmt) {
			case '-': flags |= __LEFT;
				goto repeat;
			case '+': flags |= __PLUS;
				goto repeat;
			case ' ': flags |= __SPACE;
				goto repeat;
			case '#': flags |= __SPECIAL;
				goto repeat;
			case '0': flags |= __ZEROPAD;
				goto repeat;
		}

		/* get field width */
		field_width = -1;
		if (ci_char_is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= __LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (ci_char_is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
			*fmt == 'Z' || *fmt == 'z' || *fmt == 't') 
		{
			qualifier = *fmt;
			++fmt;
			if (qualifier == 'l' && *fmt == 'l') {
				qualifier = 'L';
				++fmt;
			}
		}

		/* default base */
		base = 10;

		switch (*fmt) {
			case 'c':
				if (!(flags & __LEFT)) {
					while (--field_width > 0) {
						if (str < end)
							*str = ' ';
						++str;
					}
				}

				c = (char)va_arg(args, int);
				if (str < end)
					*str = c;
				++str;
				while (--field_width > 0) {
					if (str < end)
						*str = ' ';
					++str;
				}
				continue;

			case 's':
				s = va_arg(args, char *);
				if ((unsigned long long)(ptrdiff_t)s == 0)
					s = "<NULL>";

				len = (int)str_nlen(s, precision);
				if (!(flags & __LEFT)) {
					while (len < field_width--) {
						if (str < end)
							*str = ' ';
						++str;
					}
				}

				for (i = 0; i < len; ++i) {
					if (str < end)
						*str = *s;
					++str;
					++s;
				}

				while (len < field_width--) {
					if (str < end)
						*str = ' ';
					++str;
				}
				continue;

			case 'p':
				if (field_width == -1) {
					field_width = 2 * sizeof(void *) + 2;	/* +0x */
					flags |= __ZEROPAD;
				}

				flags |= __SPECIAL;
				str = number(str, end,
							(unsigned long long)(ptrdiff_t)va_arg(args, void *),
							16, field_width, precision, flags);
				continue;

			case 'P':
				if (field_width == -1) {
					field_width = 2 * sizeof(void *) + 2;	/* +0x */
					flags |= __ZEROPAD;
				}

				flags |= __LARGE | __SPECIAL;
				str = number(str, end,
							(unsigned long long)(ptrdiff_t)va_arg(args, void *),
							16, field_width, precision >= 8 ? precision : 8, flags);
				continue;

			case 'n':
				/* FIXME:
				* What does C99 say about the overflow case here? */
				if (qualifier == 'l') {
					long *ip = va_arg(args, long *);
					*ip = (long)(str - buf);
				} else if (qualifier == 'Z' || qualifier == 'z') {
					size_t *ip = va_arg(args, size_t *);
					*ip = (size_t)(str - buf);
				} else {
					int *ip = va_arg(args, int *);
					*ip = (long)(str - buf);
				}
				continue;

			case '%':
				if (str < end)
					*str = '%';
				++str;
				continue;

				/* integer number formats - set up the flags and "break" */
			case 'o':
				base = 8;
				break;

			case 'X':
				flags |= __LARGE;
			case 'x':
				base = 16;
				break;

			case 'b':
				base = 2;
				break;

			case 'i':
				flags |= __COMMA;
			case 'd':
				flags |= __SIGN;
			case 'u':
				break;

			default:
				if (str < end)
					*str = '%';
				++str;
				if (*fmt) {
					if (str < end)
						*str = *fmt;
					++str;
				} else 
					--fmt;
				continue;
		}

		if (qualifier == 'L')
			nr = (unsigned long long)va_arg(args, long long);
		else if (qualifier == 'l') {
			nr = va_arg(args, unsigned long);
			if (flags & __SIGN)
				nr = (unsigned long long)(signed long) nr;
		} else if (qualifier == 'Z' || qualifier == 'z') {
			nr = va_arg(args, size_t);
		} else if (qualifier == 't') {
			nr = (unsigned long long)va_arg(args, ptrdiff_t);
		} else if (qualifier == 'h') {
			nr = (unsigned short) va_arg(args, int);
			if (flags & __SIGN)
				nr = (unsigned long long)(signed short)nr;
		} else {
			nr = va_arg(args, unsigned int);
			if (flags & __SIGN)
				nr = (unsigned long long)(signed int)nr;
		}

		str = number(str, end, nr, base,
					field_width, precision, flags);
	}

	if (size > 0) {
		if (str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}

	/* the trailing null byte doesn't count towards the total */
	return (int)(str - buf);
}


/*
 * __ci_vscnprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * size:	The size of the buffer, including the trailing null space
 * fmt:		The format string to use
 * args:	Arguments for the format string
 *
 * The return value is the number of characters which have been written into
 * the @buf not including the trailing '\0'. If @size is <= 0 the function
 * returns 0.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want ci_scnprintf instead.
 */
int __ci_vscnprintf(char *buf, int size, const char *fmt, va_list args)
{
	int rv;

	rv = ci_vsnprintf(buf, size, fmt, args);
	return rv >= size ? size - 1 : rv;
}

/*
 * __ci_snprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * size:	The size of the buffer, including the trailing null space
 * fmt:		The format string to use
 * ...:		Arguments for the format string
 *
 * The return value is the number of characters which would be
 * generated for the given input, excluding the trailing null,
 * as per ISO C99.  If the return is greater than or equal to
 * size, the resulting string is truncated.
 */
int __ci_snprintf(char *buf, int size, const char *fmt, ...)
{
	int rv;
	va_list args;

	va_start(args, fmt);
	rv = ci_vsnprintf(buf, size, fmt, args);
	va_end(args);
	return rv;
}

/* same as __ci_snprintf(), but return the buf instead */
char *__ci_snprintf_buf(char *buf, int size, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	ci_vsnprintf(buf, size, fmt, args);
	va_end(args);
	return buf;
}


/*
 * __ci_scnprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * size:	The size of the buffer, including the trailing null space
 * fmt:		The format string to use
 * ...:		Arguments for the format string
 *
 * The return value is the number of characters written into @buf not including
 * the trailing '\0'. If @size is <= 0 the function returns 0. If the return is
 * greater than or equal to @size, the resulting string is truncated.
 */

int __ci_scnprintf(char *buf, int size, const char *fmt, ...)
{
	int rv;
	va_list args;

	va_start(args, fmt);
	rv = ci_vsnprintf(buf, size, fmt, args);
	va_end(args);
	return rv >= size ? size - 1 : rv;
}

#if 0		// comment it out since it doesn't have buffer overflow check
/*
 * ci_vsprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * fmt:		The format string to use
 * args:	Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use ci_vsnprintf or ci_vscnprintf in order to avoid
 * buffer overflows.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want ci_sprintf instead.
 */
int ci_vsprintf(char *buf, const char *fmt, va_list args)
{
	return ci_vsnprintf(buf, CI_INT_MAX, fmt, args);
}


/*
 * ci_sprintf - Format a string and place it in a buffer
 * buf:		The buffer to place the result into
 * fmt:		The format string to use
 * ...:		Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use ci_snprintf or ci_scnprintf in order to avoid
 * buffer overflows.
 */
int ci_sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = ci_vsnprintf(buf, CI_INT_MAX, fmt, args);
	va_end(args);
	return i;
}
#endif

/*
 * __ci_vsscanf - Unformat a buffer into a list of arguments
 * buf:		input buffer
 * fmt:		format of buffer
 * args:	arguments
 */
int __ci_vsscanf(const char *buf, const char *fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int nr = 0;
	int qualifier;
	int base;
	int field_width;
	int is_sign;

	while (*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of white space, including none, in the input. */
		if (ci_char_is_space(*fmt)) {
			while (ci_char_is_space(*fmt))
				++fmt;
			while (ci_char_is_space(*str))
				++str;
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;

		/* skip this conversion.
		* advance both strings to next white space
		*/
		if (*fmt == '*') {
			while (!ci_char_is_space(*fmt) && *fmt)
				fmt++;
			while (!ci_char_is_space(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (ci_char_is_digit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
			*fmt == 'Z' || *fmt == 'z') {
			qualifier = *fmt++;
			if (qualifier == *fmt) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}

		base = 10;
		is_sign = 0;

		if (!*fmt || !*str)
			break;

		switch (*fmt++) {
			case 'c': {
				char *s = (char *)va_arg(args, char*);
				if (field_width == -1)
					field_width = 1;
				do {
					*s++ = *str++;
				} while (--field_width > 0 && *str);
				nr++;
			}
			continue;

			case 's': {
				char *s = (char *) va_arg(args, char *);
				if (field_width == -1)
					field_width = CI_INT_MAX;
				/* first, skip leading white space in buffer */
				while (ci_char_is_space(*str))
					str++;

				/* now copy until next white space */
				while (*str && !ci_char_is_space(*str) && field_width--) 
					*s++ = *str++;
				*s = '\0';
				nr++;
			}
			continue;

			case 'n': {
				/* return number of characters read so far */ 
				int *i = (int *)va_arg(args,int*);
				*i = (int)(str - buf);
				}
				continue;

			case 'o':
				base = 8;
				break;

			case 'x':
			case 'X':
				base = 16;
				break;

			case 'i':
				base = 0;
			case 'd':
				is_sign = 1;
			case 'u':
				break;

			case '%':
				/* looking for '%' in str */
				if (*str++ != '%')
					return nr;
				continue;

			default:
				/* invalid format; stop here */
				return nr;
		}

		/* have some sort of integer conversion.  First, skip white space in buffer. */
		while (ci_char_is_space(*str))
			str++;

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
			|| (base == 16 && !ci_char_is_xdigit(digit))
			|| (base == 10 && !ci_char_is_digit(digit))
			|| (base == 8 && (!ci_char_is_digit(digit) || digit > '7'))
			|| (base == 0 && !ci_char_is_digit(digit)))
			break;

		switch (qualifier) {
			case 'H': /* that's 'hh' in format */
				if (is_sign) {
					signed char *s = (signed char *) va_arg(args,signed char *);
					*s = (signed char) simple_str_to_l(str, &next, base);
				} else {
					unsigned char *s = (unsigned char *) va_arg(args, unsigned char *);
					*s = (unsigned char) simple_str_to_ul(str, &next, base);
				}
				break;
			case 'h':
				if (is_sign) {
					short *s = (short *) va_arg(args,short *);
					*s = (short) simple_str_to_l(str, &next, base);
				} else {
					unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
					*s = (unsigned short) simple_str_to_ul(str, &next, base);
				}
				break;
			case 'l':
				if (is_sign) {
					long *l = (long *) va_arg(args,long *);
					*l = simple_str_to_l(str, &next, base);
				} else {
					unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
					*l = simple_str_to_ul(str, &next, base);
				}
				break;
			case 'L':
				if (is_sign) {
					long long *l = (long long*) va_arg(args,long long *);
					*l = simple_str_to_ll(str, &next, base);
				} else {
					unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
					*l = simple_str_to_ull(str, &next, base);
				}
				break;
			case 'Z':
			case 'z': {
				size_t *s = (size_t*) va_arg(args,size_t*);
				*s = (size_t) simple_str_to_ul(str, &next, base);
			}
				break;
			default:
				if (is_sign) {
					int *i = (int *) va_arg(args, int*);
					*i = (int) simple_str_to_l(str, &next, base);
				} else {
					unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
					*i = (unsigned int) simple_str_to_ul(str, &next, base);
				}
				break;
		}
		nr++;

		if (!next)
			break;
		str = next;
	}
	return nr;
}


/*
 * __ci_sscanf - Unformat a buffer into a list of arguments
 * buf:		input buffer
 * fmt:		formatting of buffer
 * ...:		resulting arguments
 */
int __ci_sscanf(const char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args,fmt);
	i = ci_vsscanf(buf, fmt, args);
	va_end(args);
	return i;
}

static int __ci_printf_meta()
{
	const char *prefix;
	int rv = 0, cnt = 0;

	u32_each_set(ci_printf_info->meta_flag, idx) {
		if (cnt++ > 0)
			rv += __pal_printf(" ");

		switch(1 << idx) {
			case CI_PRF_META_SEQ:
				ci_snprintf(ci_printf_info->meta_buf, CI_PR_META_BUF_SIZE, "%06llu", ci_printf_info->seq_nr);
				rv += __pal_printf("%s", ci_printf_info->meta_buf);
				break;
			case CI_PRF_META_LOCAL_TIME:
				pal_local_time_str(ci_printf_info->meta_buf, CI_PR_META_BUF_SIZE);
				rv += __pal_printf("%s", ci_printf_info->meta_buf);
				break;
			case CI_PRF_META_SCHED_CTX:
				rv += __pal_printf(CI_PRN_FMT_NODE_WORKER, CI_PRN_VAL_NODE_WORKER);
				break;
			case CI_PRF_META_UTC_TIME:
				pal_utc_time_str(ci_printf_info->meta_buf, CI_PR_META_BUF_SIZE);
				rv += __pal_printf("%s", ci_printf_info->meta_buf);
				break;
			case CI_PRF_META_FILE:
				ci_snprintf(ci_printf_info->meta_buf, ci_min(CI_PR_META_BUF_SIZE, CI_PR_META_MAX_FILE_LEN + 1), 
							"%" ci_m_to_str(CI_PR_META_MAX_FILE_LEN) "s", 
							ci_str_file_base_name(ci_printf_info->file));
				rv += __pal_printf("%s", ci_printf_info->meta_buf);
				break;
			case CI_PRF_META_FUNC:
				ci_snprintf(ci_printf_info->meta_buf, ci_min(CI_PR_META_BUF_SIZE, CI_PR_META_MAX_FUNC_LEN + 1 + 2), 
							"%" ci_m_to_str(CI_PR_META_MAX_FUNC_LEN) "s()", 
							ci_printf_info->func);
				rv += __pal_printf("%s", ci_printf_info->meta_buf);
				break;
			default:
				break; /* ignore */
		}
	}

//	if (cnt)
//		rv += __pal_printf(": ");

	if (ci_printf_info->prefix_override && !ci_strequal(ci_printf_info->prefix_override, "pal")) 
		prefix = ci_printf_info->prefix_override;
	else {
		ci_sched_ctx_t *ctx = ci_sched_ctx();
		
		if (ctx && ctx->sched_grp && ctx->sched_grp->mod)
			prefix = ctx->sched_grp->mod->name;
		else
			prefix = ci_printf_info->prefix ? ci_printf_info->prefix : "???";
	}

	rv += __pal_printf(CI_PR_PREFIX_FMT, prefix);
	rv += __pal_printf("%c ", ci_printf_info->ntc);

	return rv;
}

static int __ci_printf_indent()
{
	int rv = 0;

	if ((ci_printf_info->flag & CI_PRF_CR) && !(ci_printf_info->flag & CI_PRF_NO_INDENT))
		ci_loop(ci_printf_info->indent)
			rv += __pal_printf("%s", CI_PR_INDENT);

	return rv;
}

void __ci_printf_line_lock()
{
	if (ci_printf_info->flag & CI_PRF_SLK) 
		ci_slk_lock(&ci_printf_info->line_lock);
}

void __ci_printf_line_unlock()
{
	if (ci_printf_info->flag & CI_PRF_SLK) 
		ci_slk_unlock(&ci_printf_info->line_lock);
}

void __ci_printf_block_lock()
{
	if (ci_printf_info->flag & CI_PRF_SLK) 
		ci_slk_lock(&ci_printf_info->block_lock);
}

/* 1 is success */
int __ci_printf_block_try_lock()
{
	if (!(ci_printf_info->flag & CI_PRF_SLK))
		return 1;
	
	return ci_slk_try_lock_timeout(&ci_printf_info->block_lock, CI_PR_BLOCK_LOCK_TIMEOUT);
}

void __ci_printf_block_unlock()
{
	if (ci_printf_info->flag & CI_PRF_SLK) 
		ci_slk_unlock(&ci_printf_info->block_lock);
}


void __ci_printf_set_loc(const char *file, const char *func, int line, const char *prefix, char ntc)
{
	ci_printf_info->file 	= file;
	ci_printf_info->func 	= func;
	ci_printf_info->line 	= line;
	ci_printf_info->prefix	= prefix;
	ci_printf_info->ntc 	= ntc;
}

void __ci_printf_set_prefix_override(const char *prefix_override)
{
	ci_printf_info->prefix_override = prefix_override;
}

void __ci_printf_clear_prefix_override()
{
	ci_printf_info->prefix_override = NULL;
}

void __ci_printf_set_flag(int flag)
{
	ci_printf_info->flag |= flag;
}

void __ci_printf_clear_flag(int flag)
{
	ci_printf_info->flag &= ~flag;
}

void __ci_printf_try_block_lock()
{
	ci_sched_ctx_t *ctx = ci_sched_ctx();

	if (!ctx || (ctx->flag & CI_SCHED_CTXF_PRINTF))		/* no ctx or already get the lock */
		return;

	if (__ci_printf_block_try_lock())
		ctx->flag |= CI_SCHED_CTXF_PRINTF;
//	else printf("#####################################################################\n");
}


/*
 * __ci_raw_printf() 
 * buf:		input buffer
 * fmt:		formatting of buffer
 * ...:		resulting arguments
 */
int __ci_raw_printf(const char *format, ...)
{
	int len, rv = 0;
	char *buf = ci_printf_info->print_buf;

	char c, *p = buf;
	va_list ap;

	*buf = 0;
	va_start(ap, format);
	rv += ci_vsnprintf(p, CI_PR_BUF_SIZE, format, ap);
	va_end(ap);

	do {
		if ((ci_printf_info->flag & CI_PRF_CR) && !(ci_printf_info->flag & CI_PRF_NOMETA)) {
			__ci_printf_meta();
			ci_printf_info->seq_nr++;
		}

		__ci_printf_indent();
		
		c = 0;
		if ((len = ci_str_first_char(p, '\n')) >= 0) 
			ci_printf_info->flag |= CI_PRF_CR;
		else {
			ci_printf_info->flag &= ~CI_PRF_CR;
			len = ci_strlen(p) - 1;
		} 

		len++;

		ci_swap_type(c, p[len], char);
		__pal_printf("%s", p);

		p[len] = c;
		p += len;
	} while (c);

	return rv;
}

ci_big_nr_parse_t *ci_big_nr_parse(ci_big_nr_parse_t *parse)
{
	int u_idx;
	u64 u, uu, ud, r, big_nr, dec, cmp_val;
	
	const char **ua;
	const char *unit_ary[] 		= { "", "K",  "M",  "G",  "T",  "P",  "E"  };	
	const char *unit_ary_it[]	= { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei" };
	const char *unit_ary_verbose[] = { "", "kilo", "mega", "giga", "tera", "peta", "exa" };
	
	ci_flag_range_check(parse->flag, CI_BIG_NR_PARSE_POWER_OF_2 | CI_BIG_NR_PARSE_UNIT_VERBOSE | CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_FIXED);
	ci_assert(ci_nr_elm(unit_ary) == ci_nr_elm(unit_ary_it));
	ci_range_check_i(parse->dec_len, 0, CI_BIG_NR_PARSE_MAX_DEC_LEN);

	if (parse->flag & CI_BIG_NR_PARSE_UNIT_VERBOSE) {
		ci_assert(!(parse->flag & CI_BIG_NR_PARSE_POWER_OF_2));
		u = 1000;
		ua = unit_ary_verbose;
	} else if (parse->flag & CI_BIG_NR_PARSE_POWER_OF_2) {
		u = 1024;
		ua = unit_ary_it;
	} else {
		u = 1000;
		ua = unit_ary;
	}

	ud = 1;
	ci_loop(parse->dec_len)
		ud *= 10;

	uu = 1;
	cmp_val = parse->flag & CI_BIG_NR_PARSE_FIXED ? parse->fixed : parse->big_nr;
	ci_loop(i, ci_nr_elm(unit_ary)) {
		uu *= u, u_idx = i;
		if (cmp_val < uu) 
			break;
	}

	uu /= u;
	r = parse->flag & CI_BIG_NR_PARSE_ROUND ? uu / 2 / ud : 0;
	big_nr = parse->big_nr + r;
	parse->int_part = (int)(big_nr / uu);

	dec = big_nr - parse->int_part * uu;
	if (ud > uu)
		dec = dec * ud / uu;
	else
		dec = dec / (uu / ud);
	parse->dec_part = (int)dec;
	parse->unit 	= ua[u_idx];

	return parse;
}

#if 0
int ci_printf_flag(ci_int_to_name_t *i2n, int flag)
{
	int rv = 0, first = 1;
	
	rv += ci_printf("[ ");
	u32_each_set(flag, idx) {
		if (!first)
			rv += ci_printf(" | ");
		
		const char *name = ci_int_to_name(i2n, 1 << idx);
		if (ci_strequal(name, CI_STR_UNDEFINED_VALUE))
			rv += ci_printf("%#X", 1 << idx);
		else
			rv += ci_printf("%s", name);

		first = 0;
	}

	rv += ci_printf(" ]");
	return rv;
}
#endif

int ci_snprintf_flag_str(char *buf, int len, ci_int_to_name_t *i2n, int flag)
{
	int first = 1;
	ci_mem_range_ex_def(mem, buf, buf + len);

	ci_mem_printf(&mem, "[ ");
	u32_each_set(flag, idx) {
		if (!first)
			ci_mem_printf(&mem, " | ");
		
		const char *name = ci_int_to_name(i2n, 1 << idx);
		if (ci_strequal(name, CI_STR_UNDEFINED_VALUE))
			ci_mem_printf(&mem, "%#X", 1 << idx);
		else
			ci_mem_printf(&mem, "%s", name);

		first = 0;
	}

	ci_mem_printf(&mem, " ]");
	return mem.end - mem.curr;
}

int ci_print_ruler(int len)
{
	int rv = 0, digit = 1;	
	
	ci_loop(ruler_i, len)	
		rv += ci_printf("%d", ruler_i % 10);	
	rv += ci_printfln();	
	ci_loop(ruler_i, len)	
		if (ruler_i % 10) {	
			if (--digit <= 0)	
				rv += ci_printf(" ");	
		} else {	
			digit = ci_nr_digit(ruler_i / 10);	
			rv += ci_printf("%d", ruler_i / 10);	
		}	
	rv += ci_printfln();	
	
	return rv;
}

const char *ci_pr_outline_str(int type)
{
	ci_range_check(type, 0, CI_PR_OUTLINE_NR);
	return outline_symbol[type];
}

static int __ci_box_row(int type, int *ary, int count, int parse_args, const char **name)
{
	int align, size, rv = 0;
	const char *str, *(*sym)[CI_PR_BOX_ID_NR];

	ci_range_check(type, 0, CI_PR_BOX_TYPE_NR);
	sym = &box_symbol[type];

	rv += ci_printf("%s", (*sym)[CI_PR_BOX_ID_LEFT]);
	ci_loop(i, count) {
		size = ary[i] & CI_PR_BOX_SIZE_MASK;
		i && (rv += ci_printf("%s", (*sym)[CI_PR_BOX_ID_VLINE]));

		if (!parse_args) 
			rv += ci_print_str_repeat(size + 2, (*sym)[CI_PR_BOX_ID_HLINE]);
		else {
			str = name[i];
			align = ary[i] & CI_PR_BOX_ALIGN_MASK;
//			align = type == CI_PR_BOX_TITLE_TXT ? CI_PR_BOX_ALIGN_CENTER : ary[i] & CI_PR_BOX_ALIGN_MASK;	/* override title to center */
			switch (align) {
				case CI_PR_BOX_ALIGN_LEFT:
					rv += ci_printf(ci_ssf(" %%-%ds ", size), str);
					break;
				case CI_PR_BOX_ALIGN_RIGHT:
					rv += ci_printf(ci_ssf(" %%%ds ", size), str);
					break;
				case CI_PR_BOX_ALIGN_CENTER: {
						int left = (size + 1 - ci_strlen(str)) / 2;
						int right = size - left;
						rv += ci_print_str_repeat(left + 1, " ");
						rv += ci_printf(ci_ssf("%%-%ds ", right), str);
						break;
					}
				default:
					ci_panic("unknown ary[%d]=%#X, align=%#X", i, ary[i], align);
			}
		}
	}
	rv += ci_printfln("%s", (*sym)[CI_PR_BOX_ID_RIGHT]);

	return rv;
}

int __ci_box_top(int *ary, int count, ...)
{
	int rv = 0;
	va_list args;
	const char *name[count];

	rv += __ci_box_row(CI_PR_BOX_TITLE_TOP, ary, count, 0, NULL);

	va_start(args, count);
	ci_va_list_to_str_ary(count, args, name); 
	va_end(args);
	rv += __ci_box_row(CI_PR_BOX_TITLE_TXT, ary, count, 1, name);

	rv += __ci_box_row(CI_PR_BOX_TITLE_BTM, ary, count, 0, NULL);
	return rv;
}

int __ci_box_body(int is_first, int *ary, int count, ...)
{
	int rv = 0;
	va_list args;
	const char *name[count];

	is_first || (rv += __ci_box_row(CI_PR_BOX_BODY_TOP, ary, count, 0, NULL));

	va_start(args, count);
	ci_va_list_to_str_ary(count, args, name); 
	va_end(args);
	rv += __ci_box_row(CI_PR_BOX_BODY_TXT, ary, count, 1, name);
	
	return rv;
}

int __ci_box_btm(int *ary, int count)
{
	return __ci_box_row(CI_PR_BOX_BODY_BTM, ary, count, 0, NULL);
}

int ci_box_top_ary(int *size_ary, const char **name_ary, int count)
{
	int rv = 0;
	rv += __ci_box_row(CI_PR_BOX_TITLE_TOP, size_ary, count, 0, NULL);
	rv += __ci_box_row(CI_PR_BOX_TITLE_TXT, size_ary, count, 1, name_ary);
	rv += __ci_box_row(CI_PR_BOX_TITLE_BTM, size_ary, count, 0, NULL);

	return rv;
}

int ci_box_body_ary(int is_first, int *size_ary, const char **name_ary, int count)
{
	int rv = 0;

	is_first || (rv += __ci_box_row(CI_PR_BOX_BODY_TOP, size_ary, count, 0, NULL));
	rv += __ci_box_row(CI_PR_BOX_BODY_TXT, size_ary, count, 1, name_ary);
	
	return rv;
}

int ci_box_btm_ary(int *size_ary, int count)
{
	return __ci_box_row(CI_PR_BOX_BODY_BTM, size_ary, count, 0, NULL);
}


