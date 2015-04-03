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

void ir_msg_handle_decode(char *ret_buf, int *ret_len)
{
    char val[8] = {0};
    int res = 0;

    res = ir_decode(&idr);
    if(res == 1) {

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

void ir_msg_handle_send(int code_type, char *rx_buf, int rx_len)
{
    unsigned int data = 0;
    unsigned int nbits = (unsigned int)(rx_len * 4);

    data = strtoul(rx_buf, NULL, 16);

    ir_send(code_type, data, nbits);
}


int ir_msg_handle(unsigned char msg_type, char *rx_buf, int rx_len, char *ret_buf, int *ret_len)
{
    switch(msg_type) {
        case IMT_CONNECT:
            ir_printf(MODULE "-> IRConnect");
            ENCAP_RET_BUFFER("IRConnect: OK");
            break;
        case IMT_ENABLE_INPUT_CC:
            ir_printf(MODULE "-> IREnableICC");
            ir_icc_enable_set(1);
            ENCAP_RET_BUFFER("IREnableICC: OK");
            break;
        case IMT_DISABLE_INPUT_CC:
            ir_printf(MODULE "-> IRDisableICC");
            ir_icc_enable_set(0);
            ENCAP_RET_BUFFER("IRDisableICC: OK");
            break;
        case IMT_DECODE:
            ir_printf(MODULE "-> IRDecode");
            ir_msg_handle_decode(ret_buf, ret_len) ;
            break;
        case IMT_SEND_NEC:
            ir_printf(MODULE "-> IRSendNEC");
            ir_msg_handle_send(IR_NEC, rx_buf, rx_len);
            ENCAP_RET_BUFFER("IRSendNEC: OK");
            break;
        default:
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
