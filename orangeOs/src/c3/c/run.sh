nasm pmtest3.asm -o pm.com
nasm pmtest3_privilege_violate.asm -o pm_p_v.com

[ ! -d /media/floppy ] && mkdir /media/floppy
mount -o loop ../../../util/mdisk/pm.img /media/floppy
cp *.com /media/floppy/
umount /media/floppy
