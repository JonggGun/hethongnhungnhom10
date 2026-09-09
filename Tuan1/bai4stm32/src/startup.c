#include <stdint.h>

/* Symbol được linker cung cấp */
extern uint32_t _estack;

/* Hàm main */
extern int main(void);

/* Reset Handler */
void Reset_Handler(void);

/* Vector table */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] =
{
    (uint32_t)&_estack,       // Initial Stack Pointer
    (uint32_t)Reset_Handler   // Reset Handler
};

/* Khi STM32 khởi động */
void Reset_Handler(void)
{
    main();

    /* Không cho chương trình chạy ra ngoài main */
    while (1)
    {
    }
}
