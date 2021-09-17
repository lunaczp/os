##################################################
# Makefile
##################################################

BOOT:=boot.asm
LDR:=loader.asm
KERNEL:=kernel.asm
STRING:=string.asm
KLIBA:=kliba.asm
START:=start.c
BOOT_BIN:=$(subst .asm,.bin,$(BOOT))
LDR_BIN:=$(subst .asm,.bin,$(LDR))
KERNEL_BIN:=$(subst .asm,.bin,$(KERNEL))


IMG:=../../../util/mdisk/a.img
FLOPPY:=/mnt/floppy/

.PHONY : everything

everything : $(BOOT_BIN) $(LDR_BIN) $(KERNEL_BIN)
	dd if=$(BOOT_BIN) of=$(IMG) bs=512 count=1 conv=notrunc
	mount -t msdos -o loop $(IMG) $(FLOPPY)
	cp $(LDR_BIN) $(FLOPPY) -v
	cp $(KERNEL_BIN) $(FLOPPY) -v
	umount $(FLOPPY)

clean :
	rm -f $(BOOT_BIN) $(LDR_BIN) $(KERNEL_BIN) *.o

$(BOOT_BIN) : $(BOOT)
	nasm $< -o $@

$(LDR_BIN) : $(LDR)
	nasm $< -o $@

$(KERNEL_BIN) : $(KERNEL) $(STRING) $(KLIBA)
	nasm -f elf32 -o $(subst .asm,.o,$(KERNEL)) $(KERNEL)
	nasm -f elf32 -o $(subst .asm,.o,$(STRING)) $(STRING)
	nasm -f elf32 -o $(subst .asm,.o,$(KLIBA)) $(KLIBA)
	gcc -c -m32 -fno-builtin -o $(subst .c,.o,$(START)) $(START)
	ld -melf_i386 -s -Ttext 0x30400 -o $@ $(subst .asm,.o,$(KERNEL)) $(subst .asm,.o,$(STRING)) $(subst .asm,.o,$(KLIBA)) $(subst .c,.o,$(START))