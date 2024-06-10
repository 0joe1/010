BUILD    := build
KERNEL   := kernel
DEVICE 	 := device
LIBK     := lib/kernel
LIBU     := lib/usr
THREAD   := thread
USERPROG := userprog
FS       := fs

CC 		= gcc
LIB     = -I lib/ -I $(LIBK)/ -I $(LIBU) -I $(KERNEL) -I $(DEVICE) -I $(THREAD) \
		  -I $(USERPROG) -I $(FS) -I she

#LDFALGS := -Ttext 0xc0001500 -e main
ASFLAGS = -f elf -w-orphan-label -g
CFLAGS  = -Wall $(LIB) -c -fno-builtin -m32 -fno-stack-protector \
		  -W -Wstrict-prototypes -Wmissing-prototypes -g
GDB_SETTING = gdbstub:enabled=1,port=1234,text_base=0,data_base=0,bss_base=0
GDB_CONNECT = "target remote localhost:1234"

OBJS_BIN=mbr.bin loader.bin $(BUILD)/kernel.bin

#image: ${OBJS_BIN} 定义了一个名为image的目标，依赖于OBJS_BIN，
#表示要生成一个名为boot.img的磁盘映像文件，并将mbr.bin写入该磁盘映像中。
image: ${OBJS_BIN}
#创建一个大小为30MB的全0磁盘映像文件。
	#dd if=/dev/zero of=boot.img bs=512 count=61440
#mbr.bin文件写入boot.img磁盘映像文件的第0个扇区。
	dd if=mbr.bin of=boot.img bs=512 count=1 conv=notrunc
#loader.bin文件写入boot.img磁盘映像文件的第2个扇区。
	dd if=loader.bin of=boot.img bs=512 count=3 seek=2 conv=notrunc
#kernel.bin文件写入boot.img磁盘映像文件的第9个扇区。
	dd if=$(BUILD)/kernel.bin of=boot.img bs=512 count=200 seek=9 conv=notrunc
#创建 hd80M.img
	cp hd80M.bak6 hd80M.img
#@-表示忽略删除过程中出现的错误。
	@-rm -rf *.bin	
  
OBJS_O = $(BUILD)/main.o $(BUILD)/init.o $(BUILD)/interrupt.o\
		 $(BUILD)/print.o $(BUILD)/kernel.o $(BUILD)/timer.o \
		 $(BUILD)/debug.o $(BUILD)/bitmap.o $(BUILD)/memory.o \
		 $(BUILD)/string.o $(BUILD)/switch.o $(BUILD)/list.o \
		 $(BUILD)/sync.o $(BUILD)/console.o $(BUILD)/thread.o \
		 $(BUILD)/keyboard.o $(BUILD)/ioqueue.o $(BUILD)/tss.o \
		 $(BUILD)/process.o  $(BUILD)/syscall-init.o $(BUILD)/she.o\
		 $(BUILD)/stdio.o $(BUILD)/ide.o $(BUILD)/stdio-kernel.o \
		 $(BUILD)/fs.o $(BUILD)/file.o $(BUILD)/inode.o $(BUILD)/dir.o \
		 $(BUILD)/fork.o  $(BUILD)/syscall.o $(BUILD)/buildin_cmd.o \
		 $(BUILD)/assert.o $(BUILD)/exec.o $(BUILD)/wait_exit.o \
		 $(BUILD)/pipe.o

################# 汇编代码编译 ####################
%.bin:%.asm
	nasm -I include/ $^ -o $@
$(BUILD)/%.o: $(LIBK)/%.S
	nasm $(ASFLAGS) $^ -o $@
$(BUILD)/%.o: $(KERNEL)/%.S
	nasm $(ASFLAGS) $^ -o $@
$(BUILD)/%.o: $(THREAD)/%.S
	nasm $(ASFLAGS) $^ -o $@


################# C代码编译 ####################
$(BUILD)/main.o: kernel/main.c lib/kernel/print.h \
				 lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h \
				 lib/stdint.h kernel/interrupt.h device/timer.h \
				 kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
					  lib/stdint.h kernel/global.h lib/kernel/io.h \
					  lib/kernel/print.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/timer.o: device/timer.c device/timer.h lib/stdint.h \
				  lib/kernel/io.h lib/kernel/print.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/debug.o: kernel/debug.c kernel/debug.h \
				  lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/string.o: lib/string.c lib/string.h \
				   kernel/debug.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/bitmap.o: kernel/bitmap.c kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/memory.o: kernel/memory.c kernel/memory.h lib/kernel/print.h\
				   kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/thread.o: thread/thread.c thread/thread.h kernel/memory.h \
				   lib/string.h lib/stdint.h kernel/interrupt.h \
				   lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/list.o: lib/kernel/list.c lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/sync.o: thread/sync.c thread/sync.h lib/stdint.h \
				 lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/console.o: device/console.c device/console.h \
					lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/keyboard.o: device/keyboard.c device/keyboard.h lib/kernel/io.h \
					 lib/kernel/print.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ioqueue.o: device/ioqueue.c device/ioqueue.h lib/stdint.h thread/thread.h \
					thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/tss.o: userprog/tss.c userprog/tss.h kernel/global.h lib/kernel/print.h \
				thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/process.o: userprog/process.c userprog/process.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/syscall-init.o: userprog/syscall-init.c lib/usr/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/syscall.o: lib/usr/syscall.c lib/usr/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/stdio.o: lib/stdio.c lib/stdio.h kernel/debug.h lib/string.h \
				  lib/usr/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/assert.o: lib/usr/assert.c lib/usr/assert.h lib/stdio.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ide.o: device/ide.c device/ide.h thread/sync.h lib/stdio.h \
				kernel/global.h kernel/debug.h lib/kernel/io.h \
				kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/stdio-kernel.o: lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fs.o: fs/fs.c fs/fs.h kernel/global.h kernel/debug.h device/ide.h \
			   kernel/memory.h fs/dir.h fs/inode.h userprog/syscall-init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/inode.o: fs/inode.c fs/inode.h fs/super_block.h fs/fs.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/file.o: fs/file.c fs/file.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/dir.o: fs/dir.c fs/dir.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fork.o: userprog/fork.c userprog/fork.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/she.o: she/shell.c she/shell.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/buildin_cmd.o: she/buildin_cmd.c she/buildin_cmd.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/exec.o: userprog/exec.c userprog/exec.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/wait_exit.o: userprog/wait_exit.c userprog/wait_exit.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pipe.o:she/pipe.c she/pipe.h
	$(CC) $(CFLAGS) $< -o $@


################ 链接所有目标文件 ###################
$(BUILD)/kernel.bin: $(OBJS_O)
	ld -m elf_i386 -T kernel.ld $(OBJS_O)  -o $(BUILD)/kernel.bin

gdb_symbol:
	objcopy --only-keep-debug $(BUILD)/kernel.bin $(BUILD)/kernel.sym

build: $(BUILD)/kernel.bin

#定义了一个名为run的目标
run:build
	make image
	/home/username/bochs2.6/bin/bochs -f bochsrc.disk

run_gdb:build
	make gdb_symbol
	make image
	/home/username/bochs-gdb/bin/bochs -qf bochsrc.disk	$(GDB_SETTING) 
	gdb $(BUILD)/kernel.bin  -ex $(GDB_CONNECT)


clean:
	rm -rf  *.out *.lock *.bin *.o kernel/*.o lib/kernel/*.o $(BUILD)/*.o
.PHONY:clean image build gdb_symbol
