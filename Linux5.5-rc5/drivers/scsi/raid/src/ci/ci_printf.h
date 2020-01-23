/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * ci_printf.h				CI Print Functions
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci_cfg.h"
#include "ci_type.h"
#include "ci_util.h"
#include "ci_printf_def.h"

#define CI_PRN_FMT_TIME_SEC					"%d.%03d"
#define CI_PRN_VAL_TIME_SEC(p)				p.int_part, p.dec_part		
#define CI_TIME_US_PARSE					{ .flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_FIXED, .fixed = 1000000, .dec_len = 3, .unit = NULL }
#define CI_PR_META_BUF_SIZE					256	
#define CI_PR_MAX_PREFIX_LEN				5	
#define CI_PR_PREFIX_FMT					" %" ci_m_to_str(CI_PR_MAX_PREFIX_LEN) "s: "

#define CI_PR_NTC_WARN						'!'		/* warning */
#define CI_PR_NTC_ERR						'#'		/* error */
#define CI_PR_NTC_IMP						'*'		/* important */	

#define CI_PR_INDENT						"    "

/* print little helper */
#define CI_PR_PCT_FMT						"%lld.%03lld%%"
#define ci_pr_pct_val(a, b)					((a) * 1000000ull / (b) + 5) / 10000, 	\
											((a) * 1000000ull / (b) + 5) / 10 - ((a) * 1000000ull / (b) + 5) / 10000 * 1000
											
#define CI_PR_BNP_FMT						"%d.%03d %s (%lli)"
#define CI_PR_BNP_EX_FMT(x)					"%" #x "d.%03d %s (%lli)"
#define __ci_pr_bnp_val(x)					(x)->int_part, (x)->dec_part, (x)->unit, (x)->big_nr
#define ci_pr_bnp_val(x)					__ci_pr_bnp_val(ci_make_bnp(x))

#define ci_make_bnp(x)						ci_big_nr_parse(&((ci_big_nr_parse_t) { 	\
												.flag = CI_BIG_NR_PARSE_ROUND | CI_BIG_NR_PARSE_POWER_OF_2, 	\
												.dec_len = 3,		\
												.big_nr = (x) }))

#define CI_PRN_FMT_NODE_WORKER				"%02d/%02d"
#define CI_PRN_VAL_NODE_WORKER				ci_sched_ctx() ? ci_sched_ctx()->worker->node_id : -1, 	\
											ci_sched_ctx() ? ci_sched_ctx()->worker->worker_id : -1	




/* structure for divide a big number into int_part.dec_part style */
typedef struct {
	int				 flag;
#define CI_BIG_NR_PARSE_POWER_OF_2		0x0001
#define CI_BIG_NR_PARSE_UNIT_VERBOSE	0x0002
#define CI_BIG_NR_PARSE_ROUND			0x0004
#define CI_BIG_NR_PARSE_FIXED			0x0008

	u64				 big_nr;
	u64				 fixed;				/* do not automatically choose unit, value = 1000 ^ n */
	int				 int_part;
	int				 dec_part;
	int				 dec_len;			/* [0, CI_BIG_NR_PARSE_MAX_DEC_LEN] */
#define CI_BIG_NR_PARSE_MAX_DEC_LEN		5	
	const char		*unit;				/* Gi, Mi, Ki, "" or G, M, K, "" */
} ci_big_nr_parse_t;

/* print configuration & data */
typedef struct {
	int					 flag;
#define CI_PRF_CR						0x0001			/* carriage return */
#define CI_PRF_SLK						0x0002			/* spinlock initialized */
#define CI_PRF_NOMETA					0x0004			/* don't print meta info */
#define CI_PRF_CONSOLE					0x0008			/* console connected */
#define CI_PRF_CLI						0x0010			/* only print to console */
#define CI_PRF_PREFIX					0x1000			/* override prefix */
#define CI_PRF_NO_INDENT				0x2000			/* ignore the indent */

	int				 	 meta_flag;
#define CI_PRF_META_SEQ					0x0001			/* print global seq number from 1 */
#define CI_PRF_META_LOCAL_TIME			0x0002			/* print local date time */
#define CI_PRF_META_SCHED_CTX			0x0004			/* print scheduler's context */
#define CI_PRF_META_UTC_TIME			0x0008			/* print utc date time */
#define CI_PRF_META_FILE				0x0010			/* print file name */
#define CI_PRF_META_FUNC				0x0020			/* print function name */
#define CI_PRF_META_LINE				0X0040			/* print source line */

	u64				 	 seq_nr;
	const char			*file;
	const char			*func;
	int					 line;
	
	char		 	 	 ntc;
	const char			*prefix;
	const char		   	*prefix_override;					/* when this is set, use it as prefix */
	int					 indent;

	ci_slk_t		 	 line_lock;							/* make a single printf thread safe */
	ci_slk_t			 block_lock;						/* make multiple printfs thread safe */
	
	char			 	 meta_buf[CI_PR_META_BUF_SIZE];
	char 			 	 print_buf[CI_PR_BUF_SIZE];			/* print buffer */

	ci_mem_range_ex_t	 range_mem_buf;
	char			 	 mem_buf[CI_PR_MEM_BUF_SIZE];		/* use this if console is not available */
} ci_printf_info_t;


#define ci_print_str_repeat(x, s)	\
	({		\
		ci_loop(__dash_counter__, (x))		\
			ci_printf("%s", s);		\
		(x);	\
	})
#define ci_print_hline(x)				ci_print_str_repeat(x, CI_PR_STR_HLINE)
#define ci_print_hlineln(x)				(ci_print_hline(x) + ci_printfln())
#define __ci_printf(...)				__ci_ntc_mod_printf(' ', "ci", __VA_ARGS__)

#define ci_imp_printf(...)				ci_ntc_printf(CI_PR_NTC_IMP, 	__VA_ARGS__)	
#define ci_warn_printf(...)				ci_ntc_printf(CI_PR_NTC_WARN, 	__VA_ARGS__)	
#define ci_err_printf(...)				ci_ntc_printf(CI_PR_NTC_ERR, 	__VA_ARGS__)	

#define ci_imp_printfln(...)			ci_ntc_printfln(CI_PR_NTC_IMP, 	__VA_ARGS__)	
#define ci_warn_printfln(...)			ci_ntc_printfln(CI_PR_NTC_WARN, __VA_ARGS__)	
#define ci_err_printfln(...)			ci_ntc_printfln(CI_PR_NTC_ERR, 	__VA_ARGS__)

#define ci_m_printf(m, ...)				ci_ntc_mod_printf(' ', 				(m)->name, __VA_ARGS__)
#define ci_m_imp_printf(m, ...)			ci_ntc_mod_printf(CI_PR_NTC_IMP, 	(m)->name, __VA_ARGS__)
#define ci_m_warn_printf(m, ...)		ci_ntc_mod_printf(CI_PR_NTC_WARN, 	(m)->name, __VA_ARGS__)
#define ci_m_err_printf(m, ...)			ci_ntc_mod_printf(CI_PR_NTC_ERR, 	(m)->name, __VA_ARGS__)

#define ci_m_printfln(m, ...)			ci_ntc_mod_printfln(' ', 			(m)->name, __VA_ARGS__)
#define ci_m_imp_printfln(m, ...)		ci_ntc_mod_printfln(CI_PR_NTC_IMP, 	(m)->name, __VA_ARGS__)
#define ci_m_warn_printfln(m, ...)		ci_ntc_mod_printfln(CI_PR_NTC_WARN, (m)->name, __VA_ARGS__)
#define ci_m_err_printfln(m, ...)		ci_ntc_mod_printfln(CI_PR_NTC_ERR, 	(m)->name, __VA_ARGS__)


#define ci_buf_snprintf(buf, len, ...)			\
	((buf) ? ci_snprintf((buf), (len), __VA_ARGS__) : (ci_printf(__VA_ARGS__), 0))
#define __ci_buf_snprintf(buf, len, ...)		\
	((buf) ? __ci_snprintf((buf), (len), __VA_ARGS__) : (__ci_printf(__VA_ARGS__), 0))


#define ci_mem_printf(mem_range, ...)		\
	((mem_range)->curr += ci_snprintf((char *)(mem_range)->curr, (mem_range)->end - (mem_range)->curr, __VA_ARGS__))


#if 0	
#define ci_nometa_printf(...)		/* don't print meta */	\
	({	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			int __ci_printf_rv__;	\
			__ci_printf_check(__VA_ARGS__);		\
			\
			__ci_printf_lock(); 	\
			__ci_printf_set_loc(__FILE__, __func__, __LINE__, "ci", ' ');	\
			ci_printf_info->flag |= CI_PRF_NOMETA;		\
			__ci_printf_rv__ = __ci_raw_printf(__VA_ARGS__);	\
			ci_printf_info->flag &= ~CI_PRF_NOMETA;		\
			__ci_printf_unlock();	\
			__ci_printf_rv__;		\
		)(0;)		\
	})
#define ci_cli_printf(...)		/* print to cli console without meta */	\
	({	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			int __ci_printf_rv__;	\
			__ci_printf_check(__VA_ARGS__);		\
			\
			__ci_printf_lock(); 	\
			__ci_printf_set_loc(__FILE__, __func__, __LINE__, "ci", ' ');	\
			ci_printf_info->flag |= CI_PRF_NOMETA | CI_PRF_CLI;		\
			__ci_printf_rv__ = __ci_raw_printf(__VA_ARGS__);	\
			ci_printf_info->flag &= ~(CI_PRF_NOMETA | CI_PRF_CLI);		\
			__ci_printf_unlock();	\
			__ci_printf_rv__;		\
		)(0;)		\
	})	
#endif	

/* prints with ntc & mod, print notice and mod name as prefix */
#define __ci_ntc_mod_printf(ntc, mod, ...)	\
	({	\
		int __ci_printf_rv__;	\
		\
		__ci_printf_try_block_lock();	\
		__ci_printf_line_lock(); 	\
		\
		__ci_printf_set_loc(__FILE__, __func__, __LINE__, mod, ntc);	\
		__ci_printf_rv__ = __ci_raw_printf(__VA_ARGS__);	\
		\
		__ci_printf_line_unlock();	\
		__ci_printf_rv__;		\
	})

/* a wrapper for __ci_ntc_mod_printf() */
#define ci_ntc_mod_printf(ntc, mod, ...) 	\
	({	\
		int __ci_ntc_printf_rv__;	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			__ci_printf_check(__VA_ARGS__);		\
			__ci_ntc_printf_rv__ = __ci_ntc_mod_printf(ntc, mod, __VA_ARGS__);	\
		)(	\
			__ci_ntc_printf_rv__ = __ci_ntc_mod_printf(ntc, mod, "%s", "");	\
		)	\
		__ci_ntc_printf_rv__;	\
	})	
#define ci_ntc_mod_printfln(ntc, mod, ...)	\
	({	\
		int __ci_ntc_mod_printfln__ = ci_ntc_mod_printf(ntc, mod, __VA_ARGS__);		\
		__ci_ntc_mod_printfln__ += ci_ntc_mod_printf(ntc, mod, "\n");		\
	})

/* override the prefix */
#define ci_ntc_mode_flag_printf(ntc, mod, flag, ...)	\
	({	\
		int __ci_printf_rv__;	\
		\
		__ci_printf_try_block_lock();	\
		__ci_printf_line_lock(); 	\
		\
		if (flag & CI_PRF_PREFIX) 	\
			__ci_printf_set_prefix_override(mod);	\
		\
		__ci_printf_set_flag(flag);	\
		__ci_printf_set_loc(__FILE__, __func__, __LINE__, mod, ntc);	\
		__ci_printf_rv__ = __ci_raw_printf(__VA_ARGS__);	\
		\
		__ci_printf_clear_prefix_override();	\
		__ci_printf_clear_flag(flag);	\
		\
		__ci_printf_line_unlock();	\
		__ci_printf_rv__;		\
	})
#define ci_ntc_mode_flag_printfln(ntc, mod, flag, ...)	\
	({	\
		int __ci_ntc_mode_flag_printfln__ = ci_ntc_mode_flag_printf(ntc, mod, flag, __VA_ARGS__);		\
		__ci_ntc_mode_flag_printfln__ += ci_ntc_mode_flag_printf(ntc, mod, flag, "\n");		\
	})	

/* print with ntc, takes "ci" as the default mod name */
#define ci_ntc_printf(ntc, ...)		ci_ntc_mod_printf(ntc, "ci", __VA_ARGS__)	
#define ci_ntc_printfln(ntc, ...)		\
	({	\
		int __ci_ntc_printfln__ = ci_ntc_printf(ntc, __VA_ARGS__);		\
		__ci_ntc_printfln__ += ci_printf("\n");		\
	})

/* print with mod name, takes ' ' as default ntc */
#define ci_mod_printf(mod, ...)	\
	({	\
		ci_m_if_else(ci_m_has_args(__VA_ARGS__))(	\
			__ci_printf_check(__VA_ARGS__);		\
			__ci_ntc_mod_printf(' ', mod, __VA_ARGS__);	\
		)(0;)		\
	})
#define ci_mod_printfln(mod, ...)		\
	({		\
		int __ci_printfln__ = ci_mod_printf(mod, __VA_ARGS__);		\
		__ci_printfln__ += ci_mod_printf(mod, "\n");		\
	})

/* takes "ci" as mod name, ' ' as ntc */
#define ci_printf(...)		ci_mod_printf("ci", __VA_ARGS__)
#define ci_printfln(...)	ci_mod_printfln("ci", __VA_ARGS__)

/* block lock protector */
#define ci_printf_block_lock(...)		\
	do {	\
		__ci_printf_block_lock();		\
		__VA_ARGS__;		\
		__ci_printf_block_unlock();		\
	} while (0)


#define ci_snprintf(...)	\
	({	\
		__ci_sprintf_check(__VA_ARGS__); 	\
		__ci_snprintf(__VA_ARGS__);	\
	})
#define ci_scnprintf(...)	\
	({	\
		__ci_sprintf_check(__VA_ARGS__);	\
		__ci_scnprintf(__VA_ARGS__); \
	})

#define ci_vscnprintf(...)	\
	({	\
		__ci_vsprintf_check(__VA_ARGS__);	\
		__ci_vscnprintf(__VA_ARGS__); \
	})
#define ci_vsnprintf(...)	\
	({	\
		__ci_vsprintf_check(__VA_ARGS__);	\
		__ci_vsnprintf(__VA_ARGS__); \
	})		

#define ci_sscanf(...)	\
	({	\
		__ci_sscanf_check(__VA_ARGS__);	\
		__ci_sscanf(__VA_ARGS__); \
	})		
#define ci_vsscanf(...)	\
	({	\
		__ci_vsscanf_check(__VA_ARGS__);	\
		__ci_vsscanf(__VA_ARGS__); \
	})	

#define ci_snprintf_buf(...)		\
	({	\
		__ci_sprintf_check(__VA_ARGS__); 	\
		__ci_snprintf_buf(__VA_ARGS__);	\
	})
#define ci_flag_str(i2n, flag)		\
	({	\
		char *__buf__ = (char[CI_PR_FLAG_LEN]) { 0 };	\
		ci_snprintf_flag_str(__buf__, CI_PR_FLAG_LEN, i2n, flag);	\
		__buf__;	\
	})	
	
#define __ci_sf(len, ...)	\
	({	\
		char *__buf__ = ci_snprintf_buf((char[len]) { 0 }, len, __VA_ARGS__);	\
		ci_assert(ci_strlen(__buf__) < len - 1, #len " too small");		\
		__buf__;		\
	})
#define ci_ssf(...)							__ci_sf(CI_PR_SSF_LEN, __VA_ARGS__)
#define ci_msf(...)							__ci_sf(CI_PR_MSF_LEN, __VA_ARGS__)
#define ci_lsf(...)							__ci_sf(CI_PR_LSF_LEN, __VA_ARGS__)


/* box drawing wrapper */
#define ci_box_top(ary, ...)				__ci_box_top(ary, ci_m_argc(__VA_ARGS__), __VA_ARGS__)
#define ci_box_body(is_first, ary, ...)		__ci_box_body(is_first, ary, ci_m_argc(__VA_ARGS__), __VA_ARGS__)
#define ci_box_btm(ary)						__ci_box_btm(ary, ci_nr_elm(ary))


/* function define */
int ci_printf_pre_init();
int ci_printf_init();
int ci_snprintf_flag_str(char *buf, int len, ci_int_to_name_t *i2n, int flag);
u64 ci_str_to_u64(char *cp, char **epdp, int base);
int ci_print_ruler(int len);
const char *ci_pr_outline_str(int type);

int ci_box_top_ary(int *size_ary, const char **name_ary, int count);
int ci_box_body_ary(int is_first, int *size_ary, const char **name_ary, int count);
int ci_box_btm_ary(int *size_ary, int count);


/*
 * Don't use the __foo() directly, use the foo() version for checking purpose instead.
 */
int __ci_raw_printf(const char *format, ...);
int __ci_snprintf(char * buf, int size, const char *fmt, ...);
int __ci_scnprintf(char * buf, int size, const char *fmt, ...);

int __ci_vscnprintf(char *buf, int size, const char *fmt, va_list args);
int __ci_vsnprintf(char *buf, int size, const char *fmt, va_list args);

int __ci_sscanf(const char *buf, const char *fmt, ...);
int __ci_vsscanf(const char *buf, const char *fmt, va_list args);

char *__ci_snprintf_buf(char *buf, int size, const char *fmt, ...);

// int ci_vsprintf(char *buf, const char *fmt, va_list args);		// comment it out since it doesn't have buffer overflow check
// int ci_sprintf(char * buf, const char *fmt, ...);				// comment it out since it doesn't have buffer overflow check

ci_big_nr_parse_t *ci_big_nr_parse(ci_big_nr_parse_t *parse);


/*
 * Helper functions
 */
void __ci_printf_line_lock();
void __ci_printf_line_unlock();
void __ci_printf_block_lock();
void __ci_printf_block_unlock();
void __ci_printf_try_block_lock();
void __ci_printf_set_loc(const char *file, const char *func, int line, const char *prefix, char ntc);
void __ci_printf_set_prefix_override(const char *prefix_override);
void __ci_printf_clear_prefix_override();
void __ci_printf_set_flag(int flag);
void __ci_printf_clear_flag(int flag);

int __ci_box_top(int *ary, int count, ...);
int __ci_box_body(int is_first, int *ary, int count, ...);
int __ci_box_btm(int *ary, int count);



