/**
 ******************************************************************************
 * IR Decoder Implementation File
 *
 * Modifications:
 =========================================
 * 7Jan15, yue hu, created.
 */
#include <stdio.h>
#include "ir_dec.h"
#include "usart.h"
#include "stm32f10x_conf.h"


// variables in this file
static IR_IntInfo ir_int_info;


/**
  * ir rx 3pin configuration
  * gpio-ground, gpio-vcc, gpio(tim)-data
  */
static void IR_Rx_GPIO_Config(void)
{
    GPIO_InitTypeDef    GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(IR_RX_GPIO_CLK, ENABLE);

    // data pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = IR_RX_PIN_DAT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IR_RX_PORT, &GPIO_InitStructure);

    // vcc and ground pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz ;
    GPIO_InitStructure.GPIO_Pin = IR_RX_PIN_GND | IR_RX_PIN_VCC;
    GPIO_Init( IR_RX_PORT, &GPIO_InitStructure);
    GPIO_SetBits(IR_RX_PORT,  IR_RX_PIN_VCC);
    GPIO_ResetBits(IR_RX_PORT,  IR_RX_PIN_GND);
}


/**
  * ir rx tim configuration
  */
static void IR_Rx_TIM_Config(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;

    RCC_APB2PeriphClockCmd(IR_RX_TIM_CLK, ENABLE);

    /* Time Base configuration */
    /* 72M/7200 = 0.1ms */
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

    /* Enable the CC1 Interrupt Request */
    TIM_ITConfig(IR_RX_TIM, IR_RX_TIM_IT, ENABLE);
}

/**
  * Get the ir pulse width using tim cc and up interrupt
  *
  */
static void IR_Rx_IntService(void)
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
            ir_int_info.state = IR_STATE_NO;
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
    IR_Rx_IntService();
}

void TIM8_UP_IRQHandler(void)
{
    IR_Rx_IntService();
}

static void IR_Rx_NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the TIM global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = IR_RX_CC_INT;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the TIM global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = IR_RX_UP_INT;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void IR_Rx_State_Init(void)
{
    ir_int_info.state = IR_STATE_NO;
	ir_int_info.rawLen = 0;
}

void IR_Rx_Next(void)
{
   IR_Rx_State_Init();
}

/**
  * IR rx configurations
  */
void IR_Rx_Config(void)
{
    IR_Rx_State_Init();
    IR_Rx_GPIO_Config();
    IR_Rx_NVIC_Config();
    IR_Rx_TIM_Config();
}

/**
 * convert the raw data to binary code
 * return 1 is valid
 */
uint32_t IR_DecodeNEC(IR_DecResult *result)
{
    uint8_t i = 0;
	uint32_t data = 0;

    if ((result->rawBuf[i] > NEC_BOOT_MIN) && (result->rawBuf[i] < NEC_BOOT_MAX)) {
        for (i = 1; i < NEC_CODE_LEN; i++) {
            if(result->rawBuf[i] > NEC_H_MIN && result->rawBuf[i] < NEC_H_MAX) {
                data = (data << 1) | 1;
            } else if (result->rawBuf[i] > NEC_L_MIN && result->rawBuf[i] < NEC_L_MAX) {
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
 * @return   0 - decode raw failed
 *                1 - decode success
 */
int IR_Decode(IR_DecResult *result)
{
    if(ir_int_info.state != IR_STATE_OK)
        return 0;

	result->rawBuf = ir_int_info.rawBuf;
    result->rawLen = ir_int_info.rawLen;

    if(IR_DecodeNEC(result))
        return 1;

	result->type = IR_UNKNOWN;
	result->bits = 0;
	result->value = 0;
    return 1;
}

/**
 * NEC protocol:
 * three type pulse: boot pulse + logic 0 + logic 1
 * boot code + addr code + !addr + cmd + !cmd
 * the addr/cmd code is combination of 0 and 1
 */
void IR_Raw_Data_Print(IR_DecResult *result)
{
    int i;

    printf("[RAW]Head is %d\r\n", result->rawBuf[0]);
	printf("[RAW]user info:\r\n");
    for(i = 1; i < result->rawLen; i++) {
        printf("%d ", result->rawBuf[i]);
        if(i % 8 == 0)
            printf("\r\n");
    }
    printf("\r\n");
}


/*********************************END OF FILE**********************************/
