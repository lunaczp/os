##################################################
# Makefile
##################################################

BOOT:=boot.asm
LDR:=loader.asm
BOOT_BIN:=$(subst .asm,.bin,$(BOOT))
LDR_BIN:=$(subst .asm,.bin,$(LDR))

.PHONY : everything

everything : $(BOOT_BIN) $(LDR_BIN)
	dd if=$(BOOT_BIN) of=../../../util/mdisk/a.img bs=512 count=1 conv=notrunc
	mount -o loop ../../../util/mdisk/a.img /media/floppy
	cp ${LDR_BIN} /media/floppy/
	umount /media/floppy

clean :
	rm -f $(BOOT_BIN) $(LDR_BIN)

$(BOOT_BIN) : $(BOOT)
	nasm $< -o $@

$(LDR_BIN) : $(LDR)
	nasm $< -o $@

