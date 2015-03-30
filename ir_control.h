#ifndef __IR_CONTROL_H__
#define __IR_CONTROL_H__

int ir_msg_handle(unsigned char msg_type, char *rx_buf, int rx_len, char *ret_buf, int *ret_len);
int ir_init(void);

#endif
