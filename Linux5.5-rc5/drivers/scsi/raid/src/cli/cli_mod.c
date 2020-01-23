/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli_mod.c				Command Line Interface via socket
 *                                                          hua.ye@Hua Ye.com
 */
#include "cli.h"

static ci_jcmd_t cli_jcmd[] = {
	{ "history", 	cli_jcmd_history, 	"show history",	"[--clear]" },
	{ "clear",		cli_jcmd_clear,		"clear the screen" },
	{ "quit",		cli_jcmd_quit,		"quit the cli" },
	{ "help",		cli_jcmd_help,		"show all commands", "[--help] [--module <module>]" },
	{ "ver",		cli_jcmd_ver,		"version" },
	CI_EOT
};

static void cli_mod_init(ci_mod_t *mod, ci_json_t *json)
{
	u8 *buf;
	cli_info_t *info = cli_info(mod);
	pal_sock_srv_info_t *sock_info = &info->sock_info;

	/* info init */
	ci_obj_zero(info);
	info->mod = mod;
	info->flag = CLI_INS_MODE;		// | CLI_CMD_NO_DUP;
	ci_list_init(&info->hst_head);

	buf = ci_shr_halloc(CLI_BUF_SIZE, 0, "cli.cmd_buf");
	ci_balloc_init(&info->hst_balloc, "cmd_buf", buf, buf + CLI_BUF_SIZE);

	/* socket info init */
	sock_info->port_nr = CLI_LISTEN_PORT;
	sock_info->char_handler = cli_char_handler;
	sock_info->event_handler = cli_event_handler;
	sock_info->ctx = info;

	/* other init */
	cli_key_init(info);

	/* done */
	ci_mod_init_done(mod, json);
}

static void cli_mod_start(ci_mod_t *mod, ci_json_t *json)
{
	cli_info_t *info = cli_info(mod);

	ci_jcmd_get_all_mod_jcmd_ary(&info->jcmd_all, &info->jcmd_all_count);
	info->jcmd_bmp = ci_shr_balloc(ci_bmp_size(info->jcmd_all_count));

	ci_assert(!info->json_vect);
	info->json_vect = json;		/* store context */

	pal_sock_srv_create(&info->sock_info);
	cli_imp_printf("cli socket server start, port_nr=%d\n", CLI_LISTEN_PORT);

#ifdef WIN_SIM
	extern int __pal_sock_srv_info_size;
	ci_assert(__pal_sock_srv_info_size == ci_sizeof(pal_sock_srv_info_t), "please sync pal_sock_srv_info_t");
#endif
}

static void cli_mod_stop(ci_mod_t *mod, ci_json_t *json)
{
	cli_info_t *info = cli_info(mod);

	pal_sock_srv_destroy(&info->sock_info);
	ci_imp_printf("cli socket server stopped, port_nr=%d\n", CLI_LISTEN_PORT);

	ci_mod_stop_done(mod, json);
}

ci_mod_def(mod_cli, {
	.name = "cli",
	.desc = "command line interface",

	.vect = {
		[CI_MODV_INIT]		= cli_mod_init,
		[CI_MODV_START]		= cli_mod_start,
		[CI_MODV_STOP]		= cli_mod_stop,
	},

	.mem.size_shr = ci_sizeof(cli_info_t),
	.jcmd = cli_jcmd
});


