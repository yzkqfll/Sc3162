#ifndef __MSG_HANDLE_H__
#define __MSG_HANDLE_H__



int msg_dispatch(char *rx_buf, int rx_len, char *ret_buf, int *ret_len);

#endif
