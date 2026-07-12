# Makefile for the IEEE Sign ARGB program (ATmega32U2)
# Uses the open-source AVR toolchain: avr-gcc, avr-objcopy, avrdude.

# ---- Project configuration -------------------------------------------------
TARGET   = ieee-sign
MCU      = atmega32u2
F_CPU    = 16000000UL

# C and assembly sources
CSRC     = main.c
ASRC     = timing.S

# ---- Toolchain -------------------------------------------------------------
CC       = avr-gcc
OBJCOPY  = avr-objcopy
OBJDUMP  = avr-objdump
SIZE     = avr-size
AVRDUDE  = avrdude

# ---- Flags -----------------------------------------------------------------
CFLAGS   = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99 \
           -Wall -ffunction-sections -fdata-sections \
           -funsigned-char -funsigned-bitfields

ASFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -x assembler-with-cpp

# -lm pulls in libm for floor/sin/fmod/powf; gc-sections drops dead code
LDFLAGS  = -mmcu=$(MCU) -Wl,--gc-sections -Wl,-Map,$(TARGET).map
LDLIBS   = -lm

# ---- Programmer configuration ----------------------------------------------
# Default: an STK500v2-style ISP programmer (avrdude "avrispv2"). ISP is used
# for both flashing and fuse programming.
#
# A serial avrispv2 needs its port (-P). Defaults to /dev/ttyUSB0; override for
# other hardware, e.g.:
#   make flash PORT=/dev/ttyACM0
#   make flash PROGRAMMER=usbasp PORT=usb
#   make flash PROGRAMMER=avrispmkII PORT=usb
# (The 32U2's factory USB DFU bootloader is PROGRAMMER=flip1, but it cannot
#  read or write fuses, so `make fuses` needs a real ISP programmer.)
PROGRAMMER = avrispv2
PORT       = /dev/ttyUSB0
AVRDUDE_PORT = $(if $(PORT),-P $(PORT),)
AVRDUDE_FLAGS = -p m32u2 -c $(PROGRAMMER) $(AVRDUDE_PORT)

# Target fuse values -- match the previous build setup: 16 MHz external crystal.
#   lfuse 0xEF  = full-swing crystal oscillator, slow rising power
#   hfuse 0xD9  = SPIEN, default bootsize, BOOTRST off
LFUSE      = 0xef
HFUSE      = 0xd9

# Seconds to let a serial programmer settle between avrdude invocations.
# Rapid reconnects to a USB-serial avrispv2 (e.g. CH340 on /dev/ttyUSB0) can
# leave it mid-reset and make the next fuse write fail verification.
FUSE_SETTLE = 1

# ---- Build artifacts -------------------------------------------------------
OBJ      = $(CSRC:.c=.o) $(ASRC:.S=.o)
ELF      = $(TARGET).elf
HEX      = $(TARGET).hex
EEP      = $(TARGET).eep
LSS      = $(TARGET).lss

# ---- Build rules -----------------------------------------------------------
.PHONY: all size flash fuses clean help
.DEFAULT_GOAL := all

all: $(HEX) $(LSS) size

$(ELF): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

# Flash image (Intel HEX)
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock $< $@

# EEPROM image (only produced if there is EEPROM data)
$(EEP): $(ELF)
	$(OBJCOPY) -O ihex -j .eeprom \
	  --set-section-flags=.eeprom="alloc,load" \
	  --change-section-lma .eeprom=0 --no-change-warnings $< $@ || true

# Disassembly listing (handy for debugging the cycle-counted asm)
$(LSS): $(ELF)
	$(OBJDUMP) -h -S $< > $@

size: $(ELF)
	@echo
	@$(SIZE) --mcu=$(MCU) --format=avr $(ELF)

# ---- Flashing --------------------------------------------------------------
# Ensure fuses are correct, then program the flash.
flash: $(HEX) fuses
	@sleep $(FUSE_SETTLE)
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(HEX):i

# Read the current fuses and write only the ones that differ from the target
# values. Leaves the chip untouched if both already match.
#
# Both fuses are read in ONE avrdude invocation, and any writes happen in ONE
# more invocation, with a settle delay in between -- this avoids the rapid
# reconnects that made a serial avrispv2 drop the fuse write/verify. hfuse is
# written before lfuse so the clock-select fuse (lfuse) is programmed last.
fuses:
	@fr=$$($(AVRDUDE) $(AVRDUDE_FLAGS) -qq -U lfuse:r:-:h -U hfuse:r:-:h 2>/dev/null | tr 'A-F' 'a-f' | tr -d '\r'); \
	lo=$$(printf '%s\n' "$$fr" | sed -n 1p); \
	hi=$$(printf '%s\n' "$$fr" | sed -n 2p); \
	want_lo=$$(echo $(LFUSE) | tr 'A-F' 'a-f'); \
	want_hi=$$(echo $(HFUSE) | tr 'A-F' 'a-f'); \
	if [ -z "$$lo" ] || [ -z "$$hi" ]; then \
	  echo "fuses: could not read fuses (is the programmer connected on PORT=$(PORT)?)"; exit 1; \
	fi; \
	echo "fuses: current lfuse=$$lo hfuse=$$hi | target lfuse=$$want_lo hfuse=$$want_hi"; \
	ops=""; \
	[ "$$hi" != "$$want_hi" ] && ops="$$ops -U hfuse:w:$(HFUSE):m"; \
	[ "$$lo" != "$$want_lo" ] && ops="$$ops -U lfuse:w:$(LFUSE):m"; \
	if [ -z "$$ops" ]; then \
	  echo "fuses: already correct, nothing to write."; \
	else \
	  sleep $(FUSE_SETTLE); \
	  echo "fuses: updating ->$$ops"; \
	  $(AVRDUDE) $(AVRDUDE_FLAGS) $$ops; \
	fi

# ---- Housekeeping ----------------------------------------------------------
clean:
	rm -f $(OBJ) $(ELF) $(HEX) $(EEP) $(LSS) $(TARGET).map

help:
	@echo "Targets:"
	@echo "  make            Build $(HEX) and print size (default)"
	@echo "  make flash      Check/fix fuses, then flash via \$$(PROGRAMMER) [default: avrispv2]"
	@echo "  make fuses      Read fuses; write lfuse=$(LFUSE)/hfuse=$(HFUSE) only if they differ"
	@echo "  make clean      Remove build artifacts"
	@echo
	@echo "Override the programmer/port, e.g.:"
	@echo "  make flash PROGRAMMER=usbasp"
	@echo "  make flash PROGRAMMER=avrispmkII PORT=usb"
	@echo "  make flash PORT=/dev/ttyACM0"
