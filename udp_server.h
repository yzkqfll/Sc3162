#ifndef __UDP_SERVER_H_
#define __UDP_SERVER_H_

struct  udp_server {
	char name[10];

	int fd_listen;
	char server_ip[16];
	int listen_port;

	char peer_ip[16];
	int peer_port;

	int rx_buf_size;
	char *rx_buf;
	int rx_len;

	int tx_buf_size;
	char *tx_buf;
	int tx_len;

	int init_ok;
};

struct udp_server *udp_server_init(char *name, char server_ip[16], int listen_port);
int udp_server_running(struct udp_server *us);
int udp_server_recv_data(struct udp_server *us, char **buf, int *len, char **peer_ip, int *peer_port);
int udp_server_send_data(struct udp_server *us, char *peer_ip, int peer_port, char *data, int len);

#endif
