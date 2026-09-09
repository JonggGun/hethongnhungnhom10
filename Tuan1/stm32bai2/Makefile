CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g -Wall
LDFLAGS = -T stm32f103c8.ld -nostdlib

all: main.bin

main.elf: startup.o main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.bin: main.elf
	arm-none-eabi-objcopy -O binary $< $@

clean:
	rm -f *.o *.elf *.bin

flash: main.bin
	st-flash write main.bin 0x08000000

.PHONY: all clean flash
