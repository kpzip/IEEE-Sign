# IEEE Sign — ATmega32U2 ARGB program

WS2812B ("ARGB") LED strip animations for the ATmega32U2 (16 MHz).

## Toolchain

Open-source AVR tools only:

- `avr-gcc` / `avr-libc` — compiler + device headers
- `avr-binutils` — `avr-objcopy`, `avr-objdump`, `avr-size`
- `avrdude` — flashing
- `make`

On Arch: `sudo pacman -S avr-gcc avr-libc avrdude make`

## Build

```sh
make            # compiles main.c + timing.S -> ieee-sign.hex, prints size
make clean      # removes build artifacts
make help       # list targets / flashing options
```

Output: `ieee-sign.hex` (flash image). `ieee-sign.lss` is a disassembly
listing, useful for checking the cycle-counted bit-banging in `timing.S`.

## Flash

Flashing uses an ISP programmer, defaulting to `avrispv2` (an STK500v2-style
programmer) on `PORT=/dev/ttyUSB0`. Connect the programmer to the board's ISP
header, then:

```sh
make flash
```

`make flash` first checks the fuses (writing them only if they're wrong,
see below) and then programs the flash.

Override the programmer or port for your hardware:

```sh
make flash PORT=/dev/ttyACM0
make flash PROGRAMMER=usbasp PORT=usb
make flash PROGRAMMER=avrispmkII PORT=usb
```

> The 32U2 also has a factory USB DFU bootloader (`PROGRAMMER=flip1`), but it
> cannot read or write fuses, so fuse handling requires a real ISP programmer.

### Fuses

`make fuses` **reads** the current fuses and writes the target values only
for the byte(s) that differ — if both already match, it writes nothing:

```sh
make fuses
```

Target values (from the previous build setup, for a 16 MHz external crystal):

- `lfuse=0xEF` — full-swing crystal oscillator, slow rising power
- `hfuse=0xD9` — SPIEN, default bootsize, BOOTRST off

`make flash` runs this check automatically, so fuses are kept correct on
every flash. To change the targets, override `LFUSE`/`HFUSE`:

```sh
make fuses LFUSE=0xef HFUSE=0xd9
```

This replaces the raw commands the old system used
(`avrdude ... -U hfuse:w:0xd9:m` / `-U lfuse:w:0xef:m`), but only writes when
a fuse is actually out of spec.

Both fuses are read in a single avrdude call, and any writes happen in one
more call after a short settle delay (`FUSE_SETTLE`, default 1 s). This
matters for a serial avrispv2 (e.g. a CH340 on `/dev/ttyUSB0`): rapid
back-to-back avrdude invocations can catch the programmer mid-reset and make
the fuse write fail to verify. If you still see verify errors, try a longer
delay, `make flash FUSE_SETTLE=2`.

## Notes on changes made to build with a modern toolchain

The sources came from Atmel Studio; two changes were needed to compile
with current `avr-gcc` (16.x):

- `main.c`: added `#include <math.h>` — the code uses `floor`, `fmod`,
  `sin`, `powf`, and `M_PI`, which are now hard errors if undeclared.
- `timing.S`: removed the redundant `.equ PORTB`/`.equ SREG` lines; those
  names already come from `<avr/io.h>` (and resolve to the same IO
  addresses via `#define __SFR_OFFSET 0`).
