##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=pmtest5.asm
BIN:=$(subst .asm,.com,$(SRC))

.PHONY : everything

everything : $(BIN)
	mount -o loop ../../../util/mdisk/pm.img /media/floppy
	cp -v $(BIN) /media/floppy/
	umount /media/floppy


$(BIN) : $(SRC)
	nasm $< -o $@