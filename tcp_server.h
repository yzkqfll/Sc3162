#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#define MAX_CLIENT 1

struct tcp_client {
	struct tcp_server *ts;

	int fd;

	char ip[16]; /* peer ip */
	int port; /* peer port */

	int rx_buf_size;
	char *rx_buf;
	int rx_len;

	int tx_buf_size;
	char *tx_buf;
	int tx_len;
};

struct tcp_server {
	char name[10];

	int fd_listen; /* tcp */
	char server_ip[16];
	int listen_port;

	int max_client;
	int online_client;
	struct tcp_client clients[MAX_CLIENT];

	int init_ok;
};

struct tcp_server *tcp_server_init(char *name, char server_ip[16], int listen_port);
int tcp_server_running(struct tcp_server *ts);
struct tcp_client *tcp_server_recv_data(struct tcp_server *ts, char **buf, int *len, char **peer_ip, int *peer_port);
int tcp_server_send_data(struct tcp_server *us, struct tcp_client *c, char *data, int len);

#endif
