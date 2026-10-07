#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

/* Gói tham số: chân LED + tần số nháy */
typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t      GPIO_Pin;
    float         freq_hz;
} LedBlinkParam_t;

/* 01 hàm điều khiển nháy LED, chạy như một FreeRTOS task.
   Đầu vào: chân LED (port + pin) và tần số nháy (Hz),
   được gói trong struct LedBlinkParam_t truyền qua pvParameters. */
void vTask_LedBlink(void *pvParameters)
{
    LedBlinkParam_t *px = (LedBlinkParam_t *)pvParameters;
    /* Nửa chu kỳ (ms): mỗi lần delay xong thì đảo trạng thái LED một lần.
       f = 0.1Hz -> 5000ms, f = 1Hz -> 500ms, f = 10Hz -> 50ms */
    TickType_t xHalfPeriod = pdMS_TO_TICKS((uint32_t)(500.0f / px->freq_hz));

    for (;;) {
        GPIO_WriteBit(px->GPIOx, px->GPIO_Pin,
            (BitAction)(1 - GPIO_ReadOutputDataBit(px->GPIOx, px->GPIO_Pin)));
        vTaskDelay(xHalfPeriod);
    }
}

static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);  /* PA4 = 0V: làm "đất giả" cho 3 LED */
}

int main(void)
{
    /* Tham số cho 3 task: chân LED + tần số độc lập.
       Khai báo static để vùng nhớ tồn tại suốt thời gian task chạy. */
    static LedBlinkParam_t xLed1 = { GPIOA, GPIO_Pin_1, 0.1f };  /* 0.1 Hz */
    static LedBlinkParam_t xLed2 = { GPIOA, GPIO_Pin_2, 1.0f };  /* 1 Hz   */
    static LedBlinkParam_t xLed3 = { GPIOA, GPIO_Pin_3, 10.0f }; /* 10 Hz  */

    GPIO_Config();

    /* Tạo 3 task từ cùng 01 hàm vTask_LedBlink, khác nhau ở tham số */
    xTaskCreate(vTask_LedBlink, "LED_0.1Hz", configMINIMAL_STACK_SIZE,
                &xLed1, 1, NULL);
    xTaskCreate(vTask_LedBlink, "LED_1Hz", configMINIMAL_STACK_SIZE,
                &xLed2, 1, NULL);
    xTaskCreate(vTask_LedBlink, "LED_10Hz", configMINIMAL_STACK_SIZE,
                &xLed3, 1, NULL);

    /* Chạy scheduler - từ đây FreeRTOS tiếp quản CPU */
    vTaskStartScheduler();

    for (;;);   /* Không bao giờ tới đây nếu scheduler chạy đúng */
}
