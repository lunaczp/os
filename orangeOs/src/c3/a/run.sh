nasm pmtest1.asm -o pmtest1.bin
dd if=pmtest1.bin of=a.img bs=512 count=1 conv=notrunc
