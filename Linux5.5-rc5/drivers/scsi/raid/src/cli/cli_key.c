/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli_key.c				CLI Key Detection
 *                                                          hua.ye@Hua Ye.com
 */
#include "cli.h"

/* for escape sequence detection */
static const char *esc_seq_map[CLI_KEY_ESC_NR] = {	
	[CLI_KEY_ESC]		= "\x1B",
		
	[CLI_KEY_UP]		= "[A",
	[CLI_KEY_DOWN]		= "[B",
	[CLI_KEY_RIGHT]		= "[C",
	[CLI_KEY_LEFT]		= "[D",

	[CLI_KEY_HOME]		= "[1~",
	[CLI_KEY_INS]		= "[2~",
	[CLI_KEY_DEL]		= "[3~",
	[CLI_KEY_END]		= "[4~",
	[CLI_KEY_PAGE_UP]	= "[5~",
	[CLI_KEY_PAGE_DOWN]	= "[6~",

	[CLI_KEY_F1]		= "[[A",
	[CLI_KEY_F2]		= "[[B",
	[CLI_KEY_F3]		= "[[C",
	[CLI_KEY_F4]		= "[[D",
	[CLI_KEY_F5]		= "[[E",
	[CLI_KEY_F6]		= "[17~",
	[CLI_KEY_F7]		= "[18~",
	[CLI_KEY_F8]		= "[19~",
	[CLI_KEY_F9]		= "[20~",
	[CLI_KEY_F10]		= "[21~",
	[CLI_KEY_F11]		= "[23~",
	[CLI_KEY_F12]		= "[24~",
};

ci_unused static ci_int_to_name_t esc_key_to_name[] = {
	ci_int_name(CLI_KEY_ESC),
	ci_int_name(CLI_KEY_UP),
	ci_int_name(CLI_KEY_DOWN),
	ci_int_name(CLI_KEY_RIGHT),
	ci_int_name(CLI_KEY_LEFT),
	ci_int_name(CLI_KEY_HOME),
	ci_int_name(CLI_KEY_INS),
	ci_int_name(CLI_KEY_DEL),
	ci_int_name(CLI_KEY_END),
	ci_int_name(CLI_KEY_PAGE_UP),
	ci_int_name(CLI_KEY_F1),
	ci_int_name(CLI_KEY_F2),
	ci_int_name(CLI_KEY_F3),
	ci_int_name(CLI_KEY_F4),
	ci_int_name(CLI_KEY_F5),
	ci_int_name(CLI_KEY_F6),
	ci_int_name(CLI_KEY_F7),
	ci_int_name(CLI_KEY_F8),
	ci_int_name(CLI_KEY_F9),
	ci_int_name(CLI_KEY_F10),
	ci_int_name(CLI_KEY_F11),
	ci_int_name(CLI_KEY_F12),
	CI_EOT	
};

typedef void (*cli_key_handler_t)(cli_info_t *info, int key);


static void cli_reset_status(cli_info_t *info)
{
	info->flag &= ~CLI_UP_DOWN_MODE;
	info->up_down_cmd = NULL;
}

static void cli_reset_line_buf(cli_info_t *info)
{
	info->buf_len = info->cur_pos = 0;
	info->line_buf[0] = 0;
}

static void cli_set_line_buf(cli_info_t *info, char *cmd)
{
	ci_assert(ci_strsize(cmd) <= CLI_LINE_BUF_SIZE);
	info->buf_len = info->cur_pos = ci_strlen(cmd);
	ci_strlcpy(info->line_buf, cmd, CLI_LINE_BUF_SIZE);
	
	pal_sock_send_str(&info->sock_info, cmd);
}

static void cli_cursor_off(cli_info_t *info)		/* move cursor faster */
{
	pal_sock_send_str(&info->sock_info, "\e[?25l");
}

static void cli_cursor_on(cli_info_t *info)
{
	pal_sock_send_str(&info->sock_info, "\e[?25h");
}

static void cli_clear_line(cli_info_t *info)
{
	if (!info->buf_len && !info->cur_pos && !info->line_buf[0])		/* nothing to do */
		return;

	cli_cursor_off(info);
	
	ci_loop(info->cur_pos)	/* home */
		pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	ci_loop(info->buf_len)	/* white space wipe */
		pal_sock_send_char(&info->sock_info, ' ');
	ci_loop(info->buf_len)	/* home */
		pal_sock_send_char(&info->sock_info, CI_ASCII_BS);

	cli_reset_line_buf(info);
	cli_cursor_on(info);
}

/* scan the line_buf, result put in scratch_buf */
static char *cli_get_partial_command(cli_info_t *info)
{
	char *start, *dst;
	
	if (!*(start = ci_strnotchrnul(info->line_buf, ' ')))
		return NULL;

	dst = info->scratch_buf;
	while ((start < &info->line_buf[info->cur_pos]) && (*start != ' '))
		*dst++ = *start++;
	
	*dst = 0;
	return start == &info->line_buf[info->cur_pos] ? info->scratch_buf : NULL;	/* space in string? */
}

static int cli_get_partial_candidate(cli_info_t *info, const char *prtl_cmd)
{
	int rv = 0;
	ci_jcmd_t **p = info->jcmd_all;
	int cmp_len = ci_strlen(prtl_cmd);

	ci_assert(cmp_len > 0);
	ci_bmp_zero(info->jcmd_bmp, info->jcmd_all_count);

	ci_loop(i, info->jcmd_all_count) {
		const char *cmd = (*p)->name;
		if (ci_strnequal(cmd, prtl_cmd, cmp_len)) {
			rv++;
			ci_bmp_set_bit(info->jcmd_bmp, info->jcmd_all_count, i);
		}
		p++;
	}
	
	return rv;
}

static void cli_handler_key_printable(cli_info_t *info, int key)
{
	int copy_len;
	
	if (info->buf_len >= CLI_LINE_BUF_SIZE - 1) {
		pal_sock_send_str(&info->sock_info, "\nThe input line is too long.");
		cli_show_prompt(info, 1);
		return;
	}

	if (info->cur_pos == info->buf_len) {	/* common case, append char */
		info->line_buf[info->cur_pos++] = key;	
		info->line_buf[info->cur_pos] = 0;
		info->buf_len++;
		pal_sock_send_char(&info->sock_info, key);
		return;	
	}

	if (!(info->flag & CLI_INS_MODE)) {		/* replace mode */
		info->line_buf[info->cur_pos++] = key;	
		ci_max_set(info->buf_len, info->cur_pos);
		pal_sock_send_char(&info->sock_info, key);
		return;	
	}

	/* move the buffer forward */
	copy_len = info->buf_len - info->cur_pos + 1;
	ci_memmove(info->line_buf + info->cur_pos + 1, info->line_buf + info->cur_pos, copy_len);
	info->line_buf[info->cur_pos++] = key;
	info->buf_len++;

	/* send string, then cursor restore location */
	cli_cursor_off(info);
	pal_sock_send_str(&info->sock_info, info->line_buf + info->cur_pos - 1);
	ci_loop(copy_len - 1)
		pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	cli_cursor_on(info);
}

static void cli_handler_key_unprintable(cli_info_t *info, int key)
{
	/* ignore */
	ci_dbg_exec(cli_printf("NOT printable: %X\n", key));
}

static void cli_handler_ascii_nul(cli_info_t *info, int key)
{
	/* ignore, NULL comes from telnet's CR (0x0D, 0x00) */
}

static void cli_handler_ascii_etx(cli_info_t *info, int key)
{
	pal_sock_send_str(&info->sock_info, "^C");
	cli_show_prompt(info, 1);
}

static void cli_handler_ascii_cr_lf(cli_info_t *info, int key)
{
	cli_line_buf_handler(info);		/* will destroy the command */
}

/* enter the escape mode */
static void cli_handler_ascii_esc(cli_info_t *info, int key)
{
	info->flag |= CLI_ESC_MODE;
	cli_esc_map_fill(&info->esc_map);
	info->esc_char_pos = 0;
}

static void cli_handler_key_esc(cli_info_t *info, int key)
{
	cli_reset_status(info);
	cli_clear_line(info);
}

static void cli_handler_key_up(cli_info_t *info, int key)
{
	cli_cmd_t *cli_cmd;
	
	if (!(info->flag & CLI_UP_DOWN_MODE)) {
		if(!(cli_cmd = ci_list_tail_obj(&info->hst_head, cli_cmd_t, link)))		/* empty */
			return;
		info->flag |= CLI_UP_DOWN_MODE;
		info->up_down_cmd = cli_cmd;
	} else {
		ci_assert(info->up_down_cmd);
		if (!(cli_cmd = ci_list_prev_obj(&info->hst_head, info->up_down_cmd, link)))
			return;		/* no prev */
		
		info->up_down_cmd = cli_cmd;		/* move prev */
	}

	ci_assert(cli_cmd);
	cli_clear_line(info);
	cli_set_line_buf(info, cli_cmd->buf);
}

static void cli_handler_key_down(cli_info_t *info, int key)
{
	cli_cmd_t *cli_cmd;
	
	if (!(info->flag & CLI_UP_DOWN_MODE)) 
		return;		/* we already at element */

	ci_assert(info->up_down_cmd);
	if (!(cli_cmd = ci_list_next_obj(&info->hst_head, info->up_down_cmd, link)))
		return;		/* no next */

	info->up_down_cmd = cli_cmd;	/* move next */
	ci_assert(cli_cmd);
	cli_clear_line(info);
	cli_set_line_buf(info, cli_cmd->buf);
}

static void cli_handler_key_left(cli_info_t *info, int key)
{
	if (info->cur_pos <= 0)
		return;
	
//	pal_sock_send_str(&info->sock_info, "\033[1D");
	pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	info->cur_pos--;
}

static void cli_handler_key_right(cli_info_t *info, int key)
{
	if (info->cur_pos >= info->buf_len)
		return;

//	pal_sock_send_str(&info->sock_info, "\033[1C");
	pal_sock_send_char(&info->sock_info, info->line_buf[info->cur_pos]);
	info->cur_pos++;
}

static void cli_handler_key_home(cli_info_t *info, int key)
{
	cli_cursor_off(info);
	ci_loop(info->cur_pos)
		pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	info->cur_pos = 0;		
	cli_cursor_on(info);
}

static void cli_handler_key_end(cli_info_t *info, int key)
{
	cli_cursor_off(info);
	ci_loop(i, info->cur_pos, info->buf_len)
		pal_sock_send_char(&info->sock_info, info->line_buf[info->cur_pos++]);
	cli_cursor_on(info);
}

static void cli_handler_key_ins(cli_info_t *info, int key)
{
	info->flag ^= CLI_INS_MODE;
}

static void cli_handler_key_del(cli_info_t *info, int key)
{
	int copy_len;
	
	if (info->cur_pos >= info->buf_len)
		return;

	copy_len = info->buf_len - info->cur_pos;
	ci_memmove(info->line_buf + info->cur_pos, info->line_buf + info->cur_pos + 1, copy_len);
	info->buf_len--;

	if (key == CI_ASCII_BEL)	/* the caller turn it off already */
		cli_cursor_off(info);
	pal_sock_send_str(&info->sock_info, info->line_buf + info->cur_pos);
	pal_sock_send_char(&info->sock_info, ' ');
	ci_loop(copy_len)
		pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	cli_cursor_on(info);
}

static void cli_handler_ascii_bs(cli_info_t *info, int key)
{
	if (info->cur_pos <= 0)
		return;

	info->cur_pos--;
	cli_cursor_off(info);	/* turn on by cli_handler_key_del() */
	pal_sock_send_char(&info->sock_info, CI_ASCII_BS);
	cli_handler_key_del(info, key);
}

static void cli_handler_ascii_tab(cli_info_t *info, int key)
{
	int nr_cand, idx, count, move_left;
	const char *rest_cmd, *prtl_cmd /* point to scatch_buf */;

	if (!(prtl_cmd = cli_get_partial_command(info)))
		return;

	if ((nr_cand = cli_get_partial_candidate(info, prtl_cmd)) <= 0)
		return;

	if (nr_cand == 1) {		/* only one candidate, finish it */
		idx = ci_bmp_first_set(info->jcmd_bmp, info->jcmd_all_count);
		ci_assert(idx >= 0);
		rest_cmd = info->jcmd_all[idx]->name + ci_strlen(prtl_cmd);
		while (*rest_cmd) 
			cli_handler_key_printable(info, *rest_cmd++);
		if (info->cur_pos == info->buf_len)
			cli_handler_key_printable(info, ' ');
		return;
	}

	/* print all the candidates */
	pal_sock_send_str(&info->sock_info, "\n");
	count = 0;
	ci_bmp_each_set(info->jcmd_bmp, info->jcmd_all_count, idx) {
		pal_sock_send_str(&info->sock_info, ci_ssf("%-20s", info->jcmd_all[idx]->name));
		if (!(++count % CLI_CAND_CMD_PER_ROW))
			pal_sock_send_str(&info->sock_info, "\n");
	}
	if (count % CLI_CAND_CMD_PER_ROW)
		pal_sock_send_str(&info->sock_info, "\n");

	/* print user's input */
	pal_sock_send_str(&info->sock_info, CLI_PROMPT);
	pal_sock_send_str(&info->sock_info, info->line_buf);

	/* incase the cursor is not at the end */
	move_left = info->buf_len - info->cur_pos;
	if (move_left > 0) {
		info->cur_pos = info->buf_len;
		ci_loop(move_left)
			cli_handler_key_left(info, CLI_KEY_ESC_BASE + CLI_KEY_LEFT);
	}
}

static void cli_handler_key_page_up(cli_info_t *info, int key)
{
	cli_handler_key_up(info, key);
}

static void cli_handler_key_page_down(cli_info_t *info, int key)
{
	cli_handler_key_down(info, key);
}

static void cli_sim_user_input(cli_info_t *info, char *str)
{
	pal_sock_send_str(&info->sock_info, str);
	ci_strlcpy(info->line_buf, str, CLI_LINE_BUF_SIZE);
	info->cur_pos = info->buf_len = ci_strlen(str);
	cli_line_buf_handler(info);
}

static void cli_handler_key_fn(cli_info_t *info, int key)
{
	cli_cmd_t *cli_cmd;
	int key_id = key - CLI_KEY_F1 + 1;

	switch (key_id) {
		case 1:
			cli_handler_key_esc(info, key);
			cli_sim_user_input(info, "help");
			break;
		case 2:
			cli_handler_key_esc(info, key);
			cli_sim_user_input(info, "history");
			break;
		case 3: 
			if ((cli_cmd = ci_list_tail_obj(&info->hst_head, cli_cmd_t, link))) {
				cli_handler_key_esc(info, key);
				cli_sim_user_input(info, cli_cmd->buf);
			}
			break;
		default:
			break;
	}
}


static cli_key_handler_t cli_key_handler[CLI_KEY_NR] = { 
	[CI_ASCII_NUL]							= cli_handler_ascii_nul,
	[CI_ASCII_ETX]							= cli_handler_ascii_etx,	/* ctrl-c */
	[CI_ASCII_BS]							= cli_handler_ascii_bs,
	[CI_ASCII_TAB]							= cli_handler_ascii_tab,
	[CI_ASCII_CR]							= cli_handler_ascii_cr_lf,
	[CI_ASCII_LF]							= cli_handler_ascii_cr_lf,
	[CI_ASCII_ESC]							= cli_handler_ascii_esc,	/* enter escape mode */
	/* end of ascii table */
		
	[CLI_KEY_ESC_BASE + CLI_KEY_ESC]		= cli_handler_key_esc,		/* to handle esc/esc sequence */
		
	[CLI_KEY_ESC_BASE + CLI_KEY_UP]			= cli_handler_key_up,
	[CLI_KEY_ESC_BASE + CLI_KEY_DOWN]		= cli_handler_key_down,
	[CLI_KEY_ESC_BASE + CLI_KEY_RIGHT]		= cli_handler_key_right,
	[CLI_KEY_ESC_BASE + CLI_KEY_LEFT]		= cli_handler_key_left,

	[CLI_KEY_ESC_BASE + CLI_KEY_HOME]		= cli_handler_key_home,
	[CLI_KEY_ESC_BASE + CLI_KEY_INS]		= cli_handler_key_ins,
	[CLI_KEY_ESC_BASE + CLI_KEY_DEL]		= cli_handler_key_del,
	[CLI_KEY_ESC_BASE + CLI_KEY_END]		= cli_handler_key_end,
	[CLI_KEY_ESC_BASE + CLI_KEY_PAGE_UP]	= cli_handler_key_page_up,
	[CLI_KEY_ESC_BASE + CLI_KEY_PAGE_DOWN]	= cli_handler_key_page_down,
	
	[CLI_KEY_ESC_BASE + CLI_KEY_F1]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F2]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F3]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F4]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F5]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F6]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F7]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F8]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F9]			= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F10]		= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F11]		= cli_handler_key_fn,
	[CLI_KEY_ESC_BASE + CLI_KEY_F12]		= cli_handler_key_fn,
};

static int cli_esc_seq_detect(cli_info_t *info, int key)
{
	int count;

	cli_esc_map_each_set(&info->esc_map, idx) {
		const char *seq = esc_seq_map[idx];
		if (!seq || (seq[info->esc_char_pos] != (char)key))
			cli_esc_map_clear_bit(&info->esc_map, idx);
	}

	info->esc_char_pos++;

	if (!(count = cli_esc_map_count_set(&info->esc_map))) {		/* nothing found, fail */
		info->flag &= ~CLI_ESC_MODE;
		return -CI_E_INVALID;
	} else if (count == 1) {	/* the only candidate, check */
		int idx = cli_esc_map_first_set(&info->esc_map);
		const char *seq = esc_seq_map[idx];
		if (!seq[info->esc_char_pos]) {		/* end of string */
			info->flag &= ~CLI_ESC_MODE;			/* exit escape mode */
			return idx;
		}
	} 

	/* more than one candidates here, waiting next char */
	return -CI_E_AGAIN;
}

void cli_char_handler(pal_sock_srv_info_t *sock_info, char ch)
{
	int key = (unsigned char)ch;
	cli_info_t *info = (cli_info_t *)sock_info->ctx;

	ci_assert(cli_key_handler[key]);
//	ci_printf("%#02X, %c\n", ch, ch);
	
	if (!(info->flag & CLI_ESC_MODE)) 
		cli_key_handler[key](info, key);
	else if ((key = cli_esc_seq_detect(info, key)) >= 0) 
		cli_key_handler[CLI_KEY_ESC_BASE + key](info, key);
#ifdef CI_DEBUG	
	else if (key == -CI_E_INVALID) {	/* invlaid sequence detected */
		char msg[] = "\n*** INVALID ESCAPE SEQUENCE DETECTED ***\n";
		pal_sock_send_str(sock_info, msg);
	}
#endif	
}

int cli_key_init(cli_info_t *info)
{
	ci_loop(key, CLI_KEY_NR)
		if (!cli_key_handler[key]) 
			cli_key_handler[key] = ci_char_is_printable(key) ? cli_handler_key_printable : cli_handler_key_unprintable;

	return 0;
}

void cli_show_prompt(cli_info_t *info, int newline)
{
	cli_reset_status(info);
	cli_reset_line_buf(info);
	pal_sock_send_str(&info->sock_info, newline ? "\n> " : CLI_PROMPT);
}

