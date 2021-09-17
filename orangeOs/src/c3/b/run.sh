nasm pmtest_old.asm -o old.com
nasm pmtest_screen.asm -o screen.com
nasm pmtest_zhp.asm -o zhp.com

[ ! -d /media/floppy ] && mkdir /media/floppy
mount -o loop ../../../util/mdisk/pm.img /media/floppy
cp *.com /media/floppy/
umount /media/floppy
