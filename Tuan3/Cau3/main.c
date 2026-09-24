#include "stm32f10x.h"
#include <stdint.h>
#define MA_LOP   "D23CQDT01"
#define MA_NHOM  "Nhom03"

#define TX_BUFFER_SIZE 64
static char tx_buffer[TX_BUFFER_SIZE];
static uint32_t button_count = 0;
static volatile uint8_t dma_busy = 0;
static void GPIO_Config(void);
static void USART1_Config(void);
static void DMA_USART1_Config(void);

static void Delay_ms(uint32_t ms);

static uint16_t String_Length(const char *str);

static void String_Copy(
    char *dest,
    const char *src
);

static void String_Append(
    char *dest,
    const char *src
);

static void UInt_To_String(
    uint32_t number,
    char *str
);

static void Make_Message(void);

static void USART1_DMA_Send(
    char *data,
    uint16_t length
);

static void DMA_CheckComplete(void);

static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_AFIO,
        ENABLE
    );
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IPU;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_9;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IN_FLOATING;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );
}
static void USART1_Config(void)
{
    USART_InitTypeDef USART_InitStructure;
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1,
        ENABLE
    );
    USART_InitStructure.USART_BaudRate =
        115200;

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
static void DMA_USART1_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    RCC_AHBPeriphClockCmd(
        RCC_AHBPeriph_DMA1,
        ENABLE
    );
    DMA_DeInit(
        DMA1_Channel4
    );
    DMA_InitStructure.DMA_PeripheralBaseAddr =
        (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr =
        (uint32_t)tx_buffer;
    DMA_InitStructure.DMA_DIR =
        DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize =
        0;

    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc =
        DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Priority =
        DMA_Priority_High;
    DMA_InitStructure.DMA_Mode =
        DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M =
        DMA_M2M_Disable;
    DMA_Init(
        DMA1_Channel4,
        &DMA_InitStructure
    );
}
static uint16_t String_Length(
    const char *str
)
{
    uint16_t length = 0;


    while (str[length] != '\0')
    {
        length++;
    }


    return length;
}
static void String_Copy(
    char *dest,
    const char *src
)
{
    while (*src != '\0')
    {
        *dest = *src;

        dest++;
        src++;
    }


    *dest = '\0';
}

static void String_Append(
    char *dest,
    const char *src
)
{
    while (*dest != '\0')
    {
        dest++;
    }
    while (*src != '\0')
    {
        *dest = *src;

        dest++;
        src++;
    }


    *dest = '\0';
}
static void UInt_To_String(
    uint32_t number,
    char *str
)
{
    char temp[12];

    uint32_t i = 0;
    uint32_t j;
    if (number == 0)
    {
        str[0] = '0';
        str[1] = '\0';

        return;
    }
    while (number > 0)
    {
        temp[i] =
            (char)('0' + (number % 10));

        number /= 10;

        i++;
    }
    for (j = 0; j < i; j++)
    {
        str[j] =
            temp[i - j - 1];
    }


    str[i] = '\0';
}
static void Make_Message(void)
{
    char number_string[12];
    tx_buffer[0] = '\0';
    String_Copy(
        tx_buffer,
        MA_LOP
    );
    String_Append(
        tx_buffer,
        "-"
    );

    String_Append(
        tx_buffer,
        MA_NHOM
    );
    String_Append(
        tx_buffer,
        "-BTN:"
    );
    UInt_To_String(
        button_count,
        number_string
    );
    String_Append(
        tx_buffer,
        number_string
    );
    String_Append(
        tx_buffer,
        "\r\n"
    );
}
static void USART1_DMA_Send(
    char *data,
    uint16_t length
)
{
    if (dma_busy != 0)
    {
        return;
    }

    dma_busy = 1;
    DMA_Cmd(
        DMA1_Channel4,
        DISABLE
    );
    DMA1_Channel4->CMAR =
        (uint32_t)data;
    DMA1_Channel4->CNDTR =
        length;
    DMA1->IFCR =
        DMA_IFCR_CGIF4 |
        DMA_IFCR_CTCIF4 |
        DMA_IFCR_CHTIF4 |
        DMA_IFCR_CTEIF4;
    DMA_Cmd(
        DMA1_Channel4,
        ENABLE
    );
}

static void DMA_CheckComplete(void)
{
    if ((DMA1->ISR & DMA_ISR_TCIF4) != 0)
    {
        DMA1->IFCR =
            DMA_IFCR_CTCIF4;

        DMA_Cmd(
            DMA1_Channel4,
            DISABLE
        );

        dma_busy = 0;
    }
}

static void Delay_ms(
    uint32_t ms
)
{
    volatile uint32_t i;
    volatile uint32_t j;


    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 8000; j++)
        {
            __asm volatile ("nop");
        }
    }
}

int main(void)
{
    uint8_t button_old;
    uint8_t button_new;

    button_old = 1;

    GPIO_Config();
    USART1_Config();
    DMA_USART1_Config();
    String_Copy(
        tx_buffer,
        "BAI 3 - DMA UART READY\r\n"
    );


    USART1_DMA_Send(
        tx_buffer,
        String_Length(tx_buffer)
    );

    while (1)
    {
        DMA_CheckComplete();

        button_new =
            GPIO_ReadInputDataBit(
                GPIOA,
                GPIO_Pin_0
            );

        if ((button_old == 1) &&
            (button_new == 0))
        {
            button_count++;

            Make_Message();
            USART1_DMA_Send(
                tx_buffer,
                String_Length(tx_buffer)
            );

            Delay_ms(30);
        }

        button_old =
            button_new;
    }
}
