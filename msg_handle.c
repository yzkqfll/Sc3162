#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"
#include "ir_control.h"
#include "msg_handle.h"

#define MODULE "[msg dispatch] "

#define MSG_MAGIC 0x12345678

enum {
	MSG_TYPE_SET_SSID = 1,
	MSG_TYPE_QUERY_IP,
	MSG_TYPE_INFRA,
};

struct msg_header {
	unsigned int magic;
	unsigned char msg_type;
	unsigned char msg_subtype;
} __attribute__((packed));

extern int config_net(char *buf, int len, char *ret_buf, int *ret_len);

int msg_dispatch(char *rx_buf, int rx_len, char *ret_buf, int *ret_len)
{
	struct msg_header *h = (struct msg_header *)rx_buf;
	char *packet;
	int packet_len;

	if (h->magic != MSG_MAGIC) {
		printf(MODULE "Error of magic:%x\n", h->magic);

		ENCAP_RET_BUFFER("Error of magic");
		return 0;
	}

	packet = rx_buf + sizeof(struct msg_header);
	packet_len = rx_len - sizeof(struct msg_header);

	switch (h->msg_type) {
		case MSG_TYPE_SET_SSID:
			printf(MODULE "-> Set SSID");
			config_net(packet, packet_len, ret_buf, ret_len);

			break;

		case MSG_TYPE_QUERY_IP:
			printf(MODULE "-> Query IP");
			ENCAP_RET_BUFFER("Query IP: OK");
			break;

		case MSG_TYPE_INFRA:
			ir_msg_handle(h->msg_subtype, packet, packet_len, ret_buf, ret_len);

			break;

		default:
			printf(MODULE "-> Unknown CMD");
			ENCAP_RET_BUFFER("Unknown CMD");

			break;
	}

	return 0;
}
