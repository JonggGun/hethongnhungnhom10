#ifndef STM32F103_H
#define STM32F103_H

#include <stdint.h>

/* ===================== RCC ===================== */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

#define RCC_BASE        0x40021000UL
#define RCC             ((RCC_TypeDef *)RCC_BASE)

/* ===================== GPIO ===================== */
typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

#define GPIOA_BASE      0x40010800UL
#define GPIOA           ((GPIO_TypeDef *)GPIOA_BASE)

/* ===================== AFIO ===================== */
typedef struct {
    volatile uint32_t EVCR;
    volatile uint32_t MAPR;
    volatile uint32_t EXTICR[4];
    uint32_t          RESERVED0;
    volatile uint32_t MAPR2;
} AFIO_TypeDef;

#define AFIO_BASE       0x40010000UL
#define AFIO            ((AFIO_TypeDef *)AFIO_BASE)

/* ===================== TIM2 (general purpose timer) ===================== */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    uint32_t          RESERVED0;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    uint32_t          RESERVED1;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;

#define TIM2_BASE       0x40000000UL
#define TIM2            ((TIM_TypeDef *)TIM2_BASE)

/* ===================== USART1 ===================== */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

#define USART1_BASE     0x40013800UL
#define USART1          ((USART_TypeDef *)USART1_BASE)

/* ===================== NVIC (tối giản, chỉ ISER) ===================== */
typedef struct {
    volatile uint32_t ISER[3];
} NVIC_TypeDef;

#define NVIC_BASE       0xE000E100UL
#define NVIC            ((NVIC_TypeDef *)NVIC_BASE)

#endif /* STM32F103_H */
