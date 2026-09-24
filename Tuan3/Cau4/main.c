#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_usart.h"
#include "misc.h"

#define ADC_BUFFER_SIZE 100
#define TX_BUFFER_SIZE 512

volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];

volatile uint8_t dma_half_ready = 0;
volatile uint8_t dma_full_ready = 0;

volatile uint8_t uart_dma_busy = 0;

char tx_buffer[TX_BUFFER_SIZE];
volatile uint16_t tx_length = 0;
static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_AFIO |
        RCC_APB2Periph_ADC1 |
        RCC_APB2Periph_USART1,
        ENABLE
    );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
}
static void USART1_Config(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 115200;

    USART_InitStructure.USART_WordLength =
        USART_WordLength_8b;

    USART_InitStructure.USART_StopBits =
        USART_StopBits_1;

    USART_InitStructure.USART_Parity =
        USART_Parity_No;

    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;

    USART_InitStructure.USART_Mode =
        USART_Mode_Tx |
        USART_Mode_Rx;

    USART_Init(
        USART1,
        &USART_InitStructure
    );
    USART_DMACmd(
        USART1,
        USART_DMAReq_Tx,
        ENABLE
    );

    USART_Cmd(
        USART1,
        ENABLE
    );
}
static void TIM3_Config(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM3,
        ENABLE
    );

    TIM_TimeBaseStructure.TIM_Prescaler = 7199;

    TIM_TimeBaseStructure.TIM_Period = 99;

    TIM_TimeBaseStructure.TIM_CounterMode =
        TIM_CounterMode_Up;

    TIM_TimeBaseStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;

    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(
        TIM3,
        &TIM_TimeBaseStructure
    );
    TIM_SelectOutputTrigger(
        TIM3,
        TIM_TRGOSource_Update
    );
}
static void ADC1_Config(void)
{
    ADC_InitTypeDef ADC_InitStructure;

    ADC_InitStructure.ADC_Mode =
        ADC_Mode_Independent;

    ADC_InitStructure.ADC_ScanConvMode =
        DISABLE;

    ADC_InitStructure.ADC_ContinuousConvMode =
        DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv =
        ADC_ExternalTrigConv_T3_TRGO;

    ADC_InitStructure.ADC_DataAlign =
        ADC_DataAlign_Right;

    ADC_InitStructure.ADC_NbrOfChannel = 1;

    ADC_Init(
        ADC1,
        &ADC_InitStructure
    );
    ADC_RegularChannelConfig(
        ADC1,
        ADC_Channel_0,
        1,
        ADC_SampleTime_55Cycles5
    );
    ADC_DMACmd(
        ADC1,
        ENABLE
    );
    ADC_Cmd(
        ADC1,
        ENABLE
    );
    ADC_ResetCalibration(
        ADC1
    );

    while (
        ADC_GetResetCalibrationStatus(ADC1)
    );

    ADC_StartCalibration(
        ADC1
    );

    while (
        ADC_GetCalibrationStatus(ADC1)
    );
    ADC_ExternalTrigConvCmd(
        ADC1,
        ENABLE
    );
}
static void DMA_ADC_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(
        RCC_AHBPeriph_DMA1,
        ENABLE
    );

    DMA_DeInit(
        DMA1_Channel1
    );

    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&ADC1->DR;

    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)adc_buffer;

    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralSRC;

    DMA_InitStructure.DMA_BufferSize =
        ADC_BUFFER_SIZE;

    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;

    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;

    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_HalfWord;

    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Circular;

    DMA_InitStructure.DMA_Priority =
        DMA_Priority_High;

    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;

    DMA_Init(
        DMA1_Channel1,
        &DMA_InitStructure
    );
    DMA_ITConfig(
        DMA1_Channel1,
        DMA_IT_HT | DMA_IT_TC,
        ENABLE
    );

    DMA_Cmd(
        DMA1_Channel1,
        ENABLE
    );
}

static void DMA_USART_TX_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(
        DMA1_Channel4
    );

    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&USART1->DR;

    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)tx_buffer;

    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralDST;

    DMA_InitStructure.DMA_BufferSize = 1;

    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;

    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;

    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_Byte;

    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_Byte;

    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Normal;

    DMA_InitStructure.DMA_Priority =
        DMA_Priority_Medium;

    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;

    DMA_Init(
        DMA1_Channel4,
        &DMA_InitStructure
    );
    DMA_ITConfig(
        DMA1_Channel4,
        DMA_IT_TC,
        ENABLE
    );

    DMA_Cmd(
        DMA1_Channel4,
        DISABLE
    );
}

static void NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel =
        DMA1_Channel1_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;

    NVIC_Init(
        &NVIC_InitStructure
    );
    NVIC_InitStructure.NVIC_IRQChannel =
        DMA1_Channel4_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        1;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        0;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;

    NVIC_Init(
        &NVIC_InitStructure
    );
}

static void TX_AddChar(char c)
{
    if (tx_length < TX_BUFFER_SIZE - 1)
    {
        tx_buffer[tx_length++] = c;
    }
}

static void TX_AddNumber(uint16_t number)
{
    char temp[6];

    uint8_t i = 0;

    if (number == 0)
    {
        TX_AddChar('0');
        return;
    }

    while (number > 0)
    {
        temp[i++] =
            '0' + (number % 10);

        number /= 10;
    }

    while (i > 0)
    {
        TX_AddChar(
            temp[--i]
        );
    }
}
static void Make_ADC_Message(
    uint16_t start,
    uint16_t count
)
{
    uint16_t i;

    tx_length = 0;

    for (i = 0; i < count; i++)
    {
        TX_AddNumber(
            adc_buffer[start + i]
        );

        TX_AddChar('\r');
        TX_AddChar('\n');
    }
}
static void USART1_DMA_Send(void)
{
    if (tx_length == 0)
    {
        return;
    }

    if (uart_dma_busy)
    {
        return;
    }

    uart_dma_busy = 1;
    DMA_Cmd(
        DMA1_Channel4,
        DISABLE
    );
    DMA_ClearFlag(
        DMA1_FLAG_GL4 |
        DMA1_FLAG_TC4 |
        DMA1_FLAG_HT4 |
        DMA1_FLAG_TE4
    );
    DMA1_Channel4->CMAR =
        (uint32_t)tx_buffer;
    DMA1_Channel4->CNDTR =
        tx_length;
    DMA_Cmd(
        DMA1_Channel4,
        ENABLE
    );
}
void DMA1_Channel1_IRQHandler(void)
{
    if (
        DMA_GetITStatus(
            DMA1_IT_HT1
        ) != RESET
    )
    {
        DMA_ClearITPendingBit(
            DMA1_IT_HT1
        );

        dma_half_ready = 1;
    }
    if (
        DMA_GetITStatus(
            DMA1_IT_TC1
        ) != RESET
    )
    {
        DMA_ClearITPendingBit(
            DMA1_IT_TC1
        );

        dma_full_ready = 1;
    }
}


void DMA1_Channel4_IRQHandler(void)
{
    if (
        DMA_GetITStatus(
            DMA1_IT_TC4
        ) != RESET
    )
    {
        DMA_ClearITPendingBit(
            DMA1_IT_TC4
        );

        DMA_Cmd(
            DMA1_Channel4,
            DISABLE
        );

        uart_dma_busy = 0;
    }
}


int main(void)
{
    GPIO_Config();
    USART1_Config();
    DMA_ADC_Config();
    DMA_USART_TX_Config();
    ADC1_Config();
    TIM3_Config();
    NVIC_Config();
    TIM_Cmd(
        TIM3,
        ENABLE
    );

    while (1)
    {
        if (dma_half_ready)
        {
            dma_half_ready = 0;

            if (!uart_dma_busy)
            {
                Make_ADC_Message(
                    0,
                    50
                );

                USART1_DMA_Send();
            }
        }
        if (dma_full_ready)
        {
            dma_full_ready = 0;

            if (!uart_dma_busy)
            {
                Make_ADC_Message(
                    50,
                    50
                );

                USART1_DMA_Send();
            }
        }
    }
}
