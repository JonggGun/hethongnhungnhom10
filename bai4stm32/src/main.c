#include <stdint.h>

/* =========================
   RCC
   ========================= */
#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018UL)

/* =========================
   GPIOA - Nút nhấn PA0
   ========================= */
#define GPIOA_CRL   (*(volatile uint32_t *)0x40010800UL)
#define GPIOA_IDR   (*(volatile uint32_t *)0x40010808UL)
#define GPIOA_ODR   (*(volatile uint32_t *)0x4001080CUL)

/* =========================
   GPIOC - LED PC13
   ========================= */
#define GPIOC_CRH   (*(volatile uint32_t *)0x40011004UL)
#define GPIOC_ODR   (*(volatile uint32_t *)0x4001100CUL)

/* Delay chống dội phím */
void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile ("nop");
    }
}

int main(void)
{
    /* =========================
       1. Bật clock GPIOA + GPIOC
       ========================= */
    RCC_APB2ENR |= (1 << 2);   // GPIOA
    RCC_APB2ENR |= (1 << 4);   // GPIOC


    /* =========================
       2. PA0 = Input Pull-up
       ========================= */

    GPIOA_CRL &= ~(0xF << 0);
    GPIOA_CRL |=  (0x8 << 0);

    /* Bật điện trở kéo lên */
    GPIOA_ODR |= (1 << 0);


    /* =========================
       3. PC13 = Output Push-Pull 10MHz
       ========================= */

    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x1 << 20);


    /* =========================
       4. LED ban đầu TẮT
       ========================= */

    GPIOC_ODR &= ~(1 << 13);

    /* Trạng thái LED */
    uint8_t led_state = 0;


    /* =========================
       5. Vòng lặp chính
       ========================= */

    while (1)
    {
        /* -------------------------
           Chờ nhấn nút
           PA0 = 0 khi nhấn
           ------------------------- */
        if ((GPIOA_IDR & (1 << 0)) == 0)
        {
            /* Chống dội khi nhấn */
            delay(50000);

            /* Xác nhận vẫn đang nhấn */
            if ((GPIOA_IDR & (1 << 0)) == 0)
            {
                /*
                 * KHÔNG đổi LED ở đây.
                 *
                 * Chờ người dùng THẢ nút.
                 */
                while ((GPIOA_IDR & (1 << 0)) == 0)
                {
                }

                /* Chống dội khi thả */
                delay(50000);

                /*
                 * Đến đây nghĩa là:
                 *
                 * ĐÃ NHẤN
                 *      +
                 * ĐÃ THẢ
                 *
                 * → mới đổi trạng thái LED
                 */

                led_state = !led_state;

                if (led_state)
                {
                    /* LED ON */
                    GPIOC_ODR |= (1 << 13);
                }
                else
                {
                    /* LED OFF */
                    GPIOC_ODR &= ~(1 << 13);
                }
            }
        }
    }
}
