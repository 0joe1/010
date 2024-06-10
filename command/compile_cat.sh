if [[ ! -d "../lib" || ! -d "../build" ]];then
    echo "dependent dir don't exist!"
    cwd=$(pwd)
    cwd=$(cwd##*/)
    if [[ $cwd != "command" ]];then
        echo -e "you'd better in command dir\n"
    fi
    exit
fi

BIN="cat"
CFLAGS="-Wall -c -fno-builtin -m32 -fno-stack-protector \
          -W -Wstrict-prototypes -Wmissing-prototypes -g"
LIB="-I ../lib -I ../fs -I ../device -I ../thread -I ../kernel -I ../lib/usr"
OBJS="../build/string.o ../build/syscall.o \
      ../build/stdio.o  ../build/assert.o "

DD_IN=$BIN
DD_OUT="/home/username/Desktop/Os/elephant/010/boot.img"

nasm -f elf ./start.S -o ./start.o
ar rcs simple_crt.a $OBJS ./start.o
gcc $CFLAGS $LIB -I "../lib/kernel" -o $BIN".o" $BIN".c"
ld -e _start -m elf_i386 -T program.ld $BIN".o" simple_crt.a -o $BIN
SEC_CNT=$(ls -l $BIN|awk '{printf("%d", ($5+511)/512)}')

if [[ -f $BIN ]];then
    dd if=./$DD_IN of=$DD_OUT bs=512 \
    count=$SEC_CNT seek=300 conv=notrunc
fi

