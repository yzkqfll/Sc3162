#include "stdio.h"
#include "ctype.h"

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
    IMT_SEND,
};

#define DEBUG

#if defined(DEBUG)
#define ir_printf   printf
#else
#define ir_printf
#endif

static IR_DecResult idr;

int ir_msg_handle_decode(void)
{
    if(ir_decode(&idr)) {
        if(idr.type == IR_NEC) {
            ir_raw_data_print(&idr);
            ir_rx_next();
            return 1;
        }
        ir_rx_next();
    }
    return 0;
}

int ir_msg_handle_send(char *rx_buf, int rx_len)
{
    return 1;
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
            if(ir_msg_handle_decode()) {
                ENCAP_RET_BUFFER("IRDecode: OK");
            } else {
                ENCAP_RET_BUFFER("IRDecode: ERR");
            }
            break;
        case IMT_SEND:
            ir_printf(MODULE "-> IRSend");
            ir_msg_handle_send(rx_buf, rx_len);
            ENCAP_RET_BUFFER("IRSend: OK");
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
