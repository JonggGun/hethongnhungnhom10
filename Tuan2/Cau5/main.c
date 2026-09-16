/*
 * Bai tap 05 - HTN'26 Bare-Metal Week-02
 * STM32F103 (vd: Blue Pill STM32F103C8T6)
 *
 * - PA0  = TIM2_CH1 (PWM) -> noi LED qua tro han dong
 * - PA9  = USART1_TX
 * - PA10 = USART1_RX
 *
 * Lenh dieu khien qua UART (115200-8-N-1), moi lenh ket thuc bang '!':
 *   ON!            -> bat LED, sang theo muc PWM da cau hinh gan nhat
 *   OFF!           -> tat LED (khong doi cau hinh PWM da luu)
 *   PWM:<0-100>!   -> cau hinh do sang (%). Neu dang OFF thi chi luu cau hinh,
 *                     khong doi do sang thuc te cho den khi co lenh ON!
 *   Status!        -> tra ve trang thai hien tai, vd: "LED:ON,PWM:70%"
 *
 * Khong dung HAL/StdPeriph: viet thang thanh ghi (bare-metal), dung
 * arm-none-eabi-gcc + Makefile, nap bang st-flash/openocd, khong qua IDE.
 */

#include <stdint.h>
#include "stm32f103.h"

#define CMD_BUF_LEN 32

/* Bo dem nhan tu ngat UART */
static volatile char    rx_buf[CMD_BUF_LEN];
static volatile uint8_t rx_index = 0;
static volatile uint8_t cmd_ready = 0;
static char cmd_buf[CMD_BUF_LEN];

/* Trang thai LED / PWM */
static volatile uint8_t  led_on = 0;
static volatile uint16_t pwm_percent = 0; /* 0-100, cau hinh gan nhat */

/* SYSCLK mac dinh sau reset = HSI = 8MHz (khong cau hinh PLL cho don gian) */
#define TIM2_PSC 79u   /* 8MHz / (79+1) = 100kHz */
#define TIM2_ARR 99u   /* 100kHz / (99+1) = 1kHz PWM */

static void gpio_init(void);
static void tim2_pwm_init(void);
static void usart1_init(void);
static void usart_send_char(char c);
static void usart_send_string(const char *s);
static void update_pwm_output(void);
static void process_command(char *cmd);
static uint16_t parse_uint(const char *s);
static int str_starts_with(const char *s, const char *prefix);

int main(void)
{
    gpio_init();
    tim2_pwm_init();
    usart1_init();

    usart_send_string("STM32F103 Ready\r\n");

    while (1)
    {
        if (cmd_ready)
        {
            cmd_ready = 0;
            process_command(cmd_buf);
        }
    }
}

/* ===================== GPIO ===================== */
static void gpio_init(void)
{
    /* Bat clock GPIOA + AFIO */
    RCC->APB2ENR |= (1u << 2) | (1u << 0); /* IOPAEN, AFIOEN */

    /* PA0 = TIM2_CH1, Alternate Function Push-Pull, 50MHz
       CRL[3:0]: MODE0=11 (output 50MHz), CNF0=10 (AF push-pull) -> 0b1011 */
    GPIOA->CRL &= ~(0xFu << (0 * 4));
    GPIOA->CRL |=  (0xBu << (0 * 4));

    /* PA9 = USART1_TX, AF push-pull 50MHz */
    GPIOA->CRH &= ~(0xFu << ((9 - 8) * 4));
    GPIOA->CRH |=  (0xBu << ((9 - 8) * 4));

    /* PA10 = USART1_RX, input floating (CNF=01, MODE=00 -> 0b0100) */
    GPIOA->CRH &= ~(0xFu << ((10 - 8) * 4));
    GPIOA->CRH |=  (0x4u << ((10 - 8) * 4));
}

/* ===================== TIM2 PWM (kenh 1, PA0) ===================== */
static void tim2_pwm_init(void)
{
    RCC->APB1ENR |= (1u << 0); /* TIM2EN */

    TIM2->PSC = TIM2_PSC;
    TIM2->ARR = TIM2_ARR;

    /* Kenh 1: CC1S=00 (output), OC1M=110 (PWM mode 1), OC1PE=1 (preload) */
    TIM2->CCMR1 &= ~(0x7u << 4);
    TIM2->CCMR1 |=  (0x6u << 4);
    TIM2->CCMR1 |=  (1u << 3);

    TIM2->CCER |= (1u << 0); /* CC1E: bat ngo ra kenh 1 */

    TIM2->CCR1 = 0; /* bat dau o trang thai tat */

    TIM2->CR1 |= (1u << 7); /* ARPE */
    TIM2->EGR |= (1u << 0); /* UG: nap lai thanh ghi ngay */

    TIM2->CR1 |= (1u << 0); /* CEN: chay counter */
}

/* ===================== USART1 ===================== */
static void usart1_init(void)
{
    RCC->APB2ENR |= (1u << 14); /* USART1EN */

    /* Baudrate 115200, PCLK2 = 8MHz (mac dinh, chua chinh prescaler)
       USARTDIV = 8 000 000 / (16 * 115200) = 4.34
       Mantissa = 4, Fraction = round(0.34*16) = 5 -> BRR = (4<<4)|5 */
    USART1->BRR = (4u << 4) | 5u;

    USART1->CR1 |= (1u << 13); /* UE : bat USART */
    USART1->CR1 |= (1u << 3);  /* TE : bat truyen */
    USART1->CR1 |= (1u << 2);  /* RE : bat nhan */
    USART1->CR1 |= (1u << 5);  /* RXNEIE : ngat khi co du lieu nhan */

    /* USART1_IRQn = 37 -> nam trong ISER[1], bit (37-32)=5 */
    NVIC->ISER[1] |= (1u << (37 - 32));
}

static void usart_send_char(char c)
{
    while (!(USART1->SR & (1u << 7))) { /* cho TXE */ }
    USART1->DR = (uint8_t)c;
}

static void usart_send_string(const char *s)
{
    while (*s)
    {
        usart_send_char(*s++);
    }
}

/* ===================== Xu ly PWM/LED ===================== */
static void update_pwm_output(void)
{
    if (led_on)
    {
        uint32_t ccr = ((uint32_t)pwm_percent * (TIM2_ARR + 1u)) / 100u;
        TIM2->CCR1 = ccr;
    }
    else
    {
        TIM2->CCR1 = 0;
    }
}

/* ===================== Ham tien ich xu ly chuoi ===================== */
static uint16_t parse_uint(const char *s)
{
    uint16_t val = 0;
    while (*s >= '0' && *s <= '9')
    {
        val = (uint16_t)(val * 10 + (*s - '0'));
        s++;
    }
    return val;
}

static int str_starts_with(const char *s, const char *prefix)
{
    while (*prefix)
    {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

/* ===================== Parser lenh ===================== */
static void process_command(char *cmd)
{
    int len = 0;
    while (cmd[len]) len++;
    if (len > 0 && cmd[len - 1] == '!') cmd[len - 1] = '\0';

    if (str_starts_with(cmd, "ON"))
    {
        led_on = 1;
        update_pwm_output();
        usart_send_string("OK:ON\r\n");
    }
    else if (str_starts_with(cmd, "OFF"))
    {
        led_on = 0;
        update_pwm_output();
        usart_send_string("OK:OFF\r\n");
    }
    else if (str_starts_with(cmd, "PWM:"))
    {
        uint16_t val = parse_uint(cmd + 4);
        if (val > 100) val = 100;
        pwm_percent = val;
        update_pwm_output(); /* neu dang OFF, ham nay se khong bat sang thuc */
        usart_send_string("OK:PWM\r\n");
    }
    else if (str_starts_with(cmd, "Status"))
    {
        char msg[48];
        int i = 0;
        const char *p1 = "LED:";
        while (*p1) msg[i++] = *p1++;

        const char *st = led_on ? "ON" : "OFF";
        while (*st) msg[i++] = *st++;

        msg[i++] = ',';

        const char *p2 = "PWM:";
        while (*p2) msg[i++] = *p2++;

        uint16_t v = pwm_percent;
        char digits[4];
        int d = 0;
        if (v == 0)
        {
            digits[d++] = '0';
        }
        while (v > 0)
        {
            digits[d++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (d > 0) msg[i++] = digits[--d];

        msg[i++] = '%';
        msg[i++] = '\r';
        msg[i++] = '\n';
        msg[i]   = '\0';

        usart_send_string(msg);
    }
    else
    {
        usart_send_string("ERR:UNKNOWN\r\n");
    }
}

/* ===================== ISR USART1 ===================== */
void USART1_IRQHandler(void)
{
    if (USART1->SR & (1u << 5)) /* RXNE: co du lieu moi */
    {
        char c = (char)(USART1->DR & 0xFFu);

        if (c == '!')
        {
            if (rx_index < CMD_BUF_LEN) rx_buf[rx_index] = '!';
            rx_index++;

            uint8_t n = (rx_index < CMD_BUF_LEN) ? rx_index : (CMD_BUF_LEN - 1);
            for (uint8_t i = 0; i < n; i++)
            {
                cmd_buf[i] = rx_buf[i];
            }
            cmd_buf[n] = '\0';

            rx_index = 0;
            cmd_ready = 1;
        }
        else if (c == '\r' || c == '\n')
        {
            /* bo qua ky tu xuong dong */
        }
        else
        {
            if (rx_index < (CMD_BUF_LEN - 1))
            {
                rx_buf[rx_index++] = c;
            }
            else
            {
                rx_index = 0; /* tran bo dem -> reset */
            }
        }
    }
}
