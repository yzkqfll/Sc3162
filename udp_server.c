#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"

#include "udp_server.h"

static void us_close(struct  udp_server *us)
{
	close(us->fd_listen);
	us->fd_listen = 0;
}

static int us_recv_data(struct udp_server *us)
{
	int cnt;
	struct sockaddr_t addr;
	int addr_len;

	cnt = recvfrom(us->fd_listen, us->rx_buf, us->rx_buf_size, 0, &addr, &addr_len);
	if (!cnt) {
		printf("%s recvfrom 0 bytes, exit\r\n", us->name);
		return -1;
	}

	us->rx_len = cnt;
	inet_ntoa(us->peer_ip, addr.s_ip);
	us->peer_port = addr.s_port;

#ifdef DEBUG
	us->rx_buf[cnt] = '\0';
	printf("%s [%s:%d] get msg from [%s:%d] <%s>, cnt %d\r\n", us->name,
			us->server_ip, us->listen_port,
			us->peer_ip, us->peer_port, us->rx_buf, cnt);
#endif

	return 0;
}

static int us_send_data(struct udp_server *us)
{
	struct sockaddr_t addr;

	if (us->tx_len) {
		addr.s_ip = inet_addr(us->peer_ip);
		addr.s_port = us->peer_port;

#ifdef DEBUG
		if (us->tx_len < us->tx_buf_size) {
			us->tx_buf[us->tx_len] = '\0';
			printf("%s [%s:%d] send msg to [%s:%d] <%s>, cnt %d\r\n", us->name,
				us->server_ip, us->listen_port,
				us->peer_ip, us->peer_port, us->tx_buf, us->tx_len);
		}
#endif

		sendto(us->fd_listen, us->tx_buf, us->tx_len, 0, &addr, sizeof(addr));
		us->tx_len = 0;
	}

	return 0;
}

int udp_server_recv_data(struct udp_server *us, char **buf, int *len, char **peer_ip, int *peer_port)
{
	udp_server_running(us);

	if (us->rx_len) {
		*buf = us->rx_buf;
		*len = us->rx_len;
		if (peer_ip) {
			*peer_ip = us->peer_ip;
			*peer_port = us->peer_port;
		}

		/* Clear flag */
		us->rx_len = 0;

		return 1;
	}

	return 0;
}

int udp_server_send_data(struct udp_server *us, char *peer_ip, int peer_port, char *data, int len)
{
	if (peer_ip) {
		strcpy(us->peer_ip, peer_ip);
		us->peer_port = peer_port;

		memcpy(us->tx_buf, data, len);
		us->tx_len = len;
	}

	return us_send_data(us);
}

int udp_server_running(struct udp_server *us)
{
	fd_set readfds, writefds, exceptfds;
	struct timeval_t t = {0, 500};
	int err = 0;
	int ret;

	if (!us->init_ok)
		return -1;

	if (!us->fd_listen)
		return -1;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	/*
	 * Only receive packet, do not send packet.
	 * */
	FD_SET(us->fd_listen, &readfds);
	//FD_SET(us->fd_listen, &writefds);
	//FD_SET(us->fd_listen, &exceptfds);

	ret = select(1, &readfds, &writefds, &exceptfds, &t);
	if (ret < 0) {
		us_close(us);
		return -1;
	} else if (ret == 0) {
		//printf("%s udp: time out\r\n", us->name);
		return 0;
	}

	if (FD_ISSET(us->fd_listen, &readfds)) {
		if (us_recv_data(us))
			err = 1;
	}
	if (FD_ISSET(us->fd_listen, &writefds)) {
		printf("%s udp server: us->fd_listen %d in writefds ?\r\n", us->name,
						us->fd_listen);
		/*
		 * Do not send packet here. Use udp_server_send_data instead
		 *
		if (us_send_data(us))
			err = 1;
		*/
	}
	if (FD_ISSET(us->fd_listen, &exceptfds)) {
		printf("%s udp server: us->fd_listen %d in exceptfds ?\r\n", us->name,
				us->fd_listen);
		err = 1;
	}

	if (err)
		us_close(us);

	return err;
}

static int us_open(struct udp_server *us)
{
	int fd;
	int val;
	struct sockaddr_t addr;

	fd = socket(AF_INET, SOCK_DGRM, IPPROTO_UDP);
	if (!fd) {
		printf("%s fail to create server socket\r\n", us->name);
		return -1;
	}

	val = 1;
	setsockopt(fd, 0, SO_REUSEADDR, &val, sizeof(val));
	setsockopt(fd, 0, SO_BROADCAST, &val, sizeof(val));
	val = 0;
	/*
	 * use select() instead of block mode like below:
	 * setsockopt(fd, 0, SO_BLOCKMODE, &val, sizeof(val));
	 * */

	addr.s_ip = inet_addr(us->server_ip);
	addr.s_port = us->listen_port;
	if (bind(fd, &addr, sizeof(addr))) {
		printf("%s fail to bind to %s:%d\n", us->name,
				us->server_ip, us->listen_port);
		close(fd);
		return -1;
	}
	printf("%s create udp server %s:%d, fd %d\r\n", us->name, us->server_ip, us->listen_port, fd);

	us->fd_listen = fd;

	return 0;
}

struct udp_server *udp_server_init(char *name, char server_ip[16], int listen_port)
{
	struct udp_server *us;

	us = malloc(sizeof(struct udp_server));
	if (!us)
		return NULL;

	memset(us, 0, sizeof(*us));

	strcpy(us->name, name);
	us->tx_buf_size = TX_BUF_SIZE;
	us->rx_buf_size = RX_BUF_SIZE;

	us->rx_buf = malloc(us->rx_buf_size);
	if (!us->rx_buf) {
		printf("%s fail to allocate rx buffer\r\n", us->name);
		goto err_rx_buf;
	}
	us->tx_buf = malloc(us->tx_buf_size);
	if (!us->tx_buf) {
		printf("%s fail to allocate tx buffer\r\n", us->name);
		goto err_tx_buf;
	}

	strcpy(us->server_ip, server_ip);
	us->listen_port = listen_port;

	if (us_open(us)) {
		goto err_open;
	}

	us->init_ok = 1;

	return us;

err_open:
	free(us->tx_buf);
err_tx_buf:
	free(us->rx_buf);
err_rx_buf:
	free(us);
	return NULL;
}

