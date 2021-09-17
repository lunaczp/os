nasm pmtest5.asm -o pm5.com
nasm pmtest5a.asm -o pm5a.com
nasm pmtest5b.asm -o pm5b.com
nasm pmtest5c.asm -o pm5c.com

[ ! -d /media/floppy ] && mkdir /media/floppy
mount -o loop ../../../util/mdisk/pm.img /media/floppy
cp *.com /media/floppy/
umount /media/floppy
