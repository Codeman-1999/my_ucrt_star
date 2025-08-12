#!/bin/bash
# 获取当前脚本文件所在的目录
SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)

if [ ! -d "$SHELL_FOLDER/output" ]; then  
mkdir $SHELL_FOLDER/output
fi  

cd qemu-8.0.2

# 清理之前的编译
make clean

# 设置调试编译选项
export CFLAGS="-g -O0 -DDEBUG"
export CXXFLAGS="-g -O0 -DDEBUG"

# 配置QEMU编译选项，启用调试符号
./configure \
    --prefix=$SHELL_FOLDER/output/qemu \
    --target-list=riscv64-softmmu \
    --enable-gtk \
    --enable-virtfs \
    --disable-gio \
    --enable-debug \
    --enable-debug-info \
    --disable-strip \
    --disable-werror \
    --enable-debug-tcg \
    --enable-debug-mutex

# 编译QEMU (使用-j8以免过载系统)
echo "开始编译QEMU调试版本..."
make -j8 V=1
make install

# 验证调试符号
echo "检查QEMU调试符号..."
if objdump -h $SHELL_FOLDER/output/qemu/bin/qemu-system-riscv64 | grep -q debug; then
    echo "✓ QEMU包含调试符号"
else
    echo "✗ 警告: QEMU可能缺少调试符号"
fi

cd ..

# 编译底层引导程序(保持原有逻辑)
CROSS_PREFIX=riscv64-unknown-elf
if [ ! -d "$SHELL_FOLDER/output/lowlevelboot" ]; then  
mkdir $SHELL_FOLDER/output/lowlevelboot
fi  

cd boot
$CROSS_PREFIX-gcc -x assembler-with-cpp -c start.s -o $SHELL_FOLDER/output/lowlevelboot/start.o
$CROSS_PREFIX-gcc -nostartfiles -T./boot.lds -Wl,-Map=$SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.map -Wl,--gc-sections $SHELL_FOLDER/output/lowlevelboot/start.o -o $SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.elf
$CROSS_PREFIX-objcopy -O binary -S $SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.elf $SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.bin
$CROSS_PREFIX-objdump --source --demangle --disassemble --reloc --wide $SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.elf > $SHELL_FOLDER/output/lowlevelboot/lowlevel_fw.lst

cd $SHELL_FOLDER/output/lowlevelboot
rm -rf fw.bin
dd of=fw.bin bs=1k count=32k if=/dev/zero
dd of=fw.bin bs=1k conv=notrunc seek=0 if=lowlevel_fw.bin

echo ""
echo "==================="
echo "调试版本QEMU编译完成！"
echo "QEMU位置: $SHELL_FOLDER/output/qemu/bin/qemu-system-riscv64"
echo ""
echo "VSCode调试步骤："
echo "1. 在VSCode中打开 quard_star.c"
echo "2. 在第237行设置断点"
echo "3. 按F5选择 'QEMU Debug - quard_star.c'"
echo "4. 或者先运行 ./debug-qemu-vscode.sh 然后附加调试"
echo "===================" 