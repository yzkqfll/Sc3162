#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"
#include "ap.h"
#include "sta.h"
#include "ir_control.h"

#define MODULE "[sys cntl] "

int init_sys(void)
{
	if (ap_init()) {
		printf(MODULE "ap init failed\r\n");
		return -1;
	}

	if (sta_init()) {
		printf(MODULE "sta init failed\r\n");
		return -1;
	}

	ir_init();

	return 0;
}

/*
 * Profile 1
 *     4 independent state machine
 * */
int sys_control(void)
{
	ap_state_machine();

	sta_state_machine();

	return 0;
}
