all:
	mkdir -p build/
	as boot.s -o build/boot.o
	ld -Ttext 0x7C00 -o build/boot.elf build/boot.o
	objcopy -O binary build/boot.elf build/boot.bin
	qemu-system-x86_64 build/boot.bin

clean:
	rm -rf build/

