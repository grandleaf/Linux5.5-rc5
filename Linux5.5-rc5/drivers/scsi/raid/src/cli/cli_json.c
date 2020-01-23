/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli_json.c				Convert string to a JSON object
 *                                                          hua.ye@Hua Ye.com
 */
#include "cli.h"

/* private data structures and definitions */
typedef struct {
	int			 state;
#define CLI_JSONS_CMD					0
#define CLI_JSONS_OPT					1
#define CLI_JSONS_OPT_ARG				2
#define CLI_JSONS_ARG					3

	int			 flag;
#define CLI_JSONF_STR					0x0001
#define CLI_JSONF_BIN					0x0002

	cli_info_t	*cli_info;
	char		*start;
	char		*end;
	char 		*err_ptr;		/* parse error at this location */
} cli_json_cookie_t;

enum {
	CLI_JSON_OA_OPT_LONG,		/* option arg long option */
	CLI_JSON_OA_OPT_SHORT,
	CLI_JSON_OA_ARG_STR,
	CLI_JSON_OA_ARG_BIN,
	CLI_JSON_OA_ARG_NUM
};



/* parse quoted string and put into scratch_buf */
static int cli_parse_quote(cli_info_t *info, char *start, int *bin, int *rlen, int *wlen)	
{
	char *buf, *wp, *rp;
	int rv, hex, hex_mode, escape_mode;

	rv = -CI_E_INVALID; 
	hex_mode = escape_mode = *bin = 0;

	buf = info->scratch_buf;
	wp = buf, rp = start;

	for (;;) {
		if (!*rp) 	/* unexpected end of string */
			goto __exit;

		if (!escape_mode) {		/* normal string scan, not escape mode */
			if (*rp == '\\') {
				escape_mode = 1;
				rp++;
				continue;
			}
			
			if (*rp == '"')	{	/* success: end of string */
				rv = 0;
				goto __exit;	
			}
			
			*wp++ = *rp++;
			continue;
		}

		/* detect escape mode's hex value */
		if (hex_mode) {
			if (!ci_char_is_xdigit(*rp)) {	/* not 0..9, a..f, A..F */
				rp++;
				goto __exit;
			}

			hex = (hex << 4) | (ci_char_is_digit(*rp) ? *rp - '0' : ci_char_to_upper(*rp) - 'A' + 10);
			rp++;

			if (++hex_mode == 3) {	/* \x?? parse finished */
				hex_mode = escape_mode = 0;
				*wp++ = (char)hex;
			}

			continue;
		} 

		/* escape mode */
		switch (*rp) {
			case '\\':
			case '"':
				*wp++ = *rp++;
				escape_mode = 0;
				break;
			case 'x':
				rp++;
				*bin = 1;
				hex = 0;
				hex_mode = 1;
				break;
			default:
				rp++;
				goto __exit;		/* unknown escape string */
		}
	}

__exit:
	*wp = 0;
	*rlen = (int)(rp - start);
	*wlen = (int)(wp - buf);
	return rv;
}

static int cli_json_opt_arg_detect(ci_json_t *json)
{
	cli_json_cookie_t *cookie = (cli_json_cookie_t *)json->cookie;
	int len = (int)(cookie->end - cookie->start);

	if (cookie->flag & CLI_JSONF_STR)
		return CLI_JSON_OA_ARG_STR;

	if (cookie->flag & CLI_JSONF_BIN)
		return CLI_JSON_OA_ARG_BIN;

	if ((len > 2) && (cookie->start[0] == '-') && (cookie->start[1] == '-'))		/* --option */
		return CLI_JSON_OA_OPT_LONG;

	if ((len > 1) && (cookie->start[0] == '-')) {		/* -abcdefg */
		char *p = cookie->start;
		while (++p < cookie->end) {
			if (!ci_char_is_digit(*p))
				return CLI_JSON_OA_OPT_SHORT;
		}
		/* num that < 0 */
	}

	return CLI_JSON_OA_ARG_NUM;
}

static int cli_json_opt_exist(ci_json_t *json, char *check_name)
{
	char *name;
	ci_json_data_t *opt_arg_ary;

	if (ci_json_get(json, "opt_arg_ary", &opt_arg_ary) < 0)
		return 0;

	ci_json_ary_each(opt_arg_ary, opt, {
		ci_exec_no_err(ci_json_obj_get(opt, "opt", &name));
		if (ci_strequal(name, check_name))
			return -CI_E_EXIST;
	});

	return 0;
}

static int cli_json_do_opt(ci_json_t *json, ci_json_data_t *opt_arg_ary)
{
	int oa_type, rv = 0;
	ci_json_data_t *opt_arg;
	cli_json_cookie_t *cookie = (cli_json_cookie_t *)json->cookie;

	oa_type = cli_json_opt_arg_detect(json);

	/* --long_option handling */
	if (oa_type == CLI_JSON_OA_OPT_LONG) {		
		if ((rv = cli_json_opt_exist(json, cookie->start + 2)) < 0) {
			cookie->err_ptr += 2;
			return rv;
		}
			
		opt_arg = ci_json_ary_add_obj(opt_arg_ary);
		ci_json_obj_set(opt_arg, "opt", cookie->start + 2);
		return 0;
	} 

	/* --short_option handling, expand -abc to a,b,c */ 
	if (oa_type == CLI_JSON_OA_OPT_SHORT) {	
		while (cookie->start++ < cookie->end - 1) {
			char short_opt[2] = { *cookie->start };

			if (*cookie->start == '-') 
				return -CI_E_FORMAT;
			
			if ((rv = cli_json_opt_exist(json, short_opt)) < 0) {
				cookie->err_ptr = cookie->start;
				return rv;
			}
			
			opt_arg = ci_json_ary_add_obj(opt_arg_ary);
			ci_json_obj_set(opt_arg, "opt", short_opt);
		}
		
		return 0;
	}
		
	/* error, not long or short option */
	return -CI_E_FORMAT;
}

static int cli_json_set_num(ci_json_t *json, ci_json_data_t *opt_arg)
{
	u64 val;
	s64 neg_val;
	char *endp;
	int is_negative = 0;
	cli_json_cookie_t *cookie = (cli_json_cookie_t *)json->cookie;

	is_negative = *cookie->start == '-';
	val = ci_str_to_u64(cookie->start + is_negative, &endp, 0);
	if (!is_negative)
		ci_json_obj_set(opt_arg, "arg", val);
	else {
		neg_val = -(s64)val;
		ci_json_obj_set(opt_arg, "arg", neg_val);
	}

	if (endp != cookie->end) {
		cookie->err_ptr = endp;
		return -CI_E_FORMAT;
	}

	return 0;
}

static int cli_json_set(ci_json_t *json)
{
	int opt_arg_ary_rv, oa_type = -1, rv = 0;
	ci_json_data_t *opt_arg, *opt_arg_ary;
	cli_json_cookie_t *cookie = (cli_json_cookie_t *)json->cookie;
	char ch = *cookie->end;

	*cookie->end = 0;
	cookie->err_ptr = cookie->start;
	opt_arg_ary_rv = ci_json_get(json, "opt_arg_ary", &opt_arg_ary);

__again:
	switch (cookie->state) {
		case CLI_JSONS_CMD:
			ci_json_set(json, "cmd", cookie->start);
			cookie->state = CLI_JSONS_OPT;
			break;
		case CLI_JSONS_OPT:
			if (opt_arg_ary_rv < 0)		/* create array if first option */
				opt_arg_ary = ci_json_set_ary(json, "opt_arg_ary");

			rv = cli_json_do_opt(json, opt_arg_ary);
			cookie->state = CLI_JSONS_OPT_ARG;		/* next will be another option or argument for this option */
			break;
		case CLI_JSONS_OPT_ARG:
			oa_type = cli_json_opt_arg_detect(json);
			cookie->state = (oa_type == CLI_JSON_OA_OPT_LONG) || (oa_type == CLI_JSON_OA_OPT_SHORT) ? 
							 CLI_JSONS_OPT : CLI_JSONS_ARG;
			goto __again;		/* redo opt/arg after peek */
		case CLI_JSONS_ARG:
			ci_json_ary_get_last(opt_arg_ary, &opt_arg);
			if (oa_type == CLI_JSON_OA_ARG_STR)
				ci_json_obj_set(opt_arg, "arg", cookie->start);
			else if (oa_type == CLI_JSON_OA_ARG_BIN)
				ci_json_obj_set(opt_arg, "arg", cookie->start, cookie->end - cookie->start);
			else {
				ci_assert(oa_type == CLI_JSON_OA_ARG_NUM);
				rv = cli_json_set_num(json, opt_arg);
			}
			
			cookie->state = CLI_JSONS_OPT;
			break;
		default:
			ci_bug();
			break;
	}

	*cookie->end = ch;
	return rv;
}

int cli_get_json(cli_info_t *info, char *cmd, ci_json_t **json)
{
	int rlen, wlen, bin, epos = -1;
	char *s, *e, *p, *q = cmd, *buf = info->scratch_buf;

	epos = -1;
	cli_json_cookie_t cookie = { .state = CLI_JSONS_CMD, .cli_info = info };

	ci_assert(json);
	*json = ci_json_create("cli_cmd");
	(*json)->cookie = &cookie;
	(*json)->flag |= CI_JSON_CLI;		/* command from cli */

	for (;;) {
		if (!*(p = ci_strnotspacenul(q)))
			break;

		cookie.flag = 0;
		if (*p != '"') 
			s = p, e = q = ci_strspacenul(p);		/* scan without ".*" */
		else {
			if ((cli_parse_quote(info, p + 1, &bin, &rlen, &wlen) < 0) || !rlen) {
				epos = rlen + (int)(p - cmd);
				goto __exit;
			}

			cookie.flag |= bin ? CLI_JSONF_BIN : CLI_JSONF_STR;
			q = p + rlen + 2;		/* forward q */
			if ((*q != ' ') && (*q != 0)) {		/* expect a space separator or NULL */
				epos = (int)(q - cmd);
				goto __exit;
			}

			s = buf, e = s + wlen;
		}

		cookie.start = s, cookie.end = e;
		if (cli_json_set(*json) < 0) {
			epos = (int)(cookie.err_ptr - s) + (int)(p - cmd);
			break;
		}
	}

__exit:
	if (epos >= 0) {
		pal_sock_send_str(&info->sock_info, "! Error parsing command line:\n  ");
		pal_sock_send_str(&info->sock_info, cmd);
		pal_sock_send_str(&info->sock_info, "\n  ");
		
		ci_loop(epos)
			pal_sock_send_char(&info->sock_info, ' ');
		pal_sock_send_str(&info->sock_info, "^\n");

		ci_json_destroy(*json);
		*json = NULL;
		return -CI_E_FORMAT;
	}

	return 0;
}


