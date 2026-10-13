/* =====================================================================
 * BAI TAP 01 - STM32F103 + FreeRTOS
 *  - 3 LED bam xung PWM o tan so 1kHz (TIM2 CH1/CH2/CH3 -> PA0/PA1/PA2)
 *  - 3 nut nhan: PB12 = chuyen LED, PB13 = tang do sang, PB14 = giam do sang
 *  - 3 task LED doc lap + 1 task doc nut nhan
 *  - Task nut nhan gui du lieu sang 3 task LED bang Queue (xQueueOverwrite)
 *
 *  Moi truong: Keil MDK, ARM Compiler 6 | Thu vien: STM32F10x SPL (STD)
 *  Port FreeRTOS: portable/GCC/ARM_CM3 (tuong thich AC6)
 * ===================================================================== */

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ---------------- Tham so chung ---------------- */
#define LED_COUNT     3
#define DUTY_MAX      100u
#define DUTY_STEP     10u

/* ---------------- Nut nhan (pull-up trong, muc thap = dang nhan) ---------------- */
#define BTN_SEL_PIN   GPIO_Pin_12   /* PB12: chuyen LED dang dieu khien */
#define BTN_UP_PIN    GPIO_Pin_13   /* PB13: tang do sang LED dang chon */
#define BTN_DOWN_PIN  GPIO_Pin_14   /* PB14: giam do sang LED dang chon */

/* ---------------- Cau hinh PWM 1kHz cho 3 LED ----------------
 * TIM2CLK = 72MHz (APB1 = 36MHz, timer duoc x2).
 * PSC = 71, ARR = 999  =>  72MHz / (72 * 1000) = 1000Hz = 1kHz
 * Do phan giai duty: CCR = duty(%) * 10  (0..1000) */
static void PWM_LED_Init(void)
{
    GPIO_InitTypeDef      gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef     oc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PA0, PA1, PA2: chan PWM (Alternate Function Push-Pull) */
    gpio.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    TIM_TimeBaseStructInit(&tb);
    tb.TIM_Period        = 999;
    tb.TIM_Prescaler     = 71;
    tb.TIM_ClockDivision = 0;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode       = TIM_OCMode_PWM1;
    oc.TIM_OutputState  = TIM_OutputState_Enable;
    oc.TIM_Pulse        = 0;                 /* ban dau tat */
    oc.TIM_OCPolarity   = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &oc);                  /* PA0 */
    TIM_OC2Init(TIM2, &oc);                  /* PA1 */
    TIM_OC3Init(TIM2, &oc);                  /* PA2 */

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    TIM_Cmd(TIM2, ENABLE);
}

/* duty: 0..100 (%). LED 0 -> CH1, LED 1 -> CH2, LED 2 -> CH3 */
static void LED_SetDuty(uint8_t idx, uint8_t duty)
{
    uint16_t ccr = (uint16_t)duty * 10u;
    if (ccr > 1000u) ccr = 1000u;

    switch (idx)
    {
        case 0:  TIM_SetCompare1(TIM2, ccr); break;
        case 1:  TIM_SetCompare2(TIM2, ccr); break;
        case 2:  TIM_SetCompare3(TIM2, ccr); break;
        default: break;
    }
}

/* ---------------- Cau hinh nut nhan ---------------- */
static void Buttons_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin  = BTN_SEL_PIN | BTN_UP_PIN | BTN_DOWN_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;          /* input pull-up, khong can tro ngoai */
    GPIO_Init(GPIOB, &gpio);
}

/* Tra ve 1 neu nut dang duoc nhan (muc thap) */
static uint8_t BtnPressed(uint16_t pin)
{
    return (GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET) ? 1u : 0u;
}

/* ---------------- FreeRTOS: Queue ----------------
 * Moi task LED so huu 1 queue rieng (do dai 1, kieu uint8_t = duty 0..100).
 * Task nut nhan dung xQueueOverwrite de luon gui gia tri moi nhat. */
static QueueHandle_t xLedQueue[LED_COUNT];

/* ---------------- Task LED (3 task doc lap) ----------------
 * Chi doi du lieu moi tren queue cua minh roi cap nhat PWM. */
static void vTask_LED(void *pvParameters)
{
    uint8_t idx  = (uint8_t)(uint32_t)pvParameters;   /* 0, 1, 2 */
    uint8_t duty;

    for (;;)
    {
        if (xQueueReceive(xLedQueue[idx], &duty, portMAX_DELAY) == pdPASS)
        {
            LED_SetDuty(idx, duty);
        }
    }
}

/* ---------------- Task nut nhan ----------------
 * Doc 3 nut (chong doi phim + bat suon xuong), giu trang thai:
 *  selected: LED nao dang duoc dieu khien (0..2)
 *  duty[3]:  do sang hien tai cua tung LED
 * Moi thay doi -> gui ngay sang queue cua LED tuong ung. */
static void vTask_Button(void *pvParameters)
{
    (void)pvParameters;

    const uint16_t pins[3] = { BTN_SEL_PIN, BTN_UP_PIN, BTN_DOWN_PIN };
    uint8_t prev[3] = { 0u, 0u, 0u };
    uint8_t selected = 0;
    uint8_t duty[LED_COUNT] = { 50u, 50u, 50u };   /* do sang ban dau 50% */
    uint8_t i;

    /* Gui gia tri ban dau cho ca 3 task LED */
    for (i = 0; i < LED_COUNT; i++)
    {
        xQueueOverwrite(xLedQueue[i], &duty[i]);
    }

    for (;;)
    {
        for (i = 0; i < 3; i++)
        {
            uint8_t pressed = BtnPressed(pins[i]);

            /* Phat hien suon xuong (vua nhan) + chong doi phim */
            if (pressed && !prev[i])
            {
                vTaskDelay(pdMS_TO_TICKS(30));
                if (BtnPressed(pins[i]))          /* van con nhan sau 30ms -> phim that */
                {
                    if (i == 0)
                    {
                        /* Chuyen LED: nhap nhay 2 lan de bao dang chon LED nay */
                        uint8_t zero = 0u;
                        uint8_t keep;
                        selected = (selected + 1u) % LED_COUNT;
                        keep = duty[selected];
                        xQueueOverwrite(xLedQueue[selected], &zero);
                        vTaskDelay(pdMS_TO_TICKS(120));
                        xQueueOverwrite(xLedQueue[selected], &keep);
                        vTaskDelay(pdMS_TO_TICKS(120));
                        xQueueOverwrite(xLedQueue[selected], &zero);
                        vTaskDelay(pdMS_TO_TICKS(120));
                        xQueueOverwrite(xLedQueue[selected], &keep);
                    }
                    else if (i == 1)
                    {
                        /* Tang do sang */
                        if (duty[selected] + DUTY_STEP <= DUTY_MAX)
                            duty[selected] += DUTY_STEP;
                        else
                            duty[selected] = DUTY_MAX;
                        xQueueOverwrite(xLedQueue[selected], &duty[selected]);
                    }
                    else
                    {
                        /* Giam do sang */
                        if (duty[selected] >= DUTY_STEP)
                            duty[selected] -= DUTY_STEP;
                        else
                            duty[selected] = 0u;
                        xQueueOverwrite(xLedQueue[selected], &duty[selected]);
                    }
                }
            }
            prev[i] = pressed;
        }
        vTaskDelay(pdMS_TO_TICKS(20));            /* quet nut moi 20ms */
    }
}

/* ---------------- Hook bao loi (giu don gian: dung he thong) ---------------- */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* ---------------- main ---------------- */
int main(void)
{
    uint8_t i;

    SystemInit();        /* clock 72MHz (startup SPL cung da goi, goi lai vo hai) */
    PWM_LED_Init();
    Buttons_Init();

    /* Tao 3 queue, moi queue 1 phan tu uint8_t */
    for (i = 0; i < LED_COUNT; i++)
    {
        xLedQueue[i] = xQueueCreate(1, sizeof(uint8_t));
        configASSERT(xLedQueue[i] != NULL);
    }

    /* 3 task LED doc lap, tham so la chi so LED 0/1/2 */
    xTaskCreate(vTask_LED, "LED0", 128, (void *)(uint32_t)0, 2, NULL);
    xTaskCreate(vTask_LED, "LED1", 128, (void *)(uint32_t)1, 2, NULL);
    xTaskCreate(vTask_LED, "LED2", 128, (void *)(uint32_t)2, 2, NULL);

    /* 1 task doc nut nhan, uu tien cao hon de nut phan hoi nhanh */
    xTaskCreate(vTask_Button, "Button", 256, NULL, 3, NULL);

    vTaskStartScheduler();

    for (;;);            /* khong bao gio toi day */
}
