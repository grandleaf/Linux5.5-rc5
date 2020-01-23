/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli.h				Command Line Interface via socket
 *                                                          hua.ye@Hua Ye.com
 */
#pragma once

#include "ci.h"

#define CLI_LISTEN_PORT					3333
#define CLI_BUF_SIZE					ci_balloc_mem_eval(ci_kib(128))		/* 256KB command buffer */
#define CLI_LINE_BUF_SIZE				ci_kib(1)							/* max chars per line */

#define CLI_PROMPT						"> "
#define CLI_CAND_CMD_PER_ROW			4
#define CLI_KEY_ESC_BASE				0x80	/* last ascii + 1 */
#define CLI_KEY_NR						(CLI_KEY_ESC_BASE + CLI_KEY_ESC_NR)


#define cli_info(mod)					((cli_info_t *)((mod)->mem.range_shr.start))
#define cli_info_from_sock_info(si)		((cli_info_t *)(si)->ctx)

enum {
	CLI_KEY_ESC,

	CLI_KEY_UP,
	CLI_KEY_DOWN,
	CLI_KEY_RIGHT,
	CLI_KEY_LEFT,

	CLI_KEY_HOME,
	CLI_KEY_INS,
	CLI_KEY_DEL,
	CLI_KEY_END,
	CLI_KEY_PAGE_UP,
	CLI_KEY_PAGE_DOWN,

	CLI_KEY_F1,
	CLI_KEY_F2,
	CLI_KEY_F3,
	CLI_KEY_F4,
	CLI_KEY_F5,
	CLI_KEY_F6,
	CLI_KEY_F7,
	CLI_KEY_F8,
	CLI_KEY_F9,
	CLI_KEY_F10,
	CLI_KEY_F11,
	CLI_KEY_F12,

	CLI_KEY_ESC_NR		/* total escape sequence keys */
};

ci_bmp_def(cli_esc_map, CLI_KEY_ESC_NR);
#define cli_esc_map_each_set(bmp, idx)		ci_bmp_each_set(bmp, CLI_KEY_ESC_NR, idx)


typedef struct {
	ci_list_t						 link;
	int								 id;			/* assign each command a unique id */
	char							 buf[0];
} cli_cmd_t;

typedef struct {
	int								 flag;
#define CLI_CMD_NO_DUP			0x0001				/* no duplicate command in history */
#define CLI_INS_MODE			0x0002				/* insert/overwrite mode */
#define CLI_ESC_MODE			0x0004				/* escape mode */
#define CLI_UP_DOWN_MODE		0x0008				/* arrow/page up/down */

	int								 hst_id;		/* hst_id++ when new command arrives */
	ci_list_t					 	 hst_head;		/* history: queue all the command that issued (lru) */
	ci_balloc_t					 	 hst_balloc;	/* memory allocator for commands in history */

	cli_esc_map_t					 esc_map;		/* for escape sequence detection */
	int								 esc_char_pos;	/* which char we are detecting now */

	int								 buf_len;
	int								 cur_pos;
	char							 line_buf[CLI_LINE_BUF_SIZE];
	char							 scratch_buf[CLI_LINE_BUF_SIZE];

	ci_jcmd_t					   **jcmd_all;		/* pointer[] to all jcmds of all modules */
	int								 jcmd_all_count;
	ci_bmp_t						*jcmd_bmp;		/* for partial command candidates */

	ci_mod_t						*mod;			/* pointer to mod */
	ci_json_t						*json_vect;		/* store context for async vector call */
	cli_cmd_t						*up_down_cmd; 		 
	pal_sock_srv_info_t			 	 sock_info;		/* for socket communciation */
} cli_info_t;


/*	
 *	Since the CLI has its own thread, which is outside of the node/worker context,
 *	we need to define the following print macros
 */
#define cli_printf(...)				ci_mod_printf("cli", __VA_ARGS__)
#define cli_printfln(...)			ci_mod_printfln("cli", __VA_ARGS__)
#define cli_imp_printf(...)			ci_ntc_mod_printf(CI_PR_NTC_IMP, "cli", __VA_ARGS__)	


int  cli_key_init(cli_info_t *info);
void cli_char_handler(pal_sock_srv_info_t *sock_info, char ch);
void cli_event_handler(pal_sock_srv_info_t *sock_info, int event);
void cli_show_prompt(cli_info_t *info, int newline);
void cli_line_buf_handler(cli_info_t *info);
int  cli_cmd_handler(cli_info_t *info, char *cmd);
int  cli_get_json(cli_info_t *info, char *cmd, ci_json_t **json);

/* cli json command handler */
int cli_jcmd_history(ci_mod_t *mod, ci_json_t *json);
int cli_jcmd_clear(ci_mod_t *mod, ci_json_t *json);
int cli_jcmd_quit(ci_mod_t *mod, ci_json_t *json);
int cli_jcmd_help(ci_mod_t *mod, ci_json_t *json);
int cli_jcmd_ver(ci_mod_t *mod, ci_json_t *json);



