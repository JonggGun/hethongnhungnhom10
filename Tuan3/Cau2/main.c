#include <stdint.h>
#include "max7219.h"

// Mã bitmap chuẩn chữ A
const uint8_t char_A[8] = {
    0b00011000,
    0b00111100,
    0b01100110,
    0b01100110,
    0b01111110,
    0b01100110,
    0b01100110,
    0b00000000
};

int main(void) {
    MAX7219_Init();

    while (1) {
        MAX7219_DrawMatrix(char_A);
    }

    return 0;
}
