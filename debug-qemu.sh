#!/bin/bash

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
QEMU_BIN=$SHELL_FOLDER/output/qemu/bin/qemu-system-riscv64

# 检查QEMU是否存在
if [ ! -f "$QEMU_BIN" ]; then
    echo "错误: QEMU二进制文件不存在: $QEMU_BIN"
    echo "请先运行 ./build-debug.sh 编译调试版本"
    exit 1
fi

# 检查固件是否存在
if [ ! -f "$SHELL_FOLDER/output/lowlevelboot/fw.bin" ]; then
    echo "错误: 固件文件不存在，请先运行 ./build-debug.sh"
    exit 1
fi

# 检查调试符号
if ! objdump -h "$QEMU_BIN" | grep -q debug; then
    echo "警告: QEMU可能没有调试符号，建议重新编译"
    echo "运行: ./build-debug.sh"
fi

# 创建GDB配置文件
cat > /tmp/qemu_debug.gdb << 'EOF'
# 设置GDB配置
set confirm off
set verbose off
set history save on
set history filename ~/.gdb_history
set print pretty on
set print array on
set breakpoint pending on

# 设置断点在关键函数上
break quard_star_machine_init
break quard_star_cpu_create
break quard_star_memory_create
break quard_star_flash_create
break main

# 显示所有断点
info breakpoints

# 提示信息
echo \n=== QEMU GDB调试会话 ===\n
echo 已设置断点:\n
echo - main (程序入口)\n
echo - quard_star_machine_init (机器初始化)\n  
echo - quard_star_cpu_create (CPU创建)\n
echo - quard_star_memory_create (内存创建)\n
echo - quard_star_flash_create (Flash创建)\n
echo \n常用GDB命令:\n
echo - 'c' 继续执行\n
echo - 'n' 下一行\n
echo - 's' 进入函数\n
echo - 'bt' 查看调用栈\n
echo - 'p variable_name' 打印变量\n
echo - 'info locals' 查看局部变量\n
echo - 'info registers' 查看寄存器\n
echo ========================\n

# 运行程序
run
EOF

echo "启动QEMU GDB调试会话..."
echo "QEMU命令行参数:"
echo "  -M quard-star -m 1G -smp 8 -bios none"
echo "  -drive if=pflash,bus=0,unit=0,format=raw,file=fw.bin"
echo "  -monitor stdio"
echo ""
echo "GDB将自动设置断点并启动QEMU"
echo ""

# 启动GDB调试
gdb \
    -x /tmp/qemu_debug.gdb \
    --args $QEMU_BIN \
    -M quard-star \
    -m 1G \
    -smp 8 \
    -bios none \
    -drive if=pflash,bus=0,unit=0,format=raw,file=$SHELL_FOLDER/output/lowlevelboot/fw.bin \
    -monitor stdio

# 清理临时文件
rm -f /tmp/qemu_debug.gdb

echo "GDB调试会话结束" 