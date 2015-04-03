/*
 * ir encoder
 */
#include <stdio.h>
#include "stm32f2xx.h"
#include "ir_enc.h"
#include "ir_dec.h"

#define MODULE "[ir send] "

#define CONFIG_MXCHIPWNET

#ifndef CONFIG_MXCHIPWNET
static __IO uint32_t TimingDelay;

void delay_us(__IO uint32_t nTime)
{
    TimingDelay = nTime;

    while(TimingDelay != 0);
}

void timing_delay_decrement(void)
{
    if (TimingDelay != 0x00) {
        TimingDelay--;
    }
}
#else
void delay_us(uint32_t n)
{
    uint8_t i = 0;

    while(n--) {
        i = 12;
        while(i--) {
            __NOP();
        }
    }
}
#endif

void ir_tx_om_set(unsigned char mode)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    if(mode == GPIO_MODE_PWM) {
        GPIO_InitStructure.GPIO_Pin   = IR_TX_PIN_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_Init(IR_TX_PORT_DAT, &GPIO_InitStructure);
        GPIO_PinAFConfig(IR_TX_PORT_DAT, IR_TX_PIN_DATSRC, IR_TX_PIN_AF);
    } else {
        GPIO_InitStructure.GPIO_Pin   = IR_TX_PIN_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(IR_TX_PORT_DAT, &GPIO_InitStructure);
        GPIO_ResetBits(IR_TX_PORT_DAT, IR_TX_PIN_DAT);
    }
}


void nec_boot(TIM_TypeDef * TIMx)
{
    ir_tx_om_set(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    delay_us(9000);

    TIM_Cmd(TIMx, DISABLE);
    ir_tx_om_set(GPIO_MODE_LOW);
    delay_us(4500);
}

void nec_one(TIM_TypeDef * TIMx)
{
    ir_tx_om_set(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    delay_us(565);

    TIM_Cmd(TIMx, DISABLE);
    ir_tx_om_set(GPIO_MODE_LOW);
    delay_us(1695);
}

void nec_zero(TIM_TypeDef * TIMx)
{
    ir_tx_om_set(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    delay_us(565);

    TIM_Cmd(TIMx, DISABLE);
    ir_tx_om_set(GPIO_MODE_LOW);
    delay_us(565);
}

static void nec_end(TIM_TypeDef *TIMx)
{
    /* No34 pulse based from No1, calulate No33 pulse width */
    ir_tx_om_set(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    delay_us(565);
    TIM_Cmd(TIMx, DISABLE);
    ir_tx_om_set(GPIO_MODE_LOW);
    delay_us(565);

    /* mybe it's optional, No35 pulse just for decode end flag. */
    ir_tx_om_set(GPIO_MODE_PWM);
    TIM_Cmd(TIMx, ENABLE);
    delay_us(565);
    TIM_Cmd(TIMx, DISABLE);
    ir_tx_om_set(GPIO_MODE_LOW);
}

void ir_tx_tim_config(uint8_t khz)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    uint16_t TimerPeriod = 0;
    uint16_t ChannelPulse = 0;

    /* Compute the value to be set in ARR regiter to generate signal frequency at param of khz*/
    TimerPeriod = (SystemCoreClock / (khz * 1000) ) - 1;
    /* Compute CCR1 value to generate a duty cycle at 25% for channel 1 */
    ChannelPulse = (uint16_t) (((uint32_t) 25 * (TimerPeriod - 1)) / 100);

    /* Time Base configuration */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = IR_TX_TIM_CNTMOD;
    TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(IR_TX_TIM, &TIM_TimeBaseStructure);

    /* Channel in PWM mode */
    TIM_OCInitStructure.TIM_OCMode = IR_TX_TIM_OCMOD;
    TIM_OCInitStructure.TIM_OutputState = IR_TX_TIM_OCOUTPSTA;
    TIM_OCInitStructure.TIM_Pulse = ChannelPulse;
    TIM_OCInitStructure.TIM_OCPolarity = IR_TX_TIM_OCPOL;
    TIM_OCInitStructure.TIM_OCIdleState = IR_TX_TIM_OCIDLESTA;
    TIM_OC2Init(IR_TX_TIM, &TIM_OCInitStructure);

    TIM_OC2PreloadConfig(IR_TX_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(IR_TX_TIM, ENABLE);

    /* TIM counter enable */
    TIM_Cmd(IR_TX_TIM, ENABLE);

    /* the below is for tim1 or tim8 */
#if defined(CONFIG_TIM1) || defined(CONFIG_TIM8)
    TIM_CtrlPWMOutputs(IR_TX_TIM, ENABLE);
#endif
}

#ifndef CONFIG_MXCHIPWNET
static void ir_tx_systick_config(void)
{
    /* Setup SysTick Timer for 1 microsec interrupts */
    if (SysTick_Config(SystemCoreClock / 1000000)) {
        /* Capture error */
        printf("Setup SysTick Timer Error!!!\n\r");
        while (1);
    }
}
#endif

static void ir_tx_rcc_config(void)
{
    /* all clocks enable */
    RCC_APB1PeriphClockCmd(IR_TX_CLK_TIM, ENABLE);
    RCC_AHB1PeriphClockCmd(IR_TX_CLK_GPIO, ENABLE);
}

static void ir_tx_gpio_config(void)
{
    // data pin
    ir_tx_om_set(GPIO_MODE_LOW);
}

void ir_tx_config(void)
{
#ifndef CONFIG_MXCHIPWNET
    ir_tx_systick_config();
#endif
    ir_tx_rcc_config();
    ir_tx_gpio_config();
    ir_tx_tim_config(38); //enable 38khz
}

/**
 * send nec data
 */
static void ir_send_nec(uint32_t data, uint32_t nbits)
{
    unsigned char i = 0;

    // send code
    nec_boot(IR_TX_TIM);

    for(i = 1; i < nbits; i++) {
        if(data & TOPBIT) {
            nec_one(IR_TX_TIM);
        } else {
            nec_zero(IR_TX_TIM);
        }
        data <<= 1;
    }

    nec_end(IR_TX_TIM);
}

void ir_send(uint8_t type, uint32_t data, uint32_t nbits)
{
    switch(type) {
        case IR_NEC:
            printf(MODULE "send nec data 0x%x, length %d\r\n", data, nbits);
            ir_send_nec(data, nbits);
            break;
        default:
            printf(MODULE "not support!\r\n");
            break;
    }
}


