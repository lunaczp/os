nasm -f elf64 hello.asm -o hello.o
ld -s hello.o -o hello