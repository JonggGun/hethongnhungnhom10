#include <stdint.h>

#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800

#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x18)))
#define GPIOA_CRL       (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_ODR       (*((volatile uint32_t *)(GPIOA_BASE + 0x0C)))

void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    // 1. Bật clock cho PORTA
    RCC_APB2ENR |= (1 << 2);

    // 2. Cấu hình CẢ 8 CHÂN PA0-PA7 làm General Purpose Output Push-Pull 10MHz
    // Đảm bảo là giá trị 0x11111111UL (kiểu unsigned long 32-bit đủ 8 nibble)
    GPIOA_CRL = 0x11111111UL;

    int8_t position = 0;
    int8_t direction = 1;

    while (1) {
        // Xuất tín hiệu ra PA0 - PA7
        GPIOA_ODR = (GPIOA_ODR & ~0xFF) | (1 << position);

        delay(200000);

        position += direction;

        if (position == 7) {
            direction = -1;
        } else if (position == 0) {
            direction = 1;
        }
    }

    return 0;
}
