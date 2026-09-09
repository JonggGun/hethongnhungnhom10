TARGET = stm32_bai4

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = -mcpu=cortex-m3 \
         -mthumb \
         -O0 \
         -Wall \
         -Wextra \
         -ffreestanding \
         -fno-builtin

LDFLAGS = -Tlinker.ld \
          -nostdlib \
          -nostartfiles \
          -nodefaultlibs \
          -Wl,--gc-sections \
          -mcpu=cortex-m3 \
          -mthumb

SRC = src/main.c src/startup.c

OBJ = $(SRC:.c=.o)


all: $(TARGET).elf $(TARGET).bin $(TARGET).hex


$(TARGET).elf: $(OBJ) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@


$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@


$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@


size: $(TARGET).elf
	$(SIZE) $(TARGET).elf


clean:
	rm -f $(OBJ)
	rm -f $(TARGET).elf
	rm -f $(TARGET).bin
	rm -f $(TARGET).hex


flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000
