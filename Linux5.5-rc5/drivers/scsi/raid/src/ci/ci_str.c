/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_str.c						CI String related Functions
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

int ci_str_first_char(const char *str, char c)
{
	int i = 0;

	while (*str && (*str != c))
		str++, i++;

	return *str ? i : -CI_E_NOT_FOUND;
}

int ci_str_last_char(const char *str, char c)
{
	int i = ci_strlen(str);
	const char *p = str + i - 1;

	while (i > 0) {
		if (*p == c)
			return i;
		i--, p--;
	}

	return -CI_E_NOT_FOUND;
}

int ci_str_last_path_sep_idx(const char *str)
{
	int sep_idx = ci_str_last_char(str, '/');
	if (sep_idx < 0)
		sep_idx = ci_str_last_char(str, '\\');
	if (sep_idx < 0)
		sep_idx = 0;

	return sep_idx;
}

char *ci_str_file_base_name(const char *str)
{
	return (char *)str + ci_str_last_path_sep_idx(str);
}

char *ci_str_lstrip(char *str)
{
	while (*str) {
		if (!ci_char_is_blank(*str))
			break;
		str++;
	}

	return str;
}

char *ci_str_rstrip(char *str)
{
	char *p = str + ci_strlen(str) - 1;

	while (p >= str) {
		if (!ci_char_is_blank(*p)) 
			break;
		*p-- = 0;
	}

	return str;
}

char *ci_str_strip(char *str)
{
	return ci_str_lstrip(ci_str_rstrip(str));
}

char *ci_valid_str(const char *str, int len)
{
	char *first = (char *)str, *last = first + len;

	while (*str && (str < last)) {
		if (!ci_char_is_printable(*str))
			return "INVALID_STRING";
		str++;
	}

	return first;
}

char *ci_str_end(const char *str)
{
	while (*str++)
		;
	return (char *)str;
}

char *ci_strnotchrnul(const char *str, char c)
{
	char *p = (char *)str;
	ci_assert(c);

	while (*p == c)
		p++;
	
	return p;
}

int ci_str_count_char(const char *str, char c)
{
	int rv = 0;
	while (*str) {
		(*str == c) && rv++;
		str++;
	}

	return rv;
}

