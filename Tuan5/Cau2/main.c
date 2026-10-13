#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

// Khai báo Queue Handle
QueueHandle_t xCountQueue;

// ---------------------------------------------------------
// Hàm g?i 1 chu?i ký t? qua UART1
// ---------------------------------------------------------
void UART1_SendString(char* str) {
    while(*str) {
        // Ch? c? TXE (Transmit Data Register Empty) b?t lên
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        // G?i ký t?
        USART_SendData(USART1, *str++);
    }
}

// ---------------------------------------------------------
// C?u hình UART1 (Baudrate 115200, TX-PA9, RX-PA10)
// ---------------------------------------------------------
void UART1_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // C?p xung nh?p cho UART1 và GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // C?u hình chân TX (PA9) là Alternate Function Push-Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // C?u hình chân RX (PA10) là Floating Input (Bài này không b?t bu?c nhung nên c?u hình)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // C?u hình thông s? UART1
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; 
    
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

// ---------------------------------------------------------
// C?u hình Ng?t ngoài EXTI cho nút nh?n (Chân PA0)
// ---------------------------------------------------------
void EXTI0_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // C?p xung nh?p cho GPIOA và AFIO (b?t bu?c khi dùng ng?t ngoài)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // C?u hình chân PA0 là Input Pull-up (di?n tr? kéo lên)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // K?t n?i EXTI Line0 v?i chân PA0
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    // C?u hình EXTI Line0: ng?t c?nh xu?ng (Falling Edge)
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // C?u hình NVIC (B? di?u khi?n ng?t)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); // Phân nhóm uu tiên (yêu c?u c?a FreeRTOS)
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6; // Ph?i l?n hon ho?c b?ng 5 (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// ---------------------------------------------------------
// Trình ph?c v? ng?t ngoài EXTI0 (ISR)
// ---------------------------------------------------------
void EXTI0_IRQHandler(void) {
    // Ki?m tra dúng là ng?t t? Line 0
    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        static uint32_t press_count = 0;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        press_count++; // Tang bi?n d?m

        // G?i giá tr? s? d?m vào Queue t? ISR
        xQueueSendFromISR(xCountQueue, &press_count, &xHigherPriorityTaskWoken);

        // Xóa c? ng?t d? không b? nh?y vào ng?t liên t?c
        EXTI_ClearITPendingBit(EXTI_Line0);

        // Yêu c?u chuy?n ng? c?nh ngay l?p t?c sang Task nh?n (n?u Task nh?n có m?c uu tiên cao hon)
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// ---------------------------------------------------------
// Task truy?n d? li?u UART
// ---------------------------------------------------------
void vUartTask(void *pvParameters) {
    uint32_t received_count = 0;
    char tx_buffer[50];

    for(;;) {
        // Ðua Task v? tr?ng thái Blocked d? ch? d? li?u t? Queue (ch? vô t?n portMAX_DELAY)
        if(xQueueReceive(xCountQueue, &received_count, portMAX_DELAY) == pdPASS) {
            
            // Nh?n giá tr? d?m và d?nh d?ng chu?i ký t? hi?n th?
            sprintf(tx_buffer, "So lan nhan nut: %lu\r\n", received_count);
            
            // Truy?n chu?i ký t? qua UART1 lên máy tính
            UART1_SendString(tx_buffer);
        }
    }
}

// ---------------------------------------------------------
// Hàm Main
// ---------------------------------------------------------
int main(void) {
    // 1. C?u hình các ngo?i vi
    UART1_Config();
    EXTI0_Config();

    // 2. T?o Queue ch?a t?i da 10 ph?n t?, m?i ph?n t? luu giá tr? ki?u uint32_t
    xCountQueue = xQueueCreate(10, sizeof(uint32_t));

    // 3. T?o Task truy?n UART
    if (xCountQueue != NULL) {
        xTaskCreate(vUartTask, "UART_Task", 128, NULL, 1, NULL);
    }

    // 4. Kh?i d?ng b? l?p l?ch c?a FreeRTOS
    vTaskStartScheduler();

    // Vòng l?p này ch? ch?y d?n khi h? th?ng không d? RAM c?p phát cho FreeRTOS lúc kh?i d?ng
    while(1) {
    }
}