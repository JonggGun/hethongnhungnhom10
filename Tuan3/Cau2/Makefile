CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m3 -mthumb -Wall -O2
LDFLAGS = -TSTM32F103C8Tx_FLASH.ld -nostdlib

SRC = main.c max7219.c
ASM_SRC = startup_stm32f103c8tx.s

OBJ = $(SRC:.c=.o) $(ASM_SRC:.s=.o)

all: firmware.bin firmware.hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

firmware.elf: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

firmware.hex: firmware.elf
	$(OBJCOPY) -O ihex $< $@

flash: firmware.bin
	st-flash write firmware.bin 0x8000000

clean:
	rm -f *.o *.elf *.bin *.hex
