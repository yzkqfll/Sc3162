#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"
#include "sta.h"
#include "msg_handle.h"

#include "udp_server.h"
#include "tcp_server.h"

#define MODULE "[sta] "

#define STA_UDP_SRV_PORT 9528
#define STA_TCP_SRV_PORT 9529

struct __sta {
	int try_to_connect;
	int wait_cnt;
	int is_up;
	char ip[16];

	char ssid[SSID_LEN + 1];
	char passwd[PASSWD_LEN + 1];

	char tx_buf[TX_BUF_SIZE];
};

static struct __sta sta;

static struct udp_server *us = NULL;
static struct tcp_server *ts = NULL;

int sta_open(const char *ssid, const char *passwd)
{
	int ret;
	network_InitTypeDef_st station_net_config;

	printf(MODULE "try to connect to home AP: %s\r\n", ssid);
#ifdef DEBUG
	printf(MODULE "passwd %s\r\n", passwd);
#endif

	memset(&station_net_config, 0x0, sizeof(network_InitTypeDef_st));

	station_net_config.wifi_mode = Station;

	strcpy((char*)station_net_config.wifi_ssid, ssid);
	strcpy((char*)station_net_config.wifi_key, passwd);
	station_net_config.dhcpMode = DHCP_Client;
	station_net_config.wifi_retry_interval = 10;

	ret = StartNetwork(&station_net_config);

	/*
	 * callback
	 *     connected_ap_info()
	 *     NetCallback()
	 *     WifiStatusHandler()
	 * */

	return ret;
}

int sta_close(void)
{
	struct __sta *s = &sta;

	printf(MODULE "disconnect from AP\r\n");
	sta_disconnect();

	s->is_up = 0;

	return 0;
}

int sta_connect_to_ap(char *ssid, char *passwd)
{
	struct __sta *s = &sta;

	if (strlen(ssid) >= SSID_LEN || strlen(passwd) > PASSWD_LEN) {
		printf(MODULE "strlen(ssid) %d, strlen(passwd) %d, invalid\r\n",
				strlen(ssid), strlen(passwd));
		return -1;
	}

	strcpy(s->ssid, ssid);
	strcpy(s->passwd, passwd);

	s->try_to_connect = 1;
	s->wait_cnt = 0;

	return 0;
}

int sta_is_up(void)
{
	struct __sta *s = &sta;

	return s->is_up;
}

char *sta_get_ip(void)
{
	struct __sta *s = &sta;

	return s->ip;
}

/*
 * called by callback function
 * */
int sta_change_status(int is_up, net_para_st *np)
{
	struct __sta *s = &sta;

	s->is_up = is_up;
	if (is_up) {
		strcpy(s->ip, np->ip);

		printf(MODULE "status: connected to AP %s\r\n", s->ssid);
		s->try_to_connect = 0;
		s->wait_cnt = 0;
	} else {
		printf(MODULE "status: disconnected from AP %s\r\n", s->ssid);
		s->try_to_connect = 1;
	}

	return 0;
}

int sta_state_machine(void)
{
	struct __sta *s = &sta;
	char *rx_buf;
	int rx_len;
	char *peer_ip;
	int peer_port;
	struct tcp_client *c;
	int tx_len = TX_BUF_SIZE;

	if (s->try_to_connect) {
		if (sta_is_up())
			sta_close();

		if (s->wait_cnt++ % 10 == 0) {
			sta_open(s->ssid, s->passwd);
		}
		sleep(1);
	}

	if (!sta_is_up())
		return -1;

	if (!us) {
		us = udp_server_init("[sta us]", sta_get_ip(), STA_UDP_SRV_PORT);
		if (!us) {
			printf(MODULE "init udp server failed\r\n");
			return -1;
		}
	}

	if (!ts) {
		ts = tcp_server_init("[sta ts]", sta_get_ip(), STA_TCP_SRV_PORT);
		if (!ts) {
			printf(MODULE "init tcp server failed\r\n");
			return -1;
		}
	}

	/* udp server */
	if (udp_server_recv_data(us, &rx_buf, &rx_len, &peer_ip, &peer_port)) {
		msg_dispatch(rx_buf, rx_len, s->tx_buf, &tx_len);

		if (tx_len)
			udp_server_send_data(us, peer_ip, peer_port, s->tx_buf, tx_len);
	}

	/* tcp server */
	while (1) {
		c = tcp_server_recv_data(ts, &rx_buf, &rx_len, &peer_ip, &peer_port);
		if (!c)
			break;

		tcp_server_send_data(ts, c, "OK", strlen("OK"));
	}

	return 0;
}

int sta_init(void)
{
	struct __sta *s = &sta;

	memset(s, 0, sizeof(*s));

	return 0;
}
