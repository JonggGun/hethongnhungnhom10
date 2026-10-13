/* stm32f10x_conf.h - cau hinh thu vien STD (chi include driver dang dung) */
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "misc.h"

/* Tat kiem tra tham so cua SPL (khong dung USE_FULL_ASSERT) */
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line);
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

#endif /* __STM32F10x_CONF_H */
