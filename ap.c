#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"
#include "ap.h"
#include "sta.h"

#include "udp_server.h"

#define MODULE "[ap] "

#define AP_SSID "smarthome"
#define AP_PASSWD ""
#define AP_IP "10.0.0.1"
#define DHCP_START_IP "10.0.0.100"
#define DHCP_END_IP "10.0.0.200"

#define LISTEN_PORT 9527

#define MSG_PREFIX "###"
#define MSG_DELIMITER ':'
#define MSG_END '$'

struct __ap {
	char ssid[SSID_LEN + 1];
	char passwd[PASSWD_LEN + 1];

	char ip[16];
	int dhcp;
	char dhcp_start[16];
	char dhcp_end[16];

	int is_up;
	int is_opening;
};

static struct __ap ap;

static struct udp_server *us = NULL;

int ap_is_up(void)
{
	struct __ap *a = &ap;

	return a->is_up;
}

char *ap_get_ip(void)
{
	struct __ap *a = &ap;

	return a->ip;
}

/*
 * called by callback function
 * */
void ap_change_status(int is_up, net_para_st *np)
{
	struct __ap *a = &ap;

	a->is_up = is_up;
	a->is_opening = 0;

	if (a->is_up) {
		strcpy(a->ip, np->ip);

		printf(MODULE "Sort AP is up:\r\n");
		printf(MODULE "    ip: %s\r\n", np->ip);
		printf(MODULE "    netmask: %s\r\n", np->mask);
		printf(MODULE "    mac: %s\r\n", np->mac);
	} else {
		printf(MODULE "Sort AP is down\r\n");
	}

}

int ap_open(void)
{
	struct __ap *a = &ap;
	int ret;
	network_InitTypeDef_st ap_net_config;

	if (a->is_opening)
		return 0;

	memset(&ap_net_config, 0x0, sizeof(network_InitTypeDef_st));

	ap_net_config.wifi_mode = Soft_AP;

	strcpy((char*)ap_net_config.wifi_ssid, a->ssid);
	strcpy((char*)ap_net_config.wifi_key, a->passwd);

	strcpy((char*)ap_net_config.local_ip_addr, a->ip);
	strcpy((char*)ap_net_config.net_mask, "255.255.255.0");

	if (a->dhcp) {
		ap_net_config.dhcpMode = DHCP_Server;
		strcpy((char*)ap_net_config.address_pool_start, a->dhcp_start);
		strcpy((char*)ap_net_config.address_pool_end, a->dhcp_end);
	}

	printf(MODULE "start soft AP\r\n");
	printf(MODULE "    ssid: %s\r\n", a->ssid);
#ifdef DEBUG
	printf(MODULE "    passwd: %s\r\n", a->passwd);
#endif
	printf(MODULE "    ip: %s\r\n", a->ip);
	printf(MODULE "    dhcp: %s\r\n", a->dhcp ? "enabled" : "disabled");
	if (a->dhcp) {
		printf(MODULE "    dhcp start ip: %s\r\n", a->dhcp_start);
		printf(MODULE "    dhcp end ip: %s\r\n", a->dhcp_end);
	}

	ret = StartNetwork(&ap_net_config);

	/*
	 * WifiStatusHandler() is the callback
	 * */

	a->is_opening = 1;

	return ret;
}

int ap_close(void)
{
	// TODO

	return 0;
}

int ap_state_machine(void)
{
	int ret = 0;
	char *buf;
	int len;
	char *peer_ip;
	int peer_port;
	char *ssid, *passwd, *separator, *end;

	if (!ap_is_up()) {
		ret = ap_open();
		if (ret) {
			printf(MODULE "start ap failed(ret %d)\r\n", ret);
			return ret;
		}

		if (!us) {
			us = udp_server_init("[ap us] ", AP_IP, LISTEN_PORT);
			if (!us) {
				printf(MODULE "init udp server failed\r\n");
				return -1;
			}
		}
	}

	if (udp_server_recv_data(us, &buf, &len, &peer_ip, &peer_port)) {
		if (strncmp(buf, MSG_PREFIX, strlen(MSG_PREFIX))) {
			printf(MODULE "prefix should be %s\r\n", MSG_PREFIX);
			return 0;
		}

		ssid = buf + strlen(MSG_PREFIX);
		separator = strchr(ssid, MSG_DELIMITER);
		if (!separator) {
			printf(MODULE "format error, shoule be ###ssid:passwd$\n");
			return 0;
		}
		*separator = '\0';
		passwd = separator + 1;

		end = strchr(passwd, MSG_END);
		if (!end) {
			printf(MODULE "format error, shoule be ssid:passwd$\n");
			return 0;
		}
		*end = '\0';

		sta_connect_to_ap(ssid, passwd);

		udp_server_send_data(us, peer_ip, peer_port, "OK", strlen("OK"));
	}

	return ret;
}

int ap_init(void)
{
	struct __ap *a = &ap;

	memset(a, 0, sizeof(*a));

	strcpy(a->ssid, AP_SSID);
	strcpy(a->passwd, AP_PASSWD);

	strcpy(a->ip, AP_IP);
	a->dhcp = 1;
	if (a->dhcp) {
		strcpy(a->dhcp_start, DHCP_START_IP);
		strcpy(a->dhcp_end, DHCP_END_IP);
	}

	return 0;
}
