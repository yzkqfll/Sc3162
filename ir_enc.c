/**
 ******************************************************************************
 * IR Encoder Implementation File
 *
 * Modifications:
 =========================================
 * 10Jan15, yue hu, created.
 */
#include <stdio.h>
#include "stm32f10x_conf.h"
#include "ir_enc.h"
#include "ir_dec.h"

static __IO uint32_t TimingDelay;

void Delay_us(__IO uint32_t nTime)
{
    TimingDelay = nTime;

    while(TimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00) {
        TimingDelay--;
    }
}

void IR_Tx_OMSet(unsigned char mode)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    if(mode == GPIO_MODE_PWM) {
        GPIO_InitStructure.GPIO_Pin   = IR_TX_PIN_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(IR_TX_PORT_DAT, &GPIO_InitStructure);
    } else {
        GPIO_InitStructure.GPIO_Pin   = IR_TX_PIN_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(IR_TX_PORT_DAT, &GPIO_InitStructure);
        GPIO_ResetBits(IR_TX_PORT_DAT, IR_TX_PIN_DAT);
    }
}


void NEC_Boot(TIM_TypeDef * TIMx)
{
    IR_Tx_OMSet(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    Delay_us(9000);

    TIM_Cmd(TIMx, DISABLE);
    IR_Tx_OMSet(GPIO_MODE_LOW);
    Delay_us(4500);
}

void NEC_One(TIM_TypeDef * TIMx)
{
    IR_Tx_OMSet(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    Delay_us(565);

    TIM_Cmd(TIMx, DISABLE);
    IR_Tx_OMSet(GPIO_MODE_LOW);
    Delay_us(1695);
}

void NEC_Zero(TIM_TypeDef * TIMx)
{
    IR_Tx_OMSet(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    Delay_us(565);

    TIM_Cmd(TIMx, DISABLE);
    IR_Tx_OMSet(GPIO_MODE_LOW);
    Delay_us(565);
}

static void NEC_End(TIM_TypeDef *TIMx)
{
    //No34 pulse based from No1, calulate No33 pulse width
    IR_Tx_OMSet(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    Delay_us(565);
    TIM_Cmd(TIMx, DISABLE);
    IR_Tx_OMSet(GPIO_MODE_LOW);
    Delay_us(565);

    //i think it's optional, No35 pulse just for decode end flag.
    IR_Tx_OMSet(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    Delay_us(565);
    TIM_Cmd(TIMx, DISABLE);
    IR_Tx_OMSet(GPIO_MODE_LOW);
}

void IR_Tx_TIM_Config(uint8_t khz)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    uint16_t TimerPeriod = 0;
    uint16_t Channel1Pulse = 0;

    /* Compute the value to be set in ARR regiter to generate signal frequency at param of khz*/
    TimerPeriod = (SystemCoreClock / (khz*1000) ) - 1;
    /* Compute CCR1 value to generate a duty cycle at 25% for channel 1 */
    Channel1Pulse = (uint16_t) (((uint32_t) 25 * (TimerPeriod - 1)) / 100);

    /* Time Base configuration */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = IR_TX_TIM_CNTMOD;
    TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(IR_TX_TIM, &TIM_TimeBaseStructure);

    /* Channel 1 in PWM mode */
    TIM_OCInitStructure.TIM_OCMode = IR_TX_TIM_OCMOD;
    TIM_OCInitStructure.TIM_OutputState = IR_TX_TIM_OCOUTPSTA;
    TIM_OCInitStructure.TIM_Pulse = Channel1Pulse;
    TIM_OCInitStructure.TIM_OCPolarity = IR_TX_TIM_OCPOL;
    TIM_OCInitStructure.TIM_OCIdleState = IR_TX_TIM_OCIDLESTA;
    TIM_OC1Init(IR_TX_TIM, &TIM_OCInitStructure);

    /* TIM1 counter enable */
    TIM_Cmd(IR_TX_TIM, ENABLE);

    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(IR_TX_TIM, ENABLE);
}

static void IR_Tx_SysTick_Config(void)
{
    /* Setup SysTick Timer for 1 microsec interrupts */
    if (SysTick_Config(SystemCoreClock / 1000000)) {
        /* Capture error */
        printf("Setup SysTick Timer Error!!!\n\r");
        while (1);
    }
}

static void IR_Tx_RCC_Config(void)
{
    /* all clocks enable */
    RCC_APB2PeriphClockCmd(IR_TX_CLK, ENABLE);
}

static void IR_Tx_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // data pin
	IR_Tx_OMSet(GPIO_MODE_LOW);

    // vcc and ground pin
    GPIO_InitStructure.GPIO_Pin = IR_TX_PIN_GND | IR_TX_PIN_VCC;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(IR_TX_PORT_PWR, &GPIO_InitStructure);
    GPIO_SetBits(IR_TX_PORT_PWR,  IR_TX_PIN_VCC);
    GPIO_ResetBits(IR_TX_PORT_PWR,  IR_TX_PIN_GND);
}

void IR_Tx_Config(void)
{
    IR_Tx_SysTick_Config();
    IR_Tx_RCC_Config();
    IR_Tx_GPIO_Config();
}

/**
 * send nec data
 */
void IR_SendNEC(uint32_t data, uint32_t nbits)
{
    unsigned char i;

    // enable 38KHz
	IR_Tx_TIM_Config(38);

    // send code
    NEC_Boot(IR_TX_TIM);

    for(i = 1; i < nbits; i++) {
        if(data & TOPBIT) {
            NEC_One(IR_TX_TIM);
        } else{
            NEC_Zero(IR_TX_TIM);
        }
		data <<= 1;
    }

    NEC_End(IR_TX_TIM);
}


