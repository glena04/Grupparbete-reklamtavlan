CC=avr-g++
LD=avr-ld
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
AVRSIZE=avr-size
OBJISP=avrdude
MCU=atmega328p
CFLAGS=-Wall -Wextra -Wundef -pedantic \
        -Os -DF_CPU=16000000UL -mmcu=${MCU} -DBAUD=19200
LDFLAGS=-mmcu=$(MCU)
BIN=avrdemo
OUT=${BIN}.hex
SOURCES=main.cpp lcd.cpp uart.cpp

# OS detection and settings
ifeq ($(OS),Windows_NT)
    PORT=\\\\.\\COM3
    RM=del /Q
    RMDIR=rmdir /S /Q
    MKDIR=mkdir
else
    PORT=/dev/tty.usbserial
    RM=rm -f
    RMDIR=rm -rf
    MKDIR=mkdir -p
endif

DEBUG?=1

ifeq ($(DEBUG), 1)
    OUTPUTDIR=bin/debug
else
    OUTPUTDIR=bin/release
endif

OBJS=$(addprefix $(OUTPUTDIR)/,$(SOURCES:.cpp=.o))

all: $(OUTPUTDIR) $(OUT)

$(OBJS): Makefile

$(OUTPUTDIR)/%.o: %.cpp | $(OUTPUTDIR)
    $(CC) $(CFLAGS) -MD -o $@ -c $<

%.lss: %.elf
    $(OBJDUMP) -h -S -s $< > $@

%.elf: $(OBJS)
    $(CC) -Wl,-Map=$(@:.elf=.map) $(LDFLAGS) -o $@ $^
    $(AVRSIZE) $@

$(OBJS): $(SOURCES)

%.hex: %.elf
    $(OBJCOPY) -O ihex -R .fuse -R .lock -R .user_signatures -R .comment $< $@

isp: ${BIN}.hex
    $(OBJISP) -F -V -c arduino -p ${MCU} -P ${PORT} -U flash:w:$<

clean:
    $(RM) "$(OUT)" *.map *.P *.d ${BIN}.elf ${BIN}.lss
    $(RMDIR) bin

$(OUTPUTDIR):
    @$(MKDIR) "$(OUTPUTDIR)"

.PHONY: clean all isp