#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"

#include "tcp_server.h"

static int ts_new_client(struct tcp_server *ts)
{
	int i;
	int fd_new;
	struct sockaddr_t addr;
	int len;
	struct tcp_client *c;

	fd_new = accept(ts->fd_listen, &addr, &len);
	if (fd_new <= 0) {
		printf("%s fail to accept\r\n", ts->name);
		return 0;
	}

	for (i = 0; i < ts->max_client; i++) {
		c = &ts->clients[i];
		if (!c->fd)
			break;
	}
	if (i == ts->max_client) {
		printf("%s clients[].fd is full, say goodbye !\r\n", ts->name);
		send(fd_new, "goodbye", sizeof("goodbye"), 0);
		close(fd_new);
		return 0;
	}

	ts->online_client++;

	c->fd = fd_new;
	inet_ntoa(c->ip, addr.s_ip);
	c->port = addr.s_port;

	printf("%s tcp_client %s:%d connected, fd %d\r\n", ts->name,
			c->ip, c->port, c->fd);

	return 0;
}

static int ts_recv_data(struct tcp_client *c)
{
	int cnt;
	struct tcp_server *ts = c->ts;

	cnt = recv(c->fd, c->rx_buf, c->rx_buf_size, 0);
	if (cnt < 0) {
		printf("%s fail to recv on tcp_client %s:%d, fd %d\r\n", ts->name,
				c->ip, c->port, c->fd);
		return -1;
	} else if (cnt == 0) {
		printf("%s peer %s:%d closed\n", ts->name, c->ip, c->port);
		return -1;
	}
	c->rx_len = cnt;

#ifdef DEBUG_SOCKET
	if (cnt < c->rx_buf_size) {
		c->rx_buf[cnt] = '\0';  // debug only, for string msg
		printf("%s [%s:%d] get msg from [%s:%d] fd %d: <%s>, cnt %d\r\n", ts->name,
					ts->server_ip, ts->listen_port,
					c->ip, c->port, c->fd, c->rx_buf, cnt);
	}
#endif

	return 0;
}

static int ts_send_data(struct tcp_client *c)
{
	if (c->tx_len > 0) {
#ifdef DEBUG_SOCKET
		struct tcp_server *ts = c->ts;
		printf("%s [%s:%d] fd %d send msg to [%s:%d]: <%s>, cnt %d\r\n", ts->name,
				ts->server_ip, ts->listen_port,
				c->fd, c->ip, c->port, c->tx_buf, c->tx_len);
#endif
		send(c->fd, c->tx_buf, c->tx_len, 0);
		c->tx_len = 0;
	}

	return 0;
}

struct tcp_client *tcp_server_recv_data(struct tcp_server *ts, char **buf, int *len, char **peer_ip, int *peer_port)
{
	int i;

	tcp_server_running(ts);

	for (i = 0 ; i < ts->max_client; i++) {
		struct tcp_client *c = &ts->clients[i];

		if (c->rx_len) {
			*buf = c->rx_buf;
			*len = c->rx_len;
			if (peer_ip) {
				*peer_ip = c->ip;
				*peer_port = c->port;
			}
			c->rx_len = 0;
			return c;
		}
	}

	return NULL;
}

int tcp_server_send_data(struct tcp_server *us, struct tcp_client *c, char *data, int len)
{
	memcpy(c->tx_buf, data, len);
	c->tx_len = len;

	return ts_send_data(c);
}


static void ts_close_client(struct tcp_client *c)
{
	struct tcp_server *ts = c->ts;

	printf("%s close from tcp_client %s:%d, fd %d\r\n", ts->name,
			c->ip, c->port, c->fd);

	close(c->fd);
	c->fd = 0;
	ts->online_client--;

	printf("%s %d clients online\r\n", ts->name, ts->online_client);
}

static void ts_close(struct tcp_server *ts)
{
	int i;
	struct tcp_client *c;

	printf("%s close tcp server %s:%d, fd %d\r\n", ts->name,
			ts->server_ip, ts->listen_port, ts->fd_listen);

	for (i = 0; i < ts->max_client; i++) {
		c = &ts->clients[i];
		if (c->fd) {
			ts_close_client(c);
		}
	}

	close(ts->fd_listen);
	ts->fd_listen = 0;
}

static int ts_open(struct tcp_server *ts)
{
	int fd = 0;
	struct sockaddr_t addr;
	int yes = 1;
	int rx_buf_size = RX_BUF_SIZE;
	int tx_buf_size = TX_BUF_SIZE;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*
	 * Linux API returns -1 if failed,
	 * but mxchip returns 0 when failed
	 * */
	if (fd <= 0) {
		printf("%s fail to create server socket\r\n", ts->name);
		return -1;
	}

	setsockopt(fd, 0, SO_REUSEADDR, &yes, sizeof(yes));
	if (setsockopt(fd, 0, SO_RDBUFLEN, &rx_buf_size, sizeof(rx_buf_size))) {
		printf("%s fail to setsockopt(SO_RDBUFLEN)\r\n", ts->name);
		goto err;
	}
	if (setsockopt(fd, 0, SO_WRBUFLEN, &tx_buf_size, sizeof(tx_buf_size))) {
		printf("%s fail to setsockopt(SO_WRBUFLEN)\r\n", ts->name);
		goto err;
	}

	/* Bind to local ip/port */
	addr.s_ip = inet_addr(ts->server_ip);
	addr.s_port = ts->listen_port;
	if (bind(fd, &addr, sizeof(addr))) {
		printf("%s fail to bind to %s:%d\n", ts->name,
				ts->server_ip, ts->listen_port);
		goto err;
	}

	if (listen(fd, 0)) {
		printf("%s fail to listen to %s:%d\n", ts->name,
				ts->server_ip, ts->listen_port);
		goto err;
	}

	printf("%s open tcp server on %s:%d, fd %d\r\n", ts->name,
				ts->server_ip, ts->listen_port, fd);
	ts->fd_listen = fd;

	return 0;

err:
	close(fd);
	return -1;
}


int tcp_server_running(struct tcp_server *ts)
{
	int i;
	int ret;
	fd_set readfds, writefds, exceptfds;
	struct timeval_t t = {0, 500};
	struct tcp_client *c;
	int err = 0;

	if (!ts->init_ok)
		return 0;

	if (!ts->fd_listen)
		return 0;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FD_SET(ts->fd_listen, &readfds);
	//FD_SET(ts->fd_listen, &writefds);
	//FD_SET(ts->fd_listen, &exceptfds);
	for (i = 0; i < ts->max_client; i++) {
		c = &ts->clients[i];
		if (c->fd) {
			FD_SET(c->fd, &readfds);
			//FD_SET(c->fd, &writefds);
			//FD_SET(c->fd, &exceptfds);
		}
	}

	/* 1 ?? */
	ret = select(1, &readfds, &writefds, &exceptfds, &t);
	if (ret < 0) {
		ts_close(ts);
		return -1;
	} else if (ret == 0) {
		//printf("%s time out\r\n");
		return 0;
	}

	if (FD_ISSET(ts->fd_listen, &readfds)) {
		if (ts_new_client(ts))
			err = 1;
	}
	if (FD_ISSET(ts->fd_listen, &writefds))
		printf("%s ts->fd_listen %d in writefds ?\r\n", ts->name,
				ts->fd_listen);
	if (FD_ISSET(ts->fd_listen, &exceptfds)) {
		printf("%s ts->fd_listen %d in exceptfds ?\r\n", ts->name,
				ts->fd_listen);
		err = 1;
	}
	if (err)
		ts_close(ts);

	for (i = 0; i < ts->max_client; i++) {
		c = &ts->clients[i];
		if (!c->fd)
			continue;

		err = 0;
		if (FD_ISSET(c->fd, &readfds)) {
			if (ts_recv_data(c))
				err = 1;
		}
		if (FD_ISSET(c->fd, &writefds)) {
			printf("%s tcp_client %s:%d, fd %d in writefds ??\r\n", ts->name,
					c->ip, c->port, c->fd);
			/*
			 *  Do not send packet here.
			if (ts_send_data(c))
				err = 1;
			*/
		}
		if (FD_ISSET(c->fd, &exceptfds)) {
			printf("%s tcp_client %s:%d, fd %d in exceptfds ??\r\n", ts->name,
								c->ip, c->port, c->fd);
			err = 1;
		}
		if (err)
			ts_close_client(c);
	}

	return 0;
}


struct tcp_server *tcp_server_init(char *name, char server_ip[16], int listen_port)
{
	int i;
	struct tcp_client *c;
	struct tcp_server *ts;

	ts = malloc(sizeof(struct tcp_server));
	if (!ts)
		return NULL;

	strcpy(ts->name, name);
	ts->max_client = MAX_CLIENT;
	for (i = 0; i < ts->max_client; i++) {
		c = &ts->clients[i];
		c->ts = ts;

		c->tx_buf_size = TX_BUF_SIZE;
		c->rx_buf_size = RX_BUF_SIZE;

		c->tx_buf = malloc(c->tx_buf_size); /* tx buf is not used yet */
		if (!c->tx_buf) {
			printf("%s Fail to alloce tx buf(%d bytes)\r\n", ts->name, c->tx_buf_size);
			return NULL;
		}
		c->rx_buf = malloc(c->rx_buf_size);
		if (!c->rx_buf) {
			printf("%s Fail to alloce rx buf(%d bytes)\r\n", ts->name, c->rx_buf_size);
			return NULL;
		}
	}

	strcpy(ts->server_ip, server_ip);
	ts->listen_port = listen_port;

	if (ts_open(ts))
		return NULL;

	ts->init_ok = 1;

	return ts;
}
