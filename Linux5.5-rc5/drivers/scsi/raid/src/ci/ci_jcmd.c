/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_jcmd.c					CI JSON command
 *                                                          hua.ye@Hua Ye.com
 */
#include "ci.h"

static ci_jcmd_opt_t *ci_jcmd_opt_by_name(ci_jcmd_opt_t *tab, char *name, int *long_matched)
{
	int name_len = ci_strlen(name);

	while (tab->name_long) {
		if (ci_strequal(name, tab->name_long)) {
			long_matched && (*long_matched = 1);
			return tab;
		}

		if (tab->name_short && (name_len == 1) && (name[0] == tab->name_short)) {
			long_matched && (*long_matched = 0);
			return tab;
		}
		
		tab++;
	}
	
	return NULL;
}

static int ci_jcmd_set_value(ci_json_data_t *jarg, ci_jcmd_opt_t *jcmd_opt)
{
	u64 u64_val = jarg->val.ui64;
	s64 s64_val = (s64)u64_val;

	if ((jarg->type == CI_JDT_STR) && !(jcmd_opt->flag & CI_JCMD_OPT_STRING)) {
		ci_err_printf("for option \"%s\", input is a string, number expected\n", jcmd_opt->name_long);
		return -CI_E_TYPE;
	} else if ((jarg->type != CI_JDT_STR) && (jcmd_opt->flag & CI_JCMD_OPT_STRING)) {
		ci_err_printf("for option \"%s\", input is a number, string expected\n", jcmd_opt->name_long);
		return -CI_E_TYPE;
	} else if ((jarg->type == CI_JDT_STR) && (jcmd_opt->flag & CI_JCMD_OPT_STRING)) {
		*(char **)jcmd_opt->ptr_var = (char *)u64_val;
		return 0;
	}

	if (jcmd_opt->ptr_signed) {
		switch (jcmd_opt->ptr_size) {
			case 1:
				*(s8 *)jcmd_opt->ptr_var = (s8)s64_val;
				if ((s64)*(s8 *)jcmd_opt->ptr_var != s64_val) {
					ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, s64_val, *(s8 *)jcmd_opt->ptr_var);
					return -CI_E_TRUNCATED;
				}
				break;
			case 2:
				*(s16 *)jcmd_opt->ptr_var = (s16)s64_val;
				if ((s64)*(s16 *)jcmd_opt->ptr_var != s64_val) {
					ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, s64_val, *(s16 *)jcmd_opt->ptr_var);
					return -CI_E_TRUNCATED;
				}
				break;
			case 4:
				*(s32 *)jcmd_opt->ptr_var = (s32)s64_val;
				if ((s64)*(s32 *)jcmd_opt->ptr_var != s64_val) {
					ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, s64_val, *(s32 *)jcmd_opt->ptr_var);
					return -CI_E_TRUNCATED;
				}
				break;
			case 8:
				*(s64 *)jcmd_opt->ptr_var = s64_val;
				break;
			default:
				ci_bug();
		}

		return 0;
	} 

	switch (jcmd_opt->ptr_size) {
		case 1:
			*(u8 *)jcmd_opt->ptr_var = (u8)u64_val;
			if ((u64)*(u8 *)jcmd_opt->ptr_var != u64_val) {
				ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, u64_val, *(u8 *)jcmd_opt->ptr_var);
				return -CI_E_TRUNCATED;
			}
			break;
		case 2:
			*(u16 *)jcmd_opt->ptr_var = (u16)u64_val;
			if ((u64)*(u16 *)jcmd_opt->ptr_var != u64_val) {
				ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, u64_val, *(u16 *)jcmd_opt->ptr_var);
				return -CI_E_TRUNCATED;
			}
			break;
		case 4:
			*(u32 *)jcmd_opt->ptr_var = (u32)u64_val;
			if ((u64)*(u32 *)jcmd_opt->ptr_var != u64_val) {
				ci_err_printf("data truncated for option \"%s\", input:%lld, get:%d\n", jcmd_opt->name_long, u64_val, *(u32 *)jcmd_opt->ptr_var);
				return -CI_E_TRUNCATED;
			}
			break;
		case 8:
			*(u64 *)jcmd_opt->ptr_var = u64_val;
			break;
		default:
			ci_bug();
	}	

	return 0;
}

int ci_jcmd_scan(ci_json_t *json, ci_jcmd_opt_t *tab)
{
	char *opt;
	int rv, help = 0;
	ci_jcmd_opt_t *jcmd_opt, *p = tab;
	ci_json_data_t *opt_arg_ary, *jarg;

//	json->flag |= CI_JSON_DUMP_DATA_TYPE; ci_json_dump(json);
	ci_json_get(json, "opt_arg_ary", &opt_arg_ary);

	if (opt_arg_ary)
		ci_json_ary_each(opt_arg_ary, data, {
			ci_exec_no_err(ci_json_obj_get(data, "opt", &opt));
			
			jcmd_opt = ci_jcmd_opt_by_name(tab, opt, NULL);		/* find the opt/arg define */
			if (!jcmd_opt) {
				(json->flag & CI_JSON_CLI) && ci_err_printf("unknown option \"%s\"\n", opt);
				return -CI_E_UNRECOGNIZED;
			}

			ci_json_obj_get(data, "arg", &jarg);	/* missing or unexpected argument */
			if (jarg && !jcmd_opt->ptr_var) {
				(json->flag & CI_JSON_CLI) && ci_err_printf("option \"%s\" doesn't take an argument\n", opt);
				return -CI_E_EXTRA;
			}
			if (!jarg && jcmd_opt->ptr_var) {
				(json->flag & CI_JSON_CLI) && ci_err_printf("option \"%s\" missing argument\n", opt);
				return -CI_E_MISSING;
			}

			ci_assert(jcmd_opt->ptr_flag);	/* set flag */
			if (*jcmd_opt->ptr_flag == 1) {
				(json->flag & CI_JSON_CLI) && ci_err_printf("option \"%s\" already set\n", opt);
				return -CI_E_DUPLICATE;
			}
			*jcmd_opt->ptr_flag = 1;
			if (jcmd_opt->flag & CI_JCMD_OPT_HELP)
				help = 1;
			
			if (jarg && ((rv = ci_jcmd_set_value(jarg, jcmd_opt)) < 0))
				return rv;
		});

	if (!help)
		while (p->name_long) {
			if ((p->flag & CI_JCMD_OPT_REQUIRED) && !*p->ptr_flag) {
				(json->flag & CI_JSON_CLI) && ci_err_printf("option \"%s\" not set\n", p->name_long);
				return -CI_E_NOT_SET;
			}
			p++;
		}
	
	return 0;
}

int ci_jcmd_no_opt(ci_json_t *json)
{
	ci_json_data_t *opt_arg_ary;
	return ci_json_get(json, "opt_arg_ary", &opt_arg_ary) < 0;
}

int ci_jcmd_dft_help(ci_jcmd_opt_t *tab)
{
	ci_jcmd_opt_t *t = tab;
	char *d_sign, *d_type;
	int size[5] = { 4, 5, 8, 4, 11 };

	while (t->name_long) {
		ci_max_set(size[0], ci_strlen(t->name_long) + 2);		/* 2 for "--" */
		t->desc && ci_max_set(size[4], ci_strlen(t->desc));
		t++;
	}

	size[1] |= CI_PR_BOX_ALIGN_CENTER;
	ci_box_top(size, "LONG", "SHORT", "REQUIRED", "TYPE", "DESCRIPTION");

	t = tab;
	while (t->name_long) {
		d_sign = "", d_type = "";

		if (t->ptr_var) {
			if (t->flag & CI_JCMD_OPT_STRING)
				d_type = "str";
			else if (t->ptr_signed && (t->ptr_size == ci_sizeof(int)))
				d_type = "int";
			else {
				d_sign = t->ptr_signed ? "s" : "u";
				d_type = ci_ssf("%d", t->ptr_size * 8);
			}
		}

		ci_box_body(t == tab, size, ci_ssf("--%s", t->name_long), t->name_short ? ci_ssf("-%c", t->name_short) : "",
				  t->flag & CI_JCMD_OPT_REQUIRED ? "required" : "optional", 
				  ci_ssf("%s%s", d_sign, d_type), ci_str_empty_if_null(t->desc));
		t++;
	}

	ci_box_btm(size);
	return 0;
}


