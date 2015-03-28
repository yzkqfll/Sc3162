#ifndef __IR_ENCODE_H__
#define __IR_ENCODE_H__

#include "stdint.h"

// GPIO mode
#define GPIO_MODE_PWM 1
#define GPIO_MODE_LOW 2

// GPIO pin (3162.Pin11-PA1 - AF:TIM5_CH2)
#define IR_TX_PORT_DAT   GPIOA
#define IR_TX_PIN_DAT    GPIO_Pin_1
#define IR_TX_PIN_DATSRC GPIO_PinSource1
#define IR_TX_PIN_AF     GPIO_AF_TIM5

// RCC
#define IR_TX_CLK_TIM   (RCC_APB1Periph_TIM5)
#define IR_TX_CLK_GPIO  (RCC_AHB1Periph_GPIOA)

// tim config
#define IR_TX_TIM            TIM5
#define IR_TX_TIM_CNTMOD     TIM_CounterMode_Up
#define IR_TX_TIM_OCMOD      TIM_OCMode_PWM1
#define IR_TX_TIM_OCPOL      TIM_OCPolarity_High
#define IR_TX_TIM_OCOUTPSTA  TIM_OutputState_Enable
#define IR_TX_TIM_OCIDLESTA  TIM_OCIdleState_Set

//extern functions
extern void ir_tx_config(void);
extern void ir_send_nec(uint32_t data, uint32_t nbits);

#endif
/*********************************END OF FILE**********************************/

