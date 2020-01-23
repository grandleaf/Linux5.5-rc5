/*
 * Copyright (c) 2016-2018, Hua Ye and/or its affiliates. All rights reserved.
 *
 * pal_shared.c				PAL shared utilities
 *                                                          hua.ye@Hua Ye.com
 */

#include "ci.h"

pal_sock_srv_info_t *g_pal_sock_srv_info;


#define pal_sock_srv_err(info, err)			server_error(info, err, __FILE__, __FUNCTION__, __LINE__)
#define pal_sock_clt_err(info, err)			client_error(info, err, __FILE__, __FUNCTION__, __LINE__)

#define pal_sock_call_char_handler(info, ch)		\
	do {	\
		if ((info)->char_handler)		\
			(info)->char_handler(info, ch);		\
		else		\
			pal_printfln("socket: no char handler!  ch=%#02X", ch);	\
	} while (0)

#define pal_sock_evt(info, evt)		\
	do {	\
		if ((info)->event_handler)		\
			(info)->event_handler(info, evt);	\
	} while (0)

static void server_error(pal_sock_srv_info_t *info, int error, const char *file, const char *func, int line)
{
	info->file = file;
	info->func = func;
	info->line = line;
	info->err_code = error;

	if (info->event_handler)
		info->event_handler(info, PAL_SOCK_EVT_SERVER_ERROR);
	else
		pal_err_printfln("socket server_error!  file:%s, func:%s, line:%d, err:%d", file, func, line, error);

	pal_sock_srv_destroy(info);
}

static void client_error(pal_sock_srv_info_t *info, int error, const char *file, const char *func, int line)
{
	int client_socket = (int)info->client_socket;
	pthread_t client_thread;

	info->file = file;
	info->func = func;
	info->line = line;
	info->err_code = error;

	if (client_socket) {	/* cannot be 0, stdin */
		close(client_socket);
		info->client_socket = 0;
	}

	if (!error || (error == -EPERM)) {
		pal_sock_evt(info, PAL_SOCK_EVT_CLIENT_DISCONNECTED);
		pal_imp_printfln("socket client disconnected, fd=%d", client_socket);
	} else {
		pal_sock_evt(info, PAL_SOCK_EVT_CLIENT_ERROR);
		pal_err_printfln("socket client error!  file:%s, func:%s, line:%d, err:%d", file, func, line, error);
	}

	client_thread = *(pthread_t *)info->client;
	info->client = NULL;
	pal_imp_printfln("socket client thread destroyed");
	pthread_cancel(client_thread);
}	

static int is_esc_seq(int sock)		/* detect if it is escape sequence */
{
#define ESC_DETECT_LOOP_CNT			2
	char ch;
	int rv;
	
	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);	

	ci_loop(i, ESC_DETECT_LOOP_CNT) {
		if ((rv = read(sock, &ch, sizeof(ch))) > 0)
			break;
		if (i != ESC_DETECT_LOOP_CNT - 1)
			pal_msleep(1);
	}
	
	fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
	return rv > 0 ? ch : 0;
}

static void *client_thread_entry(void* data) 
{
	char ch;
	int rv;
	char line_mode[] = { 255, 253, 34, 255, 251, 1 };	
	char hk_buf[256];	/* handshake buffer */
	pal_sock_srv_info_t *info = (pal_sock_srv_info_t *)data;
	int client_socket = (int)info->client_socket;

	pal_imp_printfln("socket client connected, client thread created, fd=%d", client_socket);
	pal_sock_send_str(info, line_mode);
//	write(client_socket, line_mode, sizeof(line_mode));
	rv = read(client_socket, hk_buf, sizeof(hk_buf));		/* ignore handling */

	pal_sock_evt(info, PAL_SOCK_EVT_CLIENT_CONNECTED);

	while ((rv = read(client_socket, &ch, sizeof(ch))) > 0) {		/* one character a time */
		pal_sock_call_char_handler(info, ch);
		
		if (ch == CI_ASCII_ESC) {
			if ((ch = is_esc_seq(client_socket)))
				pal_sock_call_char_handler(info, ch);
			else 
				pal_sock_call_char_handler(info, CI_ASCII_ESC);		/* ESC, ESC */
		}
	}
	
	pal_sock_clt_err(info, rv);		/* closed */

	return info;
}	

static void *server_thread_entry(void *data)
{
	int rv, listen_socket, client_socket;
	static pthread_t client_thread;
	struct sockaddr_in server_addr, client_addr;
	pal_sock_srv_info_t *info = (pal_sock_srv_info_t *)data;

	pal_imp_printfln("socket server thread created");
	
	if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		pal_sock_srv_err(info, listen_socket);
        return info;
	}
	info->listen_socket = listen_socket;

	ci_obj_zero(&server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(info->port_nr);

	if ((rv = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int))) < 0) {
		pal_sock_srv_err(info, rv);
        return info;
	}

	if ((rv = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
		pal_sock_srv_err(info, rv);
        return info;
	}

	pal_sock_evt(info, PAL_SOCK_EVT_SERVER_STARTED);

__do_listen:
	listen(listen_socket, 2);	/* we only use one, the other one is used to reject new connections */
	client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &(socklen_t){ sizeof(client_addr) });     
	if (client_socket < 0) {
		pal_err_printfln("socket server error on accept, socket=%d", client_socket);
		goto __do_listen;
	}

	if (info->client_socket) {	/* someone already connect to this */
		char reject_msg[] = "Socket busy (someone is using it), connection rejected!";
//		write(client_socket, reject_msg, ci_strlen(reject_msg));
		pal_sock_send_str(info, reject_msg);
		close(client_socket);

		pal_sock_evt(info, PAL_SOCK_EVT_CLIENT_REJECTED);
		pal_imp_printfln("socket client_socket=%d connection rejected (busy)", client_socket);

		goto __do_listen;
	}	

	/* spawn client thread */
	info->client_socket = client_socket;
	rv = pthread_create(&client_thread, NULL, client_thread_entry, info); 
	ci_assert(!rv);
	info->client = &client_thread;	
	
	goto __do_listen;

	return info;	/* dummy */
}

int pal_sock_srv_create(pal_sock_srv_info_t *info) 
{
	ci_unused int rv;
	static pthread_t server_thread;

	g_pal_sock_srv_info = info;

	if (!info->max_str_len)
		info->max_str_len = CI_PR_MEM_BUF_SIZE;

	rv = pthread_create(&server_thread, NULL, server_thread_entry, info); 
	ci_assert(!rv);
	info->server = &server_thread;
		
    return 0;
}

int pal_sock_srv_destroy(pal_sock_srv_info_t *info) 
{
	int listen_socket = (int)info->listen_socket;
	int client_socket = (int)info->client_socket;

	if (listen_socket && (listen_socket != -EPERM)) 
		close(listen_socket);
	
	if (client_socket && (client_socket != -EPERM)) {
		pal_warn_printfln("skip closing client_socket:%d to avoid thread hang", client_socket);
//		close(client_socket);
	}

	if (info->client) {
		pthread_cancel(*(pthread_t *)info->client);
		pal_imp_printfln("socket client thread destroyed");
	}
	
	if (info->server) {
		pthread_cancel(*(pthread_t *)info->server);
		pal_imp_printfln("socket server thread destroyed");
	}

	info->listen_socket = info->client_socket = 0;
	info->server = info->client = NULL;
	g_pal_sock_srv_info = NULL;

	return 0;
}

int pal_sock_send_char(pal_sock_srv_info_t *info, char ch)
{
	int rv, client_socket = (int)info->client_socket;

	if (!client_socket)	/* socket disconnected already */
		return -1;

	if ((rv = write(client_socket, &ch, 1)) <= 0) {
		pal_sock_clt_err(info, rv);
		return -1;
	}

	return 0;
}

static int __pal_sock_send_str(pal_sock_srv_info_t *info, char *str, int len)
{
	int rv, client_socket = (int)info->client_socket;

	assert(len >= 0);
	if (!client_socket)	/* socket disconnected already */
		return -1;

	while (len > 0) {
		if ((rv = write(client_socket, str, len)) <= 0) {
			pal_sock_clt_err(info, rv);
			return -1;
		}

		str += rv;
		len -= rv;
	}

	return 0;
}

int pal_sock_send_str(pal_sock_srv_info_t *info, char *str)
{
	char *ptr;
	int rv, send_len, len;

	if (!info) {
		info = g_pal_sock_srv_info;
		if (!info)
			return -1;
	}

	if (!(len = (int)strnlen(str, info->max_str_len)))
		return 0;	

	while (len >= 0) {
		if ((ptr = ci_strstr(str, "\n"))) {
			send_len = (int)(ptr - str);

			if ((rv = __pal_sock_send_str(info, str, send_len)) < 0)
				return rv;
			if ((rv = __pal_sock_send_str(info, "\n\r", 2)) < 0)
				return rv;

			str += send_len + 1;
			len -= send_len + 1;
		} else 
			return __pal_sock_send_str(info, str, len);
	}

	return 0;		/* dummy */
}

void pal_sock_clt_close(pal_sock_srv_info_t *info)
{
	if (info->client_socket) {
		close(info->client_socket);
		info->client_socket = 0;
	}
}

