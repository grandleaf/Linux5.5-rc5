#pragma once

typedef struct __pal_sock_srv_info_t pal_sock_srv_info_t;
typedef void (*pal_sock_char_handler_t)(pal_sock_srv_info_t *info, char ch);
typedef void (*pal_sock_event_handler_t)(pal_sock_srv_info_t *info, int event);

enum {
	PAL_SOCK_EVT_NONE,
	PAL_SOCK_EVT_SERVER_STARTED,
	PAL_SOCK_EVT_SERVER_ERROR,
	PAL_SOCK_EVT_CLIENT_ERROR,
	PAL_SOCK_EVT_CLIENT_CONNECTED,
	PAL_SOCK_EVT_CLIENT_DISCONNECTED,
	PAL_SOCK_EVT_CLIENT_REJECTED,
	PAL_SOCK_EVT_NR
};

struct __pal_sock_srv_info_t {
	int							 port_nr;
	int							 max_str_len;	/* max allowed str length to be sent, if 0, override with CI_PR_MEM_BUF_SIZE */

	/* error location */
	int							 err_code;
	const char					*file;
	const char					*func;
	int							 line;

	uintptr_t					 listen_socket;
	uintptr_t					 client_socket;

	void						*server;
	void						*client;

	pal_sock_char_handler_t		 char_handler;
	pal_sock_event_handler_t	 event_handler;

	void						*ctx;			/* context */
};

extern pal_sock_srv_info_t *g_pal_sock_srv_info;


int pal_sock_srv_create(pal_sock_srv_info_t *info);
int pal_sock_srv_destroy(pal_sock_srv_info_t *info);
int pal_sock_send_char(pal_sock_srv_info_t *info, char ch);
int pal_sock_send_str(pal_sock_srv_info_t *info, char *str);
void pal_sock_clt_close(pal_sock_srv_info_t *info);



