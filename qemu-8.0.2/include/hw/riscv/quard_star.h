#ifndef HW_RISCV_QUARD_STAR__H
#define HW_RISCV_QUARD_STAR__H

#include "hw/riscv/riscv_hart.h"
#include "hw/sysbus.h"
#include "qom/object.h"
#include "hw/block/flash.h"

/**
 * QUARD_STAR_CPUS_MAX:
 * 定义单个socket支持的最大CPU核心数量，当前配置为8核
 */
#define QUARD_STAR_CPUS_MAX 8

/**
 * QUARD_STAR_SOCKETS_MAX:
 * 定义系统支持的最大socket数量（物理CPU插槽），当前配置为8路
 */
#define QUARD_STAR_SOCKETS_MAX 8

/**
 * TYPE_RISCV_QUARD_STAR_MACHINE:
 * 定义机器类型名称常量，用于QEMU对象模型注册quard-star机器类型
 */
#define TYPE_RISCV_QUARD_STAR_MACHINE MACHINE_TYPE_NAME("quard-star")

// 定义一个名为QuardStarState的结构体类型
typedef struct QuardStarState QuardStarState;

// 声明一个实例检查器，用于检查QuardStarState类型的实例
// 这个宏定义了一系列的类型检查函数和类型属性，以便在运行时动态检查和操作QuardStarState类型的实例
DECLARE_INSTANCE_CHECKER(QuardStarState, RISCV_VIRT_MACHINE,
                         TYPE_RISCV_QUARD_STAR_MACHINE)


/**
 * QuardStarState 结构体用于描述 Quard Star 处理器的状态。
 * 它继承自 MachineState，并包含特定于 Quard Star 处理器的额外状态信息。
 */
 struct QuardStarState {
    /*< private >*/
    MachineState parent;

    /*< public >*/
    /**
     * soc 数组用于存储每个 Quard Star 处理器插槽的状态。
     * 每个元素都是一个 RISCVHartArrayState 实例，代表一个插槽中的处理器核心数组。
     * QUARD_STAR_SOCKETS_MAX 定义了插槽数目的最大值。
     */
    RISCVHartArrayState soc[QUARD_STAR_SOCKETS_MAX];

    /**
     * flash 指针用于连接到系统的闪存设备。
     * 它是一个 PFlashCFI01 类型的指针，该类型是特定于闪存设备的。
     */
    PFlashCFI01 *flash;

    /**
     * plic 数组用于存储每个处理器插槽的平台级中断控制器(PLIC)的状态。
     * 每个元素都是一个 DeviceState 类型的指针，代表一个插槽中的 PLIC 设备。
     * QUARD_STAR_SOCKETS_MAX 定义了插槽数目的最大值。
     */
    DeviceState *plic[QUARD_STAR_SOCKETS_MAX];
};


// 定义一系列常量，用于标识系统中的不同硬件组件
enum {
    QUARD_STAR_MROM,       // 用于标识MROM组件
    QUARD_STAR_SRAM,       // 用于标识SRAM组件
    QUARD_STAR_CLINT,      // 用于标识CLINT组件
    QUARD_STAR_PLIC,       // 用于标识PLIC组件
    QUARD_STAR_UART0,      // 用于标识UART0组件
    QUARD_STAR_UART1,      // 用于标识UART1组件
    QUARD_STAR_UART2,      // 用于标识UART2组件
    QUARD_STAR_RTC,        // 用于标识RTC组件
    QUARD_STAR_VIRTIO0,    // 用于标识VIRTIO0组件
    QUARD_STAR_FLASH,      // 用于标识FLASH组件
    QUARD_STAR_DRAM,       // 用于标识DRAM组件
};

// 定义中断请求（IRQ）的枚举列表，用于标识不同的硬件中断
enum {
    QUARD_STAR_VIRTIO0_IRQ = 1,  // Virtio 0 设备中断请求，值为1
    QUARD_STAR_UART0_IRQ = 10,   // UART 0 端口中断请求，值为10
    QUARD_STAR_UART1_IRQ = 11,   // UART 1 端口中断请求，值为11
    QUARD_STAR_UART2_IRQ = 12,   // UART 2 端口中断请求，值为12
    QUARD_STAR_RTC_IRQ   = 13,   // 实时钟（RTC）中断请求，值为13
};

#define QUARD_STAR_PLIC_NUM_SOURCES    127      //PLIC 支持的中断源的最大数量
#define QUARD_STAR_PLIC_NUM_PRIORITIES 7        //PLIC 支持的中断优先级的数量
#define QUARD_STAR_PLIC_PRIORITY_BASE  0x04     //PLIC 中断优先级寄存器的基址偏移值，用于访问中断优先级信息
#define QUARD_STAR_PLIC_PENDING_BASE   0x1000   //PLIC 中断挂起寄存器的基址偏移值，用于访问中断挂起状态
#define QUARD_STAR_PLIC_ENABLE_BASE    0x2000   //PLIC 中断使能寄存器的基址偏移值，用于控制中断使能状态
#define QUARD_STAR_PLIC_ENABLE_STRIDE  0x80     //PLIC 中断使能寄存器之间的地址间隔
#define QUARD_STAR_PLIC_CONTEXT_BASE   0x200000 //PLIC 上下文保存寄存器的基址偏移值，用于保存中断处理程序的上下文信息
#define QUARD_STAR_PLIC_CONTEXT_STRIDE 0x1000   //PLIC 上下文保存寄存器之间的地址间隔
#define QUARD_STAR_PLIC_SIZE(__num_context) \
    (QUARD_STAR_PLIC_CONTEXT_BASE + (__num_context) * QUARD_STAR_PLIC_CONTEXT_STRIDE)//PLIC_SIZE: 根据上下文数量计算PLIC总内存占用

#endif