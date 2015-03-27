#ifndef __MSG_HANDLE_H__
#define __MSG_HANDLE_H__

#define ENCAP_RET_BUFFER(__str)  \
		memcpy(ret_buf, __str, strlen(__str)); \
		*ret_len = strlen(__str); \


int msg_dispatch(char *rx_buf, int rx_len, char *ret_buf, int *ret_len);

#endif
