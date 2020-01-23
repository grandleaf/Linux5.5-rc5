/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_jcmd.h					CI JSON command
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_json.h"


typedef struct {
	char			*name_long;
	char			 name_short;
	int				 flag;
#define CI_JCMD_OPT_REQUIRED			0x0001
#define CI_JCMD_OPT_OPTIONAL			0x0002
#define CI_JCMD_OPT_STRING				0x0004	/* this is a string */
#define CI_JCMD_OPT_HELP				0x0008	/* user types -h or --help */
	
	u8				*ptr_flag;
	void			*ptr_var;

	u8				 ptr_signed;		/* *ptr_val signed/unsigned */
	u8				 ptr_size;			/* *ptr_val's size */
	const char		*desc;				/* a detailed description */
} ci_jcmd_opt_t;


#define ci_jcmd_opt_flag_str(opt, __ptr_name)		\
	(ci_type_is_str_ptr(*(&(opt)->__ptr_name)) ? CI_JCMD_OPT_STRING : 0)
#define ci_jcmd_opt_optional_has_arg(opt, __ptr_name, __name_short, __desc)	\
	{ #__ptr_name, __name_short, CI_JCMD_OPT_OPTIONAL | ci_jcmd_opt_flag_str(opt, __ptr_name), 	\
			&(opt)->flag.__ptr_name, &(opt)->__ptr_name, \
			(u8)ci_type_is_signed(ci_typeof(*(&(opt)->__ptr_name))), (u8)ci_sizeof(*(&(opt)->__ptr_name)), __desc }
#define ci_jcmd_opt_optional_nil_arg(opt, __ptr_name, __name_short, __desc)	\
	{ #__ptr_name, __name_short, CI_JCMD_OPT_OPTIONAL, &(opt)->flag.__ptr_name, NULL, 0, 0, __desc }
#define ci_jcmd_opt_optional_hlp_arg(opt, __ptr_name, __name_short, __desc)	\
	{ #__ptr_name, __name_short, CI_JCMD_OPT_OPTIONAL | CI_JCMD_OPT_HELP, &(opt)->flag.__ptr_name, NULL, 0, 0, __desc }
#define ci_jcmd_opt_required_has_arg(opt, __ptr_name, __name_short, __desc)	\
	{ #__ptr_name, __name_short, CI_JCMD_OPT_REQUIRED | ci_jcmd_opt_flag_str(opt, __ptr_name), 	\
			&(opt)->flag.__ptr_name, &(opt)->__ptr_name, \
			(u8)ci_type_is_signed(ci_typeof(*(&(opt)->__ptr_name))), (u8)ci_sizeof(*(&(opt)->__ptr_name)), __desc }
#define ci_jcmd_opt_required_nil_arg(opt, __ptr_name, __name_short, __desc)	\
	{ #__ptr_name, __name_short, CI_JCMD_OPT_REQUIRED, &(opt)->flag.__ptr_name, NULL, 0, 0, __desc }

#define ci_jcmd_require_no_opt(json)		\
	do {	\
		if (!ci_jcmd_no_opt(json)) {		/* why has opt/arg */	\
			ci_printf("! error: unexpected argument\n");	\
			return -CI_E_EXTRA;	\
		}	\
	} while (0)


int ci_jcmd_scan(ci_json_t *json, ci_jcmd_opt_t *tab);
int ci_jcmd_no_opt(ci_json_t *json);
int ci_jcmd_dft_help(ci_jcmd_opt_t *tab);


