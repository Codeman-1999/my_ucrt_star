
CROSS_COMPILE = riscv64-unknown-elf-
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -g

GDB = riscv64-unknown-elf-gdb
# riscv64-unknown-elf-gcc 工具链可以同时编译汇编和 C 代码
CC = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump

SRCS_ASM = \
	entry.S \
	kerneltrap.S \
	switch.S \
	link_app.S \

SRCS_C = \
	sbi.c \
	main.c \
	printk.c \
	trap.c \
	syscall.c \
	string.c \
	app.c \
	timer.c \
	stack.c \
	address.c \
	task.c \
	assert.c \
	vsprintf.c \
	printf.c \
	loader.c \
	virtio_disk.c \
	bio.c \

INCLUDE:=-I./include
SRC:=./src
LIB:=./lib
# 将源文件替换为 .o 文件
OBJS = $(SRCS_ASM:.S=.o)
OBJS += $(SRCS_C:.c=.o)


all: build_app os.elf

os.elf: ${OBJS}
	${CC} ${CFLAGS} -T os.ld -Wl,-Map=os.map -o os.elf $^
	${OBJCOPY} -O binary os.elf os.bin

%.o : $(SRC)/%.c
	${CC} ${CFLAGS} $(INCLUDE) -c -o $@ $<

%.o : $(LIB)/%.c
	${CC} ${CFLAGS} $(INCLUDE) -c -o $@ $<

%.o : $(SRC)/%.S
	${CC} ${CFLAGS} $(INCLUDE) -c -o $@ $<

build_app: build.c
	@$(MAKE) -C ./user
	@gcc $< -o build.out
	./build.out

.PHONY : debug
debug: 
	@echo "os debug start..."
	@${GDB} os.elf -q -x ./gdbinit

.PHONY : clean
clean:
	rm -rf *.o *.bin *.out *.map *.elf
	rm -rf ./user/bin/*

