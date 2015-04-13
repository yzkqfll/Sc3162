#include <stdio.h>
#include "ctype.h"
#include <string.h>

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "common.h"

#include "msg_handle.h"
#include "ir_dec.h"
#include "ir_enc.h"

#define MODULE "[ir control] "

enum {
    IMT_CONNECT = 1,
    IMT_ENABLE_INPUT_CC,
    IMT_DECODE,
    IMT_DISABLE_INPUT_CC,
    IMT_SEND_NEC,
};

#define DEBUG

#if defined(DEBUG)
#define ir_printf   printf
#else
#define ir_printf
#endif

static IR_DecResult idr;

char *ir_get_type_name(uint8_t type)
{
    switch(type) {
        case IR_NEC:
            return "NEC";
        default:
            return "Unknow";
    }
}

void ir_msg_handle_decode(char *ret_buf, int *ret_len)
{
    char val[8] = {0};
    int res = 0;

    res = ir_decode(&idr);
    if(res == 1) {

        printf(MODULE "Decoded succeed : protocol=%s, data=0x%x, length=%d \r\n",
               ir_get_type_name(idr.type), idr.value, idr.bits);

        ir_raw_data_print(&idr);
        ir_icc_enable_set(0);
        ir_rx_next();

        sprintf(val, "%x", idr.value);
        strcpy(ret_buf, "IRDecode: ");
        strcat(ret_buf, val);
        *ret_len = strlen(ret_buf);

    } else {

        if(res == 0) { //unknow
            ir_raw_data_print(&idr);
        }
        ir_rx_next();
        ENCAP_RET_BUFFER("IRDecode: ERR");
    }
}

void ir_msg_handle_send(int code_type, char *rx_buf, int rx_len, char *ret_buf, int *ret_len)
{
    unsigned int data = 0;
    unsigned int nbits = (unsigned int)(rx_len * 4);
    char *sval = NULL;

    sval = (char *)malloc(rx_len+1);
    if(sval == NULL) {
	ir_printf(MODULE "alloc memory failed\r\n");
	ENCAP_RET_BUFFER("IRSendNEC: NOMEM");
	return;
    }

    data = strtoul(strncpy((char *)sval, rx_buf, rx_len), NULL, 16);
    free(sval);

    ir_send(code_type, data, nbits);
    ENCAP_RET_BUFFER("IRSendNEC: OK");
}


int ir_msg_handle(unsigned char ir_msg_type, char *rx_buf, int rx_len, char *ret_buf, int *ret_len)
{
    switch (ir_msg_type) {
        case IMT_CONNECT:
            ir_printf(MODULE "-> IRConnect\n");
            ENCAP_RET_BUFFER("IRConnect: OK");
            break;

        case IMT_ENABLE_INPUT_CC:
            ir_printf(MODULE "-> Start IR learning\n");
            ir_icc_enable_set(1);
            ENCAP_RET_BUFFER("IREnableICC: OK");
            break;

        case IMT_DISABLE_INPUT_CC:
            ir_printf(MODULE "-> Stop IR learning\n");
            ir_icc_enable_set(0);
            ENCAP_RET_BUFFER("IRDisableICC: OK");
            break;

        case IMT_DECODE:
            ir_printf(MODULE "-> Receive IR signal\n");
            ir_msg_handle_decode(ret_buf, ret_len) ;
            break;

        case IMT_SEND_NEC:
            ir_printf(MODULE "-> Send IR signal: 0x%s, len%d\n", rx_buf,rx_len);
            ir_msg_handle_send(IR_NEC, rx_buf, rx_len, ret_buf, ret_len);
            break;

        default:
            ir_printf(MODULE "-> Unknown IR msg type\n");
            break;
    }
    return 0;
}

int ir_init(void)
{
    ir_rx_config();

    ir_tx_config();

    return 0;
}
