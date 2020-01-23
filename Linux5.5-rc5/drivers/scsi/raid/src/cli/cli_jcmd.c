/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli_jcmd.c				CLI Command Handling
 *                                                          hua.ye@Hua Ye.com
 */
#include "cli.h"

static void cli_intl_line_buf_handler(cli_info_t *info);			/* will destroy the command */


static cli_cmd_t *cli_cmd_alloc(cli_info_t *info, char *cmd)
{
	cli_cmd_t *cli_cmd;
	int size = ci_strsize(cmd) + ci_sizeof(cli_cmd_t);

	while (!(cli_cmd = (cli_cmd_t *)ci_balloc(&info->hst_balloc, size))) {
		cli_cmd = ci_list_del_head_obj(&info->hst_head, cli_cmd_t, link);
		ci_assert(cli_cmd);
		ci_bfree(&info->hst_balloc, cli_cmd);
	}
	
	return cli_cmd;
}

static void cli_cmd_hist_add(cli_info_t *info, char *cmd, int id)
{
	cli_cmd_t *cli_cmd;

	if ((cli_cmd = ci_list_tail_obj(&info->hst_head, cli_cmd_t, link)) && ci_strequal(cli_cmd->buf,cmd))
		return;		/* repeated last command, skip adding */

	cli_cmd = cli_cmd_alloc(info, cmd);
	ci_strlcpy(cli_cmd->buf, cmd, CLI_LINE_BUF_SIZE);
	cli_cmd->id = id >= 0 ? id : info->hst_id++;
	ci_list_add_tail(&info->hst_head, &cli_cmd->link);
}

static char *cli_get_cooked_cmd(cli_info_t *info)
{
	char *cmd = ci_str_strip(info->line_buf);
	return cmd;
}

/* return the id */
static int cli_remove_dup(cli_info_t *info, char *cmd)	
{
	int id = -1;
	cli_cmd_t *cli_cmd;

	ci_list_each_safe(&info->hst_head, cli_cmd, link)
		if (ci_strequal(cmd, cli_cmd->buf)) {
			id = cli_cmd->id;
			ci_list_del(&cli_cmd->link);
			ci_bfree(&info->hst_balloc, cli_cmd);
			break;
		}

	return id;
}

static int cli_intl_cmd_invoke_history(cli_info_t *info, char *cmd)
{
	char *endp;
	cli_cmd_t *cli_cmd;
	int idx = ci_str_to_u64(cmd + 1, &endp, 10);

	if (endp != ci_str_end(cmd) - 1) 	/* garbage character other than 0~9 detected */
		goto __exit;

	ci_list_each_safe(&info->hst_head, cli_cmd, link)
		if (idx == cli_cmd->id) {
			ci_strlcpy(info->line_buf, cli_cmd->buf, CLI_LINE_BUF_SIZE);
			cli_intl_line_buf_handler(info);
			return 0;
		}	

__exit:
	pal_sock_send_str(&info->sock_info, cmd);
	pal_sock_send_str(&info->sock_info, ": event not found\n");
	return -CI_E_INVALID;
}

static void cli_intl_line_buf_handler(cli_info_t *info)	/* will destroy the command */
{
	int id = -1;
	char *cmd = cli_get_cooked_cmd(info);

	if (*cmd == '!') {	/* !<Number> means invoke a command in history */
		cli_intl_cmd_invoke_history(info, cmd);
		return;
	}

	if (!*cmd)	/* user simply pressed enter */
		return;

	if (info->flag & CLI_CMD_NO_DUP)
		id = cli_remove_dup(info, cmd);
	
	cli_cmd_hist_add(info, cmd, id);

	ci_printf_info->flag |= CI_PRF_NOMETA | CI_PRF_CLI;
	cli_cmd_handler(info, cmd);
	ci_printf_info->flag &= ~(CI_PRF_NOMETA | CI_PRF_CLI);
}

void cli_line_buf_handler(cli_info_t *info)
{
	pal_sock_send_str(&info->sock_info, "\n\r");
	cli_intl_line_buf_handler(info);
	cli_show_prompt(info, 0);
}

int cli_cmd_handler(cli_info_t *info, char *cmd_line)	/* will destroy the command */
{
	int rv;
	char *cmd;
	ci_json_t *json;

	if ((rv = cli_get_json(info, cmd_line, &json)) < 0)
		return rv;

	ci_exec_no_err(ci_json_get(json, "cmd", &cmd));

	ci_mod_each(mod, {
		ci_jcmd_t *jcmd = mod->jcmd;
		while (jcmd && jcmd->name) {
			if (ci_strequal(cmd, jcmd->name)) {
				ci_assert(jcmd->handler);
				rv = jcmd->handler(info->mod, json);
				goto __exit;
			}
			
			jcmd++;
		}
	});	

	pal_sock_send_str(&info->sock_info, "! ");
	pal_sock_send_str(&info->sock_info, cmd);
	pal_sock_send_str(&info->sock_info, ": command not found\n");

__exit:
	ci_json_destroy(json);
//	ci_balloc_dump_pending(&ci_node_info->ba_json);

	return 0;
}

int cli_jcmd_history(ci_mod_t *mod, ci_json_t *json)
{
	int rv;
	cli_cmd_t *cli_cmd;
	char fmt[] = " %?d %s\n";
	cli_info_t *info = cli_info(mod);

	struct {
		struct {	/* all flags are u8 */
			u8		clear;
			u8		help;
		} flag;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_nil_arg(&opt, clear, 	'c',	"clear the history"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};	
	
	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;

	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);

	if (opt.flag.clear) {
		ci_list_each_safe(&info->hst_head, cli_cmd, link)
			ci_bfree(&info->hst_balloc, cli_cmd);	/* ci_bfree() will invoke ci_list_dbg_poison_set() anyway */
		ci_list_init(&info->hst_head);
		return 0;
	}

	*ci_strstr(fmt, "?") = '0' + ci_nr_digit(info->hst_id);

	ci_list_each(&info->hst_head, cli_cmd, link) {
		ci_snprintf(info->scratch_buf, CLI_LINE_BUF_SIZE, fmt, cli_cmd->id, cli_cmd->buf);
		pal_sock_send_str(&info->sock_info, info->scratch_buf);
	}	

	return 0;
}

int cli_jcmd_clear(ci_mod_t *mod, ci_json_t *json)
{
	cli_info_t *info = cli_info(mod);
	
	ci_jcmd_require_no_opt(json);
	pal_sock_send_str(&info->sock_info, "\e[2J");		/* telnet: clear */
	pal_sock_send_str(&info->sock_info, "\e[0;0H");		/* telnet: move cursor to 0, 0 */
	
	return 0;
}

int cli_jcmd_quit(ci_mod_t *mod, ci_json_t *json)
{
	cli_info_t *info = cli_info(mod);

	ci_jcmd_require_no_opt(json);
	pal_sock_clt_close(&info->sock_info);
	return 0;
}

int cli_jcmd_help(ci_mod_t *mod, ci_json_t *json)
{
	ci_mod_t **mod_ary;
	ci_jcmd_t *jcmd, **jcmd_ary;
	int rv, nr_mod, cmd_len, desc_len, usage_len, jcmd_cnt;
	ci_mod_max_len_t *max_len = &ci_mod_info->max_len;

	struct {
		struct {	/* all flags are u8 */
			u8		 module;
			u8		 help;
		} flag;

		char 		*module;
	} opt = CI_EOT;

	ci_jcmd_opt_t opt_tab[] = {
		ci_jcmd_opt_optional_has_arg(&opt, module, 	'm',	"cli commands for the module"),
		ci_jcmd_opt_optional_hlp_arg(&opt, help, 	'h', 	"show help"),
		CI_EOT
	};	

	if ((rv = ci_jcmd_scan(json, opt_tab)) < 0)
		return rv;

	if (opt.flag.help)
		return ci_jcmd_dft_help(opt_tab);


	if (opt.flag.module) {
		ci_jcmd_t *jcmd;
		ci_mod_t *m;
		
		cmd_len = desc_len = usage_len = 0;
		if ((m = ci_mod_get(opt.module)) && (jcmd = m->jcmd)) 
			while (jcmd->name) {
				ci_max_set(cmd_len, ci_strlen(jcmd->name));
				jcmd->desc && ci_max_set(desc_len, ci_strlen(jcmd->desc));
				jcmd->usage && ci_max_set(usage_len, ci_strlen(jcmd->usage));
				jcmd++;
			}
	} else {
		cmd_len = max_len->jcmd_name;
		desc_len = max_len->jcmd_desc;
		usage_len = max_len->jcmd_usage;
	}

	int size[] = { cmd_len, desc_len, usage_len };
	ci_jcmd_get_mod_ary(&mod_ary, &nr_mod);

	rv = 0;	/* how many modules are printed */
	ci_loop(i, nr_mod) {
		ci_mod_t *m = mod_ary[i];
		jcmd = m->jcmd;
		
		ci_jcmd_get_mod_jcmd_ary(m, &jcmd_ary, &jcmd_cnt);
		if (!jcmd_ary)
			continue;

		!opt.flag.module && i && cli_printf("\n\n");
		if (opt.flag.module && !ci_strequal(m->name, opt.module)) {
			ci_jcmd_put_mod_jcmd_ary(&jcmd_ary);
			continue;
		}

		rv++;
		cli_printfln("[ %s, \"%s\" ]", m->name, m->desc);
		ci_box_top(size, "COMMAND", "DESCRIPTION", "USAGE");

		ci_loop(i, jcmd_cnt) {
			jcmd = jcmd_ary[i];
			ci_box_body(!i, size, jcmd->name, ci_str_empty_if_null(jcmd->desc), ci_str_empty_if_null(jcmd->usage));
		}
		ci_box_btm(size);
		ci_jcmd_put_mod_jcmd_ary(&jcmd_ary);
	}
	
	ci_jcmd_put_mod_ary(&mod_ary);

	if (rv == 0)
		cli_printf("unknown module \"%s\" or the module doesn't have a cli interface\n", opt.module);
	
	return 0;
}

int cli_jcmd_ver(ci_mod_t *mod, ci_json_t *json)
{
	ci_jcmd_require_no_opt(json);
	cli_printf("%s, %s, %s Build %s\n", __DATE__, __TIME__, __RELEASE_DEBUG__, ci_info.build_nr);		
	return 0;
}

