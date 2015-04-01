#ifndef __IR_DECODE_H__
#define __IR_DECODE_H__

#include <stdint.h>

// IR raw data length
#define IR_RAW_LEN  80

// NEC CODE length, include boot
#define  NEC_CODE_LEN    33

// topbit to save decode result
#define TOPBIT 0x80000000

// IR code value
#define  IR_CODE_INIT       3
#define  IR_CODE_ZERO       0
#define  IR_CODE_ONE        1
#define  IR_CODE_INVALID    2

// NEC pulse width
#define NEC_BOOT_MAX  140
#define NEC_BOOT_MIN  130
#define NEC_H_MAX      27
#define NEC_H_MIN      17
#define NEC_L_MAX      16
#define NEC_L_MIN       6

// IR decode state
#define  IR_STATE_NO        0
#define  IR_STATE_READ      1
#define  IR_STATE_OVERFLOW  2
#define  IR_STATE_OK        3

// IR decode type
enum {
    IR_UNKNOWN = 0,
    IR_NEC,
};


// IR decode information for the interrupt handler
typedef struct {
    uint8_t pin;      // pin for IR data from detector
    uint8_t state;    // state machine
    uint32_t rawBuf[IR_RAW_LEN]; // raw data
    uint8_t rawLen;   // counter for rawBuf
} IR_IntInfo;


// IR decode results
typedef struct {
    int8_t type;         // NEC, RC5, UNKNOWN
    uint32_t value;      // Decoded value
    uint32_t bits;       // Number of bits in decoded value
    uint32_t *rawBuf;    // Raw data
    uint32_t rawLen;     // Number of records in rawbuf.
} IR_DecResult;

// GPIO pin (3162 PC6 - AF:TIM8_CH1)
#define IR_RX_GPIO_CLK   RCC_AHB1Periph_GPIOC
#define IR_RX_PORT_DAT   GPIOC
#define IR_RX_PIN_DAT    GPIO_Pin_6
#define IR_RX_PIN_DATSRC GPIO_PinSource6
#define IR_RX_PIN_AF     GPIO_AF_TIM8

// interrupt
#define IR_RX_CC_INT  TIM8_CC_IRQn
#define IR_RX_UP_INT  TIM8_UP_TIM13_IRQn

// tim config
#define IR_RX_TIM     TIM8
#define IR_RX_TIM_CLK RCC_APB2Periph_TIM8
#define IR_RX_TIM_IT  TIM_IT_CC1

#define IR_RX_TIM_TBPSC    (12000 - 1) //120M/psc => min=0.1ms
#define IR_RX_TIM_TBCNTM   TIM_CounterMode_Up
#define IR_RX_TIM_TBPERIOD 2000          //max=0.1ms * 2000 = 200
#define IR_RX_TIM_TBCLK    TIM_CKD_DIV1
#define IR_RX_TIM_TBREP    0

#define IR_RX_TIM_ICCH    TIM_Channel_1
#define IR_RX_TIM_ICPOL   TIM_ICPolarity_Falling
#define IR_RX_TIM_ICSEL   TIM_ICSelection_DirectTI
#define IR_RX_TIM_ICPSC   TIM_ICPSC_DIV1
#define IR_RX_TIM_ICFLT   8

//extern funtions
extern void ir_rx_config(void);
extern int ir_decode(IR_DecResult *result);
extern void ir_raw_data_print(IR_DecResult *result);
extern void ir_rx_next(void);
extern void ir_icc_enable_set(int tof);



#endif/* __IR_DECODE_H */
/*********************************END OF FILE**********************************/

