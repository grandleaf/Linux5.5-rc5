/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_str.h					CI String related Functions
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"

#define CI_ASCII_NUL				0x00		/* null */
#define CI_ASCII_SOH				0x01		/* start of heading */
#define CI_ASCII_STX				0x02		/* start of text */
#define CI_ASCII_ETX				0x03		/* end of text */
#define CI_ASCII_EOT				0x04		/* end of transmission */
#define CI_ASCII_ENQ				0x05		/* enquiry */
#define CI_ASCII_ACK				0x06		/* acknowledge */
#define CI_ASCII_BEL				0x07		/* bell */
#define CI_ASCII_BS					0x08		/* backspace */
#define CI_ASCII_TAB				0x09		/* horizontal tab */
#define CI_ASCII_LF					0x0A		/* NL line feed, new line */
#define CI_ASCII_VT					0x0B		/* vertical tab */
#define CI_ASCII_FF					0x0C		/* NP form feed, new page */
#define CI_ASCII_CR					0x0D		/* carriage return */
#define CI_ASCII_SO					0x0E		/* shift out */
#define CI_ASCII_SI					0x0F		/* shift in */
#define CI_ASCII_DLE				0x10		/* data link escape */
#define CI_ASCII_DC1				0x11		/* device control 1 */
#define CI_ASCII_DC2				0x12		/* device control 2 */
#define CI_ASCII_DC3				0x13		/* device control 3 */
#define CI_ASCII_DC4				0x14		/* device control 4 */
#define CI_ASCII_NAK				0x15		/* negative acknowledge */
#define CI_ASCII_SYN				0x16		/* synchronous idle */
#define CI_ASCII_ETB				0x17		/* end of trans, block */
#define CI_ASCII_CAN				0x18		/* cancel */
#define CI_ASCII_EM					0x19		/* end of medium */
#define CI_ASCII_SUB				0x1A		/* substitute */
#define CI_ASCII_ESC				0x1B		/* escape */
#define CI_ASCII_FS					0x1C		/* file separator */
#define CI_ASCII_GS					0x1D		/* group separator */
#define CI_ASCII_RS					0x1E		/* record separator */
#define CI_ASCII_US					0x1F		/* unit separator */

/* [ 0x20, 0x7E ]  printable ASCII characters */
#define CI_ASCII_SPACE				0x20		/* space */

#define CI_ASCII_DEL				0x7F		/* DEL */


#define ci_char_is_space(c)			((c) == CI_ASCII_SPACE)
#define ci_char_is_lower(c)			(((c) >= 'a') && ((c) <= 'z'))
#define ci_char_is_upper(c)			(((c) >= 'A') && ((c) <= 'Z'))
#define ci_char_is_digit(c) 		(((c) >= '0') && ((c) <= '9'))
#define ci_char_is_xdigit(c)		(ci_char_is_digit(c) || (((c) >= 'a') && ((c) <= 'f')) || (((c) >= 'A') && ((c) <= 'F')))
#define ci_char_is_alpha(c)			((((c) >= 'a') && ((c) <= 'z')) || (((c) >= 'A') && ((c) <= 'Z')))
#define ci_char_to_upper(c)			(((c) >= 'a') && ((c) <= 'z') ? ((c) - 'a') + 'A' : (c))
#define ci_char_to_lower(c)			(((c) >= 'A') && ((c) <= 'Z') ? ((c) - 'A') + 'a' : (c))
#define ci_char_is_printable(c)		(((c) >= ' ') && (c) <= '~')
#define ci_char_is_blank(c)			(((c) == CI_ASCII_SPACE) || ((c) == CI_ASCII_TAB) || ((c) == CI_ASCII_CR) || ((c) == CI_ASCII_LF))

#define ci_str_str_if_null(s, d)	((s) ? (s) : (d))
#define ci_str_na_if_null(s)		ci_str_str_if_null(s, "n/a")
#define ci_str_empty_if_null(s)		ci_str_str_if_null(s, "")

#define ci_valid_file_name(s)		ci_valid_str(s, CI_MAX_FILE_NAME_LEN)
#define ci_valid_func_name(s)		ci_valid_str(s, CI_MAX_FUNC_NAME_LEN)
#define ci_strspacenul(s)			ci_strchrnul(s, CI_ASCII_SPACE)
#define ci_strnotspacenul(s)		ci_strnotchrnul(s, CI_ASCII_SPACE)


int   ci_str_first_char(const char *str, char c);
int   ci_str_last_char(const char *str, char c);
int   ci_str_last_path_sep_idx(const char *str);		/* find the location of "/" or "\" in a path, return 0 if not found */
char *ci_str_file_base_name(const char *str);			/* /usr/local/bin/test.c => test.c */
char *ci_str_lstrip(char *str);							/* keep source, remove space/tab/cr/lf at the beginning of a string */
char *ci_str_rstrip(char *str);							/* destroy source, remove space/tab/cr/lf at the end of a string */
char *ci_str_strip(char *str);							/* destroy source, remove space/tab/cr/lf at the beginning/end of a string */
char *ci_valid_str(const char *str, int len);			/* if it is a valid string, return itself, else return "INVALID_STRING" */
char *ci_str_end(const char *str);						/* return the pointer to \000 */
char *ci_strnotchrnul(const char *str, char c);			/* find first non-c char, if not find, return the end nul pointer */
int   ci_str_count_char(const char *str, char c);		/* how many c in the string */


