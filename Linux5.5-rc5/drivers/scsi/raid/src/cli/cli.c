/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * cli.c				Command Line Interface via socket
 *                                                          hua.ye@Hua Ye.com
 */
#include "cli.h"


static void cli_send_welcome(pal_sock_srv_info_t *sock_info)
{
	char msg[200];
	ci_mem_range_ex_def(mem, msg, msg + ci_sizeof(msg));
	ci_mem_range_ex_t *range = &ci_printf_info->range_mem_buf;

	__ci_printf_line_lock(); 	
	pal_sock_send_str(sock_info, (char *)range->start);
	ci_printf_info->flag |= CI_PRF_CONSOLE;	/* do not print to buf now */

	if (range->curr >= range->end) {
		ci_mem_range_ex_reinit(&mem);
		ci_mem_printf(&mem, "\n\n****** WARNING: MESSAGE MIGHT BE TRUNCATED ******\n\n");
		pal_sock_send_str(sock_info, msg);
	}
	__ci_printf_line_unlock();

	ci_mem_printf(&mem, "\n\nRAID Command Line Interface [ %s, %s, %s Build %s ]\n", __DATE__, __TIME__, __RELEASE_DEBUG__, ci_info.build_nr);		
	ci_mem_printf(&mem, "Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.\n");		
	pal_sock_send_str(sock_info, msg);
}

void cli_event_handler(pal_sock_srv_info_t *sock_info, int event)
{
	cli_info_t *info = cli_info_from_sock_info(sock_info);

	switch (event) {
		case PAL_SOCK_EVT_SERVER_STARTED: 
			ci_assert(info->json_vect);
			ci_mod_start_done(info->mod, info->json_vect);
			info->json_vect = NULL;
			break;
		case PAL_SOCK_EVT_CLIENT_CONNECTED: 
			cli_send_welcome(sock_info);
			pal_sock_send_str(sock_info, "\n");
			cli_show_prompt(info, 1);
			break;
		case PAL_SOCK_EVT_CLIENT_DISCONNECTED:
			__ci_printf_line_lock(); 	
			ci_mem_range_ex_reinit(&ci_printf_info->range_mem_buf);
			ci_printf_info->flag &= ~CI_PRF_CONSOLE;
			__ci_printf_line_unlock();
			break;
		default:
			break;
	}
}


