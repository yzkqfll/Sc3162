#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"
#include "sta.h"
#include "msg_handle.h"

#define MODULE "[net config] "

#define MSG_PREFIX "###"
#define MSG_DELIMITER ':'
#define MSG_END '$'

int config_net(char *buf, int len, char *ret_buf, int *ret_len)
{
	char *ssid, *passwd, *separator, *end;

	if (strncmp(buf, MSG_PREFIX, strlen(MSG_PREFIX))) {
		printf(MODULE "prefix should be %s\r\n", MSG_PREFIX);
		goto err;
	}

	ssid = buf + strlen(MSG_PREFIX);
	separator = strchr(ssid, MSG_DELIMITER);
	if (!separator) {
		printf(MODULE "format prefix error, shoule be ###ssid:passwd$\n");
		goto err;
	}
	*separator = '\0';
	passwd = separator + 1;

	end = strchr(passwd, MSG_END);
	if (!end) {
		printf(MODULE "format end error, shoule be ###ssid:passwd$\n");
		goto err;
	}
	*end = '\0';

	sta_connect_to_ap(ssid, passwd);

	ENCAP_RET_BUFFER("OK");

	return 0;

err:
	ENCAP_RET_BUFFER("Format error: ###ssid:passwd$");
	return -1;
}
