/*
 * ir decoder
 */
#include <stdio.h>
#include "ir_dec.h"
#include "stm32f2xx.h"

#define MODULE "[ir decode] "

// variables in this file
static IR_IntInfo ir_int_info;

static void ir_rx_gpio_config(void)
{
    GPIO_InitTypeDef    GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(IR_RX_GPIO_CLK, ENABLE);

    // data pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = IR_RX_PIN_DAT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(IR_RX_PORT_DAT, &GPIO_InitStructure);

    // Connect TIM pins to AF
    GPIO_PinAFConfig(IR_RX_PORT_DAT, IR_RX_PIN_DATSRC, IR_RX_PIN_AF);
}

void ir_icc_enable_set(int tof)
{
    if(tof) {
        TIM_ITConfig(IR_RX_TIM, TIM_IT_Update | IR_RX_TIM_IT, ENABLE);
    } else {
        TIM_ITConfig(IR_RX_TIM, TIM_IT_Update | IR_RX_TIM_IT, DISABLE);
    }
}


/**
  * ir rx tim configuration
  */
static void ir_rx_tim_config(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;

    RCC_APB2PeriphClockCmd(IR_RX_TIM_CLK, ENABLE);

    /* Time Base configuration */
    TIM_TimeBaseStructure.TIM_Prescaler = IR_RX_TIM_TBPSC;
    TIM_TimeBaseStructure.TIM_CounterMode = IR_RX_TIM_TBCNTM;
    TIM_TimeBaseStructure.TIM_Period = IR_RX_TIM_TBPERIOD;
    TIM_TimeBaseStructure.TIM_ClockDivision = IR_RX_TIM_TBCLK;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = IR_RX_TIM_TBREP;
    TIM_TimeBaseInit(IR_RX_TIM, &TIM_TimeBaseStructure);

    TIM_ICInitStructure.TIM_Channel     = IR_RX_TIM_ICCH;
    TIM_ICInitStructure.TIM_ICPrescaler = IR_RX_TIM_ICPSC;
    TIM_ICInitStructure.TIM_ICFilter    = IR_RX_TIM_ICFLT;
    TIM_ICInitStructure.TIM_ICPolarity  = IR_RX_TIM_ICPOL;
    TIM_ICInitStructure.TIM_ICSelection = IR_RX_TIM_ICSEL;;
    TIM_ICInit(IR_RX_TIM, &TIM_ICInitStructure);

    /* TIM enable counter */
    TIM_Cmd(IR_RX_TIM, ENABLE);

    /* Default disable the CC1 and Up Interrupt Request */
    ir_icc_enable_set(0);
}

/**
  * Get the ir pulse width using tim cc and up interrupt
  *
  */
static void ir_rx_int_service(void)
{
    if(TIM_GetITStatus(IR_RX_TIM, IR_RX_TIM_IT) == SET) {
        TIM_ITConfig(IR_RX_TIM, TIM_IT_Update, DISABLE);
        switch (ir_int_info.state) {
            case IR_STATE_NO: //the first falling edge
                TIM_SetCounter(IR_RX_TIM, 0);
                ir_int_info.rawLen = 0;
                ir_int_info.state = IR_STATE_READ;
                break;
            case IR_STATE_READ:
                if(ir_int_info.rawLen >= IR_RAW_LEN) {
                    ir_int_info.state = IR_STATE_OVERFLOW;
                } else {
                    ir_int_info.rawBuf[ir_int_info.rawLen++] = TIM_GetCounter(IR_RX_TIM);
                    TIM_SetCounter(IR_RX_TIM, 0);
                }
                break;
            case IR_STATE_OVERFLOW:
                // nothing
                break;
            default:
                break;
        }
        TIM_ClearITPendingBit(IR_RX_TIM, IR_RX_TIM_IT);
        TIM_ClearITPendingBit(IR_RX_TIM, TIM_IT_Update);
        TIM_ITConfig(IR_RX_TIM, TIM_IT_Update, ENABLE);

    } else if(TIM_GetITStatus(IR_RX_TIM, TIM_IT_Update) == SET) { //TIM overflow, its end
        if (ir_int_info.state == IR_STATE_OVERFLOW) {
            //ir_int_info.state = IR_STATE_NO;
            ir_int_info.rawLen = 0;
        } else if (ir_int_info.state == IR_STATE_READ) {
            ir_int_info.state = IR_STATE_OK; //ir raw code is ok
        }
        TIM_ClearITPendingBit(IR_RX_TIM, TIM_IT_Update);
        TIM_ITConfig(IR_RX_TIM, TIM_IT_Update, DISABLE);
    }
}

void TIM8_CC_IRQHandler(void)
{
    ir_rx_int_service();
}

void TIM8_UP_TIM13_IRQHandler(void)
{
    ir_rx_int_service();
}

static void ir_rx_nvic_config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    /* Enable the CC Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = IR_RX_CC_INT;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the Update Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = IR_RX_UP_INT;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void ir_rx_state_init(void)
{
    int i;

    for(i = 0; i < IR_RAW_LEN; i++) {
        ir_int_info.rawBuf[i] = 0;
    }
    ir_int_info.rawLen = 0;
    ir_int_info.state = IR_STATE_NO;
}

void ir_rx_next(void)
{
    ir_rx_state_init();
}

/**
  * IR rx configurations
  */
void ir_rx_config(void)
{
    ir_rx_state_init();
    ir_rx_gpio_config();
    ir_rx_nvic_config();
    ir_rx_tim_config();
}

/**
 * convert the raw data to binary code
 * return 1 is valid
 */
uint32_t ir_decode_nec(IR_DecResult *result)
{
    uint8_t i = 0;
    uint32_t data = 0;

    if ((result->rawBuf[i] > NEC_HDR_MIN) && (result->rawBuf[i] < NEC_HDR_MAX)) {

        if(result->rawLen <  NEC_CODE_LEN) {
            printf(MODULE "Header is NEC, but pulse count is not enough(count:%d) is not right!!!\r\n", result->rawLen);
            return 0; //not possible
        }

        for (i = 1; i < NEC_CODE_LEN; i++) {
            if(result->rawBuf[i] > NEC_ONE_MIN && result->rawBuf[i] < NEC_ONE_MAX) {
                data = (data << 1) | 1;
            } else if (result->rawBuf[i] > NEC_ZERO_MIN && result->rawBuf[i] < NEC_ZERO_MAX) {
                data <<= 1;
            }
        }
        result->bits = NEC_CODE_LEN;
        result->type = IR_NEC;
        result->value = data;
        return 1;
    } else {
        return 0; //not NEC
    }
}

/**
 * IR decode
 * @param  IR_DecResult *
 * @return  -1 - not capture
 *           0 - captured and decode failed
 *           1 - captured and decode succeed
 */
int ir_decode(IR_DecResult *result)
{
    if(ir_int_info.state != IR_STATE_OK)
        return -1;

    printf(MODULE "Signal is captured: header width=%d, pulse count=%d\r\n",
           ir_int_info.rawBuf[0], ir_int_info.rawLen);

    result->rawBuf = ir_int_info.rawBuf;
    result->rawLen = ir_int_info.rawLen;

    if(ir_decode_nec(result))
        return 1;

    /*
      if(ir_decode_rc5(result))
       return 1;
      */

    result->type = IR_UNKNOWN;
    result->bits = 0;
    result->value = 0;
    return 0;
}

/**
 * NEC protocol:
 * three type pulse: boot pulse + logic 0 + logic 1
 * full code: boot code + addr code + !addr + cmd + !cmd
 * the addr and cmd code is combination of 0 and 1
 */
void ir_raw_data_print(IR_DecResult *result)
{
    int i;

    printf(MODULE "Raw data beg : \r\n");
    printf(MODULE "Raw head is %d\r\n", result->rawBuf[0]);
    printf(MODULE "Raw buffer length is %d\r\n", result->rawLen);
    printf(MODULE "Raw user info:\r\n");
    for(i = 1; i < result->rawLen; i++) {
        printf("%d ", result->rawBuf[i]);
        if(i % 8 == 0)
            printf("\r\n");
    }
    printf(MODULE "Raw data end . \r\n");


}

/*********************************END OF FILE**********************************/
