# Makefile for compiling and flashing AVR code

# Compiler
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Compiler flags
CFLAGS = -mmcu=atmega644p -DF_CPU=12000000 -Wall -Os

# Source files
SRCS = main.c font.c lcd.c ili934x.c functions.c 
# Header files
HDRS = font.h lcd.h ili934x.h functions.h

# Output files
TARGET = main

all: $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).elf: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $^ -o $@

flash: $(TARGET).hex
	$(AVRDUDE) -c usbasp -p m644p -U flash:w:$<

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all flash clean

