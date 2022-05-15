ASM_FILES := realmode_main.s boot.s
OBJ_FILES := $(patsubst %.s, build/%.o, $(ASM_FILES))

LINKED_ELF := build/boot.elf
LINKED_BIN := build/boot.bin


build: $(LINKED_BIN)

mkdir_build:
	mkdir -p build/

$(OBJ_FILES) : mkdir_build $(ASM_FILES)
	$(AS) -g $(patsubst build/%.o, %.s, $@) -o $@

$(LINKED_ELF): $(OBJ_FILES)
	$(LD) -Tlinker.ld $(OBJ_FILES) -o $(LINKED_ELF)

$(LINKED_BIN): $(LINKED_ELF)
	objcopy -O binary $(LINKED_ELF) $(LINKED_BIN)

and_run: build run

run:
	qemu-system-x86_64 -drive file=build/boot.bin,format=raw

debug:
	qemu-system-x86_64 -s -S -drive file=build/boot.bin,format=raw &
	gdb build/boot.elf \
		-ex 'target remote localhost:1234' \
        -ex 'break *0x7C00' \
        -ex 'continue'

and_debug: build debug

clean:
	rm -rf build/

