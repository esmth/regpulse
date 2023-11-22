.PHONY: main
#MCU = attiny3217
MCU = attiny1616
CFLAGS = -mmcu=$(MCU) -fno-lto -Os -g -DF_CPU=20000000UL
LDFLAGS = -Wl,-Map=main.map -Wl,--section-start=.lookup=0x3000
SPECS = -B ../avrlibc/atpack/gcc/dev/$(MCU) -isystem ../avrlibc/atpack/include

all: main
main:
	avr-gcc $(SPECS) $(CFLAGS) $(LDFLAGS) -o main.elf main.c

size: main
	avr-size main.elf

lst: main
	avr-objdump -d -S main.elf > main.lst

hex: main
	avr-objcopy -I elf32-avr -O ihex main.elf main.hex

bin: hex
	avr-objcopy -I ihex -O binary main.hex main.bin

clean:
	rm -f main.elf main.lst main.hex main.bin

burn: hex
	avrdude -c atmelice_updi -p $(MCU) -U flash:w:main.hex

burnmnt: hex
	sudo mount /dev/sda /mnt
	sudo cp main.hex /mnt/.
	sudo umount /mnt
