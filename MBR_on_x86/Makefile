ASM_FILES := $(shell ls *.s)
OBJ_FILES := $(patsubst %.s, build/%.o, $(ASM_FILES))

LINKED_ELF := build/boot.elf
LINKED_BIN := build/boot.bin


build: $(LINKED_BIN)

$(OBJ_FILES) : $(ASM_FILES)
	@echo $@
	mkdir -p $(dir $@)
	as -g $(patsubst build/%.o, %.s, $@) -o $@

$(LINKED_ELF): $(OBJ_FILES)
	mkdir -p $(dir $@)
	ld -Tlinker.ld $(OBJ_FILES) -o $(LINKED_ELF)

$(LINKED_BIN): $(LINKED_ELF)
	mkdir -p $(dir $@)
	objcopy -O binary $(LINKED_ELF) $(LINKED_BIN)

and_run: build run

run:
	qemu-system-x86_64 build/boot.bin

debug:
	qemu-system-x86_64 -s -S build/boot.bin &
	gdb build/boot.elf

clean:
	rm -rf build/

