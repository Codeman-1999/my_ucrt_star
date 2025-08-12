# my_ucrt_star

## 介绍

从0基于qemu创建riscv开发板，并移植操作系统。

## 调试部分

### 1.自定义qemu的时候的调试工具（请切换到需要修改qemu的版本再使用）


#### 1.gdb调试流程
``` bash
./build-debug.sh      # 1. 编译调试版本
./debug-qemu.sh       # 2. 直接开始GDB调试
```

#### 2.VSCode调试流程
``` bash
./build-debug.sh           # 1. 编译调试版本
./debug-qemu-vscode.sh     # 2. 启动QEMU等待调试器
# 然后在VSCode中按F5附加调试
```

或者直接在VSCode中：
```bash
./build-debug.sh      # 1. 编译调试版本
# 2. 在VSCode中按F5（会自动启动QEMU）
```

### 2.调试OpenSBI



### 3.调试OS模块
若使用VSCode调试的话，先将`run.sh`的`-s -S`的注释去掉，然后再按下F5。