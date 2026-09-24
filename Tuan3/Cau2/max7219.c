#include "max7219.h"

#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800

#define RCC_APB2ENR     *(volatile uint32_t *)(RCC_BASE + 0x18)
#define GPIOA_CRL       *(volatile uint32_t *)(GPIOA_BASE + 0x00)
#define GPIOA_BSRR      *(volatile uint32_t *)(GPIOA_BASE + 0x10)

// Cấu hình chân PA4 (CS), PA5 (CLK), PA7 (DIN)
#define CS_HIGH()       (GPIOA_BSRR = (1 << 4))
#define CS_LOW()        (GPIOA_BSRR = (1 << 20))

#define CLK_HIGH()      (GPIOA_BSRR = (1 << 5))
#define CLK_LOW()       (GPIOA_BSRR = (1 << 21))

#define DIN_HIGH()      (GPIOA_BSRR = (1 << 7))
#define DIN_LOW()       (GPIOA_BSRR = (1 << 23))

static void delay_us(volatile uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 8; i++) {
        __asm__("nop");
    }
}

void SPI1_Init(void) {
    // Bật clock cho GPIOA
    RCC_APB2ENR |= (1 << 2);

    // Cấu hình PA4, PA5, PA7 làm General Output Push-Pull 10MHz
    GPIOA_CRL &= ~((0xF << 16) | (0xF << 20) | (0xF << 28));
    GPIOA_CRL |=  (0x1 << 16) | (0x1 << 20) | (0x1 << 28);

    CS_HIGH();
    CLK_LOW();
    DIN_LOW();
}

static void BitBang_SendByte(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        CLK_LOW();
        if (byte & (1 << i)) {
            DIN_HIGH();
        } else {
            DIN_LOW();
        }
        delay_us(2);
        CLK_HIGH();
        delay_us(2);
    }
}

void MAX7219_Write(uint8_t address, uint8_t data) {
    CS_LOW();
    delay_us(2);

    BitBang_SendByte(address);
    BitBang_SendByte(data);

    delay_us(2);
    CS_HIGH();
    delay_us(2);
}

void MAX7219_Init(void) {
    SPI1_Init();
    delay_us(100000); // Tạm dừng 100ms cho IC ổn định nguồn

    MAX7219_Write(MAX7219_REG_DISPLAYTEST, 0x00); // Tắt Test mode
    MAX7219_Write(MAX7219_REG_SCANLIMIT, 0x07);   // Quét 8 hàng
    MAX7219_Write(MAX7219_REG_DECODEMODE, 0x00);  // Tắt Decode mode
    MAX7219_Write(MAX7219_REG_INTENSITY, 0x03);   // Chỉnh độ sáng mức 3

    // Xóa sạch bộ nhớ hiển thị (tắt hết các LED)
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_Write(i, 0x00);
    }

    MAX7219_Write(MAX7219_REG_SHUTDOWN, 0x01);    // Bật màn hình
}

void MAX7219_DrawMatrix(const uint8_t *pattern) {
    for (int i = 0; i < 8; i++) {
        // Gửi trực tiếp byte dữ liệu chuẩn (không dùng dấu ~)
        MAX7219_Write(i + 1, pattern[i]); 
    }
}
