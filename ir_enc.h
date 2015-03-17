/**
 ******************************************************************************
 * IR Encoder Header File
 *
 * Modifications:
 =========================================
 * 10Jan15, yue hu, created.
 */

#ifndef __IR_ENCODE_H__
#define __IR_ENCODE_H__

// GPIO mode
#define GPIO_MODE_PWM 1
#define GPIO_MODE_LOW 2

// GPIO pin
#define IR_TX_PORT_DAT	GPIOA
#define IR_TX_PORT_PWR  GPIOC
#define IR_TX_PIN_DAT   GPIO_Pin_8
#define IR_TX_PIN_VCC   GPIO_Pin_5
#define IR_TX_PIN_GND   GPIO_Pin_4

// RCC
#define IR_TX_CLK_TIM   (RCC_APB2Periph_TIM1)
#define IR_TX_CLK_GPIO  (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO)
#define IR_TX_CLK       (IR_TX_CLK_TIM | IR_TX_CLK_GPIO)

// tim config
#define IR_TX_TIM            TIM1
#define IR_TX_TIM_CNTMOD     TIM_CounterMode_Up
#define IR_TX_TIM_OCMOD      TIM_OCMode_PWM2
#define IR_TX_TIM_OCPOL      TIM_OCPolarity_Low
#define IR_TX_TIM_OCOUTPSTA  TIM_OutputState_Enable
#define IR_TX_TIM_OCIDLESTA  TIM_OCIdleState_Set

//extern functions
extern void IR_Tx_Config(void);
extern void IR_SendNEC(uint32_t data, uint32_t nbits);

#endif
/*********************************END OF FILE**********************************/

