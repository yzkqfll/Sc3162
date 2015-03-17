#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"

#define PREFIX_CNTL "$$$CNTL$$$"
#define PREFIX_IR   "$$$IR$$$"

int msg_dispatch(char *rx_buf, int rx_len, char *ret_buf, int *ret_len)
{
	if (strncmp(rx_buf, PREFIX_CNTL, strlen(PREFIX_CNTL)) == 0) {

		return 0;

	} else if (strncmp(rx_buf, PREFIX_IR, strlen(PREFIX_IR)) == 0) {

		//ir_msg_handle(rx_buf, rx_len, ret_buf, ret_len);
		return 0;
	}

	return -1;
}
