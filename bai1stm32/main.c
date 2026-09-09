#include <stdint.h>

#define RCC_BASE        0x40021000
#define RCC_APB2ENR     *(volatile uint32_t *)(RCC_BASE + 0x18)

#define GPIOC_BASE      0x40011000
#define GPIOC_CRH       *(volatile uint32_t *)(GPIOC_BASE + 0x04)
#define GPIOC_ODR       *(volatile uint32_t *)(GPIOC_BASE + 0x0C)

// Hàm tạo độ trễ theo miligiây (tương đối cho xung clock HSI 8MHz mặc định)
void delay_ms(uint32_t ms) {
    // Với 8MHz, khoảng 1333 vòng lặp tương đương với 1ms
    volatile uint32_t count = ms * 1333;
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    // 1. Bật Clock cho GPIOC
    RCC_APB2ENR |= (1 << 4);

    // 2. Cấu hình PC13 làm Output Push-Pull (2MHz)
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |= (0x2 << 20);

    uint32_t blink_delay_ms = 500; 

    while (1) {
        GPIOC_ODR |= (1 << 13);
        delay_ms(blink_delay_ms); 

        GPIOC_ODR &= ~(1 << 13);
        delay_ms(blink_delay_ms); 
    }

    return 0;
}
