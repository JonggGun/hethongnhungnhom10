TARGET = stm32_uart

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = cortex-m3

CFLAGS = -mcpu=$(CPU)
CFLAGS += -mthumb
CFLAGS += -mfloat-abi=soft
CFLAGS += -O0
CFLAGS += -g3
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -ffreestanding
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += -DSTM32F103xB

CFLAGS += -I.
CFLAGS += -Isrc
CFLAGS += -I$(HOME)/STM32CubeF1/Drivers/CMSIS/Include
CFLAGS += -I$(HOME)/STM32CubeF1/Drivers/CMSIS/Device/ST/STM32F1xx/Include
CFLAGS += -I$(HOME)/STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc

LDFLAGS = -Tlinker.ld
LDFLAGS += -mcpu=$(CPU)
LDFLAGS += -mthumb
LDFLAGS += -mfloat-abi=soft
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += --specs=nano.specs
LDFLAGS += --specs=nosys.specs

HAL_DIR = $(HOME)/STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Src

SRCS = \
src/main.c \
system_stm32f1xx.c \
$(HAL_DIR)/stm32f1xx_hal.c \
$(HAL_DIR)/stm32f1xx_hal_rcc.c \
$(HAL_DIR)/stm32f1xx_hal_gpio.c \
$(HAL_DIR)/stm32f1xx_hal_uart.c \
$(HAL_DIR)/stm32f1xx_hal_dma.c \
$(HAL_DIR)/stm32f1xx_hal_flash.c \
$(HAL_DIR)/stm32f1xx_hal_cortex.c

STARTUP = startup_stm32f103xb.s

OBJS = $(SRCS:.c=.o)
OBJS += $(STARTUP:.s=.o)

.PHONY: all clean flash

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

flash: $(TARGET).bin
	STM32_Programmer_CLI -c port=SWD -w $(TARGET).bin 0x08000000 -v -rst

clean:
	rm -f $(OBJS)
	rm -f $(TARGET).elf
	rm -f $(TARGET).bin
	rm -f $(TARGET).hex
	rm -f $(TARGET).map
