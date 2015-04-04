/*
 * ir tx/rx test
 */
#include "main.h"
#ifdef  CONFIG_LOCAL_IR

#include "stdio.h"
#include "ctype.h"

#include "stm32f2xx.h"
#include "platform.h"
#include "mxchipWNET.h"

#include "ir_dec.h"
#include "Ir_enc.h"

#define MODULE "[ir test] "

#define IR_SEND_DIRECT
#define IR_KEY_VOL_UP 0x80bfa15e

void delay_us(__IO uint32_t nTime);

typedef struct {
    IR_DecResult res;
    uint8_t rx_cfg;
    uint8_t tx_cfg;
    uint8_t dec;
} IR_REMOTE;

static IR_REMOTE irrem;

static void ir_remote_init(void)
{
    irrem.rx_cfg = 0;
    irrem.tx_cfg = 0;
    irrem.dec = 0;
}

static void ir_rx_main(void)
{
    if(!irrem.rx_cfg) {
        printf(MODULE "[RX] rx config\r\n");
        ir_rx_config();
        irrem.rx_cfg = 1;
    }

    while(1) { //add timeout limit later
        if(ir_decode(&irrem.res)) {
            ir_raw_data_print(&irrem.res);
            ir_rx_next();
            irrem.dec = 1;
            printf(MODULE "[RX] decode len:%d,value:0x%x\r\n", irrem.res.bits, irrem.res.value);
            return;
        }
    }
}

static void ir_tx_main(void)
{
#if !defined(IR_SEND_DIRECT)
    if(!irrem.dec) {
        printf(MODULE "[TX] IR decode is NOT ok!!!\r\n");
        return;
    }
    printf(MODULE "[TX]decode len:%d,value:0x%x\r\n", irrem.res.bits, irrem.res.value);
#endif

    if(!irrem.tx_cfg) {
        ir_tx_config();
        irrem.tx_cfg = 1;
    }

#if defined(IR_SEND_DIRECT)
    irrem.res.value = IR_KEY_VOL_UP;
    irrem.res.bits = 32;
#endif
    ir_send(IR_NEC, irrem.res.value, irrem.res.bits);
}

uint8_t usart_get_byte(void)
{
    uint8_t key = 0;

    while (1) {
        if ( USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) != RESET) {
            key = USART_ReceiveData(USARTx);
            return key;
        }
    }
}

void ir_test_menu(void)
{
    uint8_t cmd = 0;

    printf("\r\n===========================================");
    printf("\r\n=          STM32F20x IR Rx and Tx         =");
    printf("\r\n===========================================");
    printf("\r\n\r\n");

    ir_remote_init();

    while(1) {
        printf("\r\n===================================");
        printf("\r\n============ Main Menu ============");
        printf("\r\nIR Receive----------------------[1]");
        printf("\r\nIR Transmit---------------------[2]");
        printf("\r\n===================================");
        printf("\r\nPlease select the option...");

        cmd = usart_get_byte();
        if(cmd == '1') {
            printf("\r\nIR Receiver :\r\n");
            ir_rx_main();
        } else if(cmd == '2') {
            printf("\r\nIR Transmit :\r\n");
            ir_tx_main();
        } else {
            printf("\r\nInvalid number!\r\n");
        }
    }
}
#endif

/*********************************END OF FILE**********************************/
