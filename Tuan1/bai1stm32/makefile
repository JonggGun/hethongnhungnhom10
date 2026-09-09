TARGET = main

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -Wall -ffreestanding -nostdlib
LDFLAGS = -T linker.ld -nostdlib

all: $(TARGET).bin

startup.o: startup.s
	$(CC) $(CFLAGS) -c startup.s -o startup.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET).elf: startup.o main.o linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) startup.o main.o -o $(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

clean:
	rm -f *.o *.elf *.bin

flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $(TARGET).bin 0x08000000 verify reset exit"
