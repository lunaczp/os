nasm pmtest9.asm -o pm9.com
nasm pmtest9a.asm -o pm9a.com
nasm pmtest9b.asm -o pm9b.com
nasm pmtest9c.asm -o pm9c.com

[ ! -d /media/floppy ] && mkdir /media/floppy
mount -o loop ../../../util/mdisk/pm.img /media/floppy
cp *.com /media/floppy/
umount /media/floppy