#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "qemu/guest-random.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/char/serial.h"
#include "target/riscv/cpu.h"

#include "hw/riscv/riscv_hart.h"
#include "hw/riscv/quard_star.h"
#include "hw/riscv/boot.h"
#include "hw/riscv/numa.h"
#include "hw/intc/riscv_aclint.h"
#include "hw/intc/riscv_aplic.h"
#include "hw/intc/riscv_imsic.h"
#include "hw/platform-bus.h"
#include "hw/intc/sifive_plic.h"
#include "hw/misc/sifive_test.h"

#include "chardev/char.h"
#include "sysemu/device_tree.h"
#include "sysemu/sysemu.h"
#include "sysemu/kvm.h"
#include "sysemu/tpm.h"
#include "hw/pci/pci.h"
#include "hw/pci-host/gpex.h"
#include "hw/display/ramfb.h"
#include "hw/acpi/aml-build.h"
#include "qapi/qapi-visit-common.h"

// 定义一个静态的常量数组，用于映射Quard Star系统中不同内存区域的地址和大小
static const MemMapEntry quard_star_memmap[] = {
/*
 * 之所以添加UART0~3，是为了后面我们可能会在多个权限域空间内跑不同的系统因此可能需要不同权限的串口打印输出。
*/
    // 映射MROM区域的起始地址为0x0，大小为0x8000字节
    [QUARD_STAR_MROM]        = {        0x0,        0x8000 },
    // 映射SRAM区域的起始地址为0x8000，大小为0x8000字节
    [QUARD_STAR_SRAM]        = {     0x8000,        0x8000 },
    // 映射内核中断控制器CLINT（Core Local Interruptor）区域的起始地址为0x02000000，大小为0x10000字节
    [QUARD_STAR_CLINT]       = { 0x02000000,       0x10000 },
    // 映射外设中断控制器PLIC（Platform Level Interrupt Controller）区域的起始地址为0x0c000000，大小为0x210000字节
    [QUARD_STAR_PLIC]        = { 0x0c000000,      0x210000 },
    // 映射UART0设备的起始地址为0x10000000，大小为0x1000字节
    [QUARD_STAR_UART0]       = { 0x10000000,        0x1000 },
    // 映射UART1设备的起始地址为0x10001000，大小为0x1000字节
    [QUARD_STAR_UART1]       = { 0x10001000,        0x1000 },
    // 映射UART2设备的起始地址为0x10002000，大小为0x1000字节
    [QUARD_STAR_UART2]       = { 0x10002000,        0x1000 },
    // 映射RTC（Real-Time Clock）设备的起始地址为0x10003000，大小为0x1000字节
    [QUARD_STAR_RTC]         = { 0x10003000,        0x1000 },

    // 映射VIRTIO0设备的起始地址为0x10100000，大小为0x1000字节
    [QUARD_STAR_VIRTIO0]     = { 0x10100000,        0x1000 },
    // 映射FLASH区域的起始地址为0x20000000，大小为0x2000000字节
    [QUARD_STAR_FLASH]       = { 0x20000000,     0x2000000 }, 
    // 映射DRAM区域的起始地址为0x80000000，大小为0x40000000字节
    [QUARD_STAR_DRAM]        = { 0x80000000,    0x40000000 },
};


/**
* 创建QuardStar CPU实例。
* @brief 
* 此函数负责根据机器配置创建QuardStar架构的CPU实例。它会检查Socket数量和ID，
* 初始化每个Socket的CPU配置，并尝试实现这些CPU实例。
* 
* @param machine 机器状态指针，包含机器配置和状态信息。
*/
static void quard_star_cpu_create(MachineState *machine)
{
	int i, base_hartid, hart_count;
	char *soc_name;

	// 获取指向QuardStarState的指针，用于后续的操作和配置
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);
	
	// 检查配置的Socket数量是否超过最大限制
	if (QUARD_STAR_SOCKETS_MAX < riscv_socket_count(machine)) {
		error_report("number of sockets/nodes should be less than %d",
			QUARD_STAR_SOCKETS_MAX);
		exit(1);
	}

	// 遍历每个Socket，检查并创建对应的CPU实例
	for (i = 0; i < riscv_socket_count(machine); i++) {
		// 检查当前Socket的Hart ID是否连续
		if (!riscv_socket_check_hartids(machine, i)) {
			error_report("discontinuous hartids in socket%d", i);
			exit(1);
		}

		// 获取当前Socket的第一个Hart ID
		base_hartid = riscv_socket_first_hartid(machine, i);
		if (base_hartid < 0) {
			error_report("can't find hartid base for socket%d", i);
			exit(1);
		}

		// 获取当前Socket的Hart数量
		hart_count = riscv_socket_hart_count(machine, i);
		if (hart_count < 0) {
			error_report("can't find hart count for socket%d", i);
			exit(1);
		}

		// 生成当前Socket的名称
		soc_name = g_strdup_printf("soc%d", i);
		// 初始化当前Socket的CPU实例
		object_initialize_child(OBJECT(machine), soc_name, &s->soc[i],
								TYPE_RISCV_HART_ARRAY);
		g_free(soc_name);

		// 设置CPU实例的类型
		object_property_set_str(OBJECT(&s->soc[i]), "cpu-type",
								machine->cpu_type, &error_abort);
		
		// 设置CPU实例的Hart ID基值
		object_property_set_int(OBJECT(&s->soc[i]), "hartid-base",
								base_hartid, &error_abort);
		
		// 设置CPU实例的Hart数量
		object_property_set_int(OBJECT(&s->soc[i]), "num-harts",
								hart_count, &error_abort);
		
		// 实现CPU实例
		sysbus_realize(SYS_BUS_DEVICE(&s->soc[i]), &error_abort);
	}
}


/**
* @brief 创建并映射NOR Flash存储设备
* @param machine 指向当前虚拟机状态的指针
* 
* 函数执行流程：
* 1. 初始化CFI闪存设备（Parallel NOR Flash）
* 2. 配置设备物理特性参数
* 3. 注册设备到QOM对象系统
* 4. 关联后端驱动设备
* 5. 验证并设置存储布局参数
* 6. 将设备映射到系统地址空间
*/
static void quard_star_flash_create(MachineState *machine)
{
	#define QUARD_STAR_FLASH_SECTOR_SIZE (256 * KiB)  // 定义闪存扇区大小为256KB
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);
	MemoryRegion *system_memory = get_system_memory();
	DeviceState *dev = qdev_new(TYPE_PFLASH_CFI01);  // 创建CFI接口的并行NOR闪存设备

	/* 配置闪存物理特性 */
	qdev_prop_set_uint64(dev, "sector-length", QUARD_STAR_FLASH_SECTOR_SIZE); // 设置擦除扇区大小
	qdev_prop_set_uint8(dev, "width", 4);         // 数据总线宽度（4位）
	qdev_prop_set_uint8(dev, "device-width", 2);  // 设备宽度（2个4位设备并联）
	qdev_prop_set_bit(dev, "big-endian", false);  // 使用小端字节序
	qdev_prop_set_uint16(dev, "id0", 0x89);       // CFI标准设备ID字节0
	qdev_prop_set_uint16(dev, "id1", 0x18);       // CFI标准设备ID字节1
	qdev_prop_set_uint16(dev, "id2", 0x00);       // 保留ID字节
	qdev_prop_set_uint16(dev, "id3", 0x00);       // 保留ID字节
	qdev_prop_set_string(dev, "name","quard-star.flash0"); // 设备名称

	/* 注册设备到对象系统 */
	object_property_add_child(OBJECT(s), "quard-star.flash0", OBJECT(dev));
	object_property_add_alias(OBJECT(s), "pflash0", OBJECT(dev), "drive"); // 创建属性别名

	s->flash = PFLASH_CFI01(dev);
	pflash_cfi01_legacy_drive(s->flash,drive_get(IF_PFLASH, 0, 0));  // 关联后端驱动

	/* 获取内存映射参数 */
	hwaddr flashsize = quard_star_memmap[QUARD_STAR_FLASH].size;
	hwaddr flashbase = quard_star_memmap[QUARD_STAR_FLASH].base;

	/* 存储布局校验 */
	assert(QEMU_IS_ALIGNED(flashsize, QUARD_STAR_FLASH_SECTOR_SIZE)); // 验证总大小对齐扇区
	assert(flashsize / QUARD_STAR_FLASH_SECTOR_SIZE <= UINT32_MAX);  // 验证块数不超过32位
	
	/* 配置设备参数 */
	qdev_prop_set_uint32(dev, "num-blocks", flashsize / QUARD_STAR_FLASH_SECTOR_SIZE);
	sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);  // 完成设备初始化

	/* 将闪存映射到系统地址空间 */
	memory_region_add_subregion(system_memory, flashbase,
								sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 0));
}


/**
* @brief 创建并映射系统内存区域
* @param machine 指向当前虚拟机状态的指针
* 
* 函数执行流程：
* 1. 分配DRAM、SRAM、MROM三个内存区域对象
* 2. 初始化各内存区域的类型和属性
* 3. 将内存区域映射到系统地址空间
* 4. 设置CPU复位启动向量
*/
static void quard_star_memory_create(MachineState *machine)
{
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);
	MemoryRegion *system_memory = get_system_memory();
	
	/* 分配内存区域对象 */
	MemoryRegion *dram_mem = g_new(MemoryRegion, 1);  // 动态随机存取存储器（主内存）
	MemoryRegion *sram_mem = g_new(MemoryRegion, 1);  // 静态随机存取存储器（高速缓存）
	MemoryRegion *mask_rom = g_new(MemoryRegion, 1);  // 掩膜只读存储器（启动固件）

	/* 初始化1GB DRAM */
	memory_region_init_ram(dram_mem, NULL, "riscv_quard_star_board.dram",
						quard_star_memmap[QUARD_STAR_DRAM].size, &error_fatal);
	memory_region_add_subregion(system_memory, 
								quard_star_memmap[QUARD_STAR_DRAM].base, dram_mem);

	/* 初始化32KB SRAM */
	memory_region_init_ram(sram_mem, NULL, "riscv_quard_star_board.sram",
						quard_star_memmap[QUARD_STAR_SRAM].size, &error_fatal);
	memory_region_add_subregion(system_memory, 
								quard_star_memmap[QUARD_STAR_SRAM].base, sram_mem);

	/* 初始化32KB MROM（存放初始启动代码） */
	memory_region_init_rom(mask_rom, NULL, "riscv_quard_star_board.mrom",
						quard_star_memmap[QUARD_STAR_MROM].size, &error_fatal);
	memory_region_add_subregion(system_memory, 
								quard_star_memmap[QUARD_STAR_MROM].base, mask_rom);
	
	/*
	* 
	* 这行代码设置系统的复位向量（reset vector），即当 CPU 复位时，它会从指定的内存地址（MROM 区域）开始执行代码。
	* 通常情况下，这个地址会指向一个简单的引导程序（bootloader），然后由 bootloader 加载操作系统内核到内存（如 Flash 或 DRAM）中并跳转执行。
	* 这在嵌入式系统和虚拟化环境中非常重要，因为需要明确指定 CPU 上电或复位后的第一段执行代码的位置。
	*
	*
	* 配置CPU复位启动流程：
	*  1. 将复位向量代码加载到MROM区域
	*  2. 设置从0号CPU开始执行
	*  3. 指定Flash作为后续程序加载位置
	* 
	*
	* 实际产生的效果：
	* 1.在 MROM 区域写入 RISC-V 启动指令序列
	* 2.配置硬件线程的初始状态
	* 3.建立从复位地址（0x0）到 Flash 固件的跳转逻辑
	* 4.为后续 U-Boot/Kernel 加载做好准备
	*/
	riscv_setup_rom_reset_vec(machine, &s->soc[0], 
							quard_star_memmap[QUARD_STAR_FLASH].base,  // Flash基地址
							quard_star_memmap[QUARD_STAR_MROM].base,   // MROM基地址
							quard_star_memmap[QUARD_STAR_MROM].size,   // MROM大小
							0x0,  // 设备树加载地址（未使用）
							0x0); // 设备树大小（未使用）
}


/**
* 创建QuardStar架构的PLIC（Platform-Level Interrupt Controller）实例。
* 
* @param machine MachineState类型的指针，表示当前机器的状态。
* 
* 此函数负责根据机器的Socket数量和每个Socket的Hart配置，创建相应的PLIC实例。
* 它通过调用riscv_socket_count获取Socket数量，为每个Socket计算基础Hart ID和Hart数量，
* 并根据这些信息生成PLIC配置字符串。随后，使用这些配置和预定义的内存映射参数，
* 创建PLIC实例并存储在QuardStarState结构体中。
*/
static void quard_star_plic_create(MachineState *machine)
{
	// 获取当前机器的Socket数量。
	int socket_count = riscv_socket_count(machine);
	// 获取QuardStar架构的机器状态。
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);
	int i,hart_count,base_hartid;
	
	// 遍历每个Socket，为每个Socket创建PLIC实例。
	for ( i = 0; i < socket_count; i++) {
		// 获取当前Socket的Hart数量。
		hart_count = riscv_socket_hart_count(machine, i);
		// 获取当前Socket的第一个Hart的ID。
		base_hartid = riscv_socket_first_hartid(machine, i);
		
		// 生成PLIC的Hart配置字符串。
		char *plic_hart_config = riscv_plic_hart_config_string(hart_count);
		
		// 创建PLIC实例，并根据QuardStar的内存映射和配置参数初始化。
		s->plic[i] = sifive_plic_create(
			quard_star_memmap[QUARD_STAR_PLIC].base + 
				i * quard_star_memmap[QUARD_STAR_PLIC].size,
			plic_hart_config, hart_count, base_hartid,
			QUARD_STAR_PLIC_NUM_SOURCES,
			QUARD_STAR_PLIC_NUM_PRIORITIES,
			QUARD_STAR_PLIC_PRIORITY_BASE,
			QUARD_STAR_PLIC_PENDING_BASE,
			QUARD_STAR_PLIC_ENABLE_BASE,
			QUARD_STAR_PLIC_ENABLE_STRIDE,
			QUARD_STAR_PLIC_CONTEXT_BASE,
			QUARD_STAR_PLIC_CONTEXT_STRIDE,
			quard_star_memmap[QUARD_STAR_PLIC].size);
		
		// 释放PLIC Hart配置字符串的内存。
		g_free(plic_hart_config);
	}
}


/**
 * 根据机器状态创建Quard Star ACLINT（抽象中断控制器）。
 * 该函数为每个插座（socket）创建软件中断（SWI）和机器定时器（MTIMER）ACLINT实例。
 * 
 * @param machine 指向MachineState结构的指针，包含机器的配置和状态信息。
 */
static void quard_star_aclint_create(MachineState *machine)
{
	// 定义变量i用于循环迭代，hart_count用于记录每个插座的HART数量，base_hartid用于记录每个插座的起始HART ID。
	int i , hart_count,base_hartid;
	
	// 获取机器中插座的数量。
	int socket_count = riscv_socket_count(machine);
	
	// 遍历每个插座。
	for ( i = 0; i < socket_count; i++) {
		// 获取当前插座的起始HART ID。
		base_hartid = riscv_socket_first_hartid(machine, i);
		// 获取当前插座的HART数量。
		hart_count = riscv_socket_hart_count(machine, i);

		// 为当前插座创建软件中断ACLINT实例。
		riscv_aclint_swi_create(
			quard_star_memmap[QUARD_STAR_CLINT].base + 
				i * quard_star_memmap[QUARD_STAR_CLINT].size,
			base_hartid, hart_count, false);

		// 为当前插座创建机器定时器ACLINT实例。
		riscv_aclint_mtimer_create(
			quard_star_memmap[QUARD_STAR_CLINT].base + 
				i * quard_star_memmap[QUARD_STAR_CLINT].size + 
				RISCV_ACLINT_SWI_SIZE,
			RISCV_ACLINT_DEFAULT_MTIMER_SIZE,
			base_hartid, hart_count,
			RISCV_ACLINT_DEFAULT_MTIMECMP,
			RISCV_ACLINT_DEFAULT_MTIME,
			RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ, 
			true);
	}
}


/**
* 创建QuardStar机器的串行端口
* 
* 此函数负责在QuardStar机器的系统内存中初始化三个UART（通用异步收发传输器）设备
* 它们分别位于不同的内存基地址，并连接到各自的中断号每个UART设备都配置为与宿主机的
* 串口后端通信，支持小端数据格式初始化过程中，不使用DMA通道，波特率设置为115200bps
* 
* @param machine 指向机器状态的指针，用于访问机器特定的配置和状态
*/
static void quard_star_serial_create(MachineState *machine)
{
	// 获取系统内存区域
	MemoryRegion *system_memory = get_system_memory();
	// 获取QuardStar机器的状态
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);
	
	// 初始化UART0：
	// 基地址: 0x10000000
	// 中断号: QUARD_STAR_UART0_IRQ
	// 波特率: 399193 (115200bps)
	// 后端设备: 第一个串口
	serial_mm_init(system_memory, quard_star_memmap[QUARD_STAR_UART0].base,
		0, // 无DMA通道
		qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART0_IRQ), // PLIC中断输入
		399193, // 时钟频率
		serial_hd(0), // 关联宿主机的第一个串口
		DEVICE_LITTLE_ENDIAN); // 小端模式

	// 初始化UART1：
	// 基地址: 0x10001000（偏移4KB）
	// 中断号: QUARD_STAR_UART1_IRQ
	serial_mm_init(system_memory, quard_star_memmap[QUARD_STAR_UART1].base,
		0, qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART1_IRQ), 399193,
		serial_hd(1), DEVICE_LITTLE_ENDIAN);

	// 初始化UART2：
	// 基地址: 0x10002000（偏移8KB）
	// 中断号: QUARD_STAR_UART2_IRQ
	serial_mm_init(system_memory, quard_star_memmap[QUARD_STAR_UART2].base,
		0, qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART2_IRQ), 399193,
		serial_hd(2), DEVICE_LITTLE_ENDIAN);
}


/**
* 创建QuardStar实时钟(RTC)设备
* 
* @param machine 机器状态指针，包含机器初始化的相关信息
* 
* 此函数通过在系统总线上创建一个名为"goldfish_rtc"的设备来初始化实时钟
* 它使用了QuardStar平台特定的内存映射信息来确定RTC设备的基地址，
* 并且配置了中断请求线(IRQ)以便处理RTC产生的中断
*/
static void quard_star_rtc_create(MachineState *machine)
{    
	// 获取QuardStar平台的状态指针
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);

	// 在系统总线上创建并初始化RTC设备
	sysbus_create_simple("goldfish_rtc", 
						quard_star_memmap[QUARD_STAR_RTC].base,
						qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_RTC_IRQ));
}

/**
* 创建QuardStar虚拟I/O的MMIO（内存映射I/O）设备。
* 
* @param machine 机器状态的指针，包含机器的相关配置和状态信息。
* 
* 此函数通过在特定的内存地址创建virtio-mmio设备，将QuardStar虚拟I/O设备添加到系统中。
* 它使用了系统总线创建函数，指定了设备的基地址和中断线。
* 这里的设备基地址和中断线是通过查表和PLIC（可编程中断控制器）相关函数获取的。
*/
static void quard_star_virtio_mmio_create(MachineState *machine){
	// 获取QuardStar状态的指针，以便访问其成员。
	QuardStarState *s = RISCV_VIRT_MACHINE(machine);

	// 创建virtio-mmio设备，指定其基地址和中断线。
	// 设备基地址和中断线是根据预定义的内存映射表获取的。
	sysbus_create_simple("virtio-mmio",
	quard_star_memmap[QUARD_STAR_VIRTIO0].base,
	qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_VIRTIO0_IRQ));
}

/* quard-star 初始化各种硬件 */
static void quard_star_machine_init(MachineState *machine)
{
    //创建CPU
    quard_star_cpu_create(machine);
   // 创建主存
    quard_star_memory_create(machine);
    //创建flash
    quard_star_flash_create(machine);
    //创建PLIC
    quard_star_plic_create(machine);
    //创建RISCV_ACLINT
    quard_star_aclint_create(machine);
    // 创建串口设备
    quard_star_serial_create(machine);
    // 创建 RTC
    quard_star_rtc_create(machine);
    // 创建 virtio 
    quard_star_virtio_mmio_create(machine);
}

/**
 * 初始化四星机器实例
 * 
 * 此函数在四星机器的生命周期中被调用，用于执行初始设置和配置
 * 它确保机器实例准备好进行后续的操作或处理
 * 
 * @param obj 指向需要初始化的对象指针
 *            此参数允许访问和操作对象的属性或方法
 * 
 * 注意：当前函数体为空，表示尚未实现具体的初始化逻辑
 *       当需要添加初始化操作时，在此函数中实现
 */
 static void quard_star_machine_instance_init(Object *obj)
 {
 
 }

/**
* 初始化Quard Star机器类。
* 
* @param oc ObjectClass指针，用于访问和修改对象类的属性。
* @param data 附加数据指针，通常用于传递额外的初始化数据。
* 
* 此函数负责设置Quard Star机器类的属性和行为，包括描述、初始化函数、最大CPU数量、默认CPU类型以及对特定功能的支持。
* 它还配置了与NUMA相关的属性，以支持非统一内存访问架构。
*/
static void quard_star_machine_class_init(ObjectClass *oc, void *data)
{
	MachineClass *mc = MACHINE_CLASS(oc);

	// 设置机器描述，提供关于机器的简要信息。
	mc->desc = "RISC-V Quard Star board";
	
	// 指定机器初始化函数，用于在机器启动时执行特定的初始化操作。
	mc->init = quard_star_machine_init;
	
	// 定义机器支持的最大CPU数量。
	mc->max_cpus = QUARD_STAR_CPUS_MAX;
	
	// 设置默认的CPU类型，这是在没有指定CPU类型时使用的。
	mc->default_cpu_type = TYPE_RISCV_CPU_BASE;
	
	// 允许PCI设备地址从0开始，这取决于具体的硬件配置和需求。
	mc->pci_allow_0_address = true;
	
	// 指定函数用于获取可能的CPU架构ID，这在NUMA配置中尤为重要。
	mc->possible_cpu_arch_ids = riscv_numa_possible_cpu_arch_ids;
	
	// 设置函数用于根据CPU索引获取CPU实例属性，这对于NUMA配置中的CPU放置很重要。
	mc->cpu_index_to_instance_props = riscv_numa_cpu_index_to_props;
	
	// 指定函数用于获取默认的CPU节点ID，这在NUMA配置中用于决定CPU的归属。
	mc->get_default_cpu_node_id = riscv_numa_get_default_cpu_node_id;
	
	// 表示机器支持NUMA内存配置，这是构建NUMA系统的基础。
	mc->numa_mem_supported = true;
}

/**
 * quard_star_machine_typeinfo: 定义了"quard-star"机器类型的类型信息
 * 这个结构体是静态常量，用于向系统注册一种特定的机器类型
 * 它包含了创建和管理这种机器类型所需的信息，比如名称、父类型、类初始化函数等
 */
 static const TypeInfo quard_star_machine_typeinfo = {
    // 机器类型的名称，用于标识这种机器类型
    .name       = MACHINE_TYPE_NAME("quard-star"),
    // 指定父类型，这里设置为基本的机器类型
    .parent     = TYPE_MACHINE,
    // 类初始化函数，用于初始化与这种机器类型相关的类属性
    .class_init = quard_star_machine_class_init,
    // 实例初始化函数，用于初始化这种机器类型的实例
    .instance_init = quard_star_machine_instance_init,
    // 指定这种机器类型实例的大小，这里是QuardStarState结构体的大小
    .instance_size = sizeof(QuardStarState),
    // 定义这种机器类型实现的接口，这里实现了热插拔处理接口
    .interfaces = (InterfaceInfo[]) {
         { TYPE_HOTPLUG_HANDLER },
         { }
    },
};

/**
* 初始化并注册四星机器的类型信息
* 
* 该函数在四星机器的初始化过程中被调用，用于注册机器的类型信息
* 通过调用type_register_static函数，将quard_star_machine_typeinfo结构体
* 中的类型信息注册到系统中，以便后续的实例化和使用
*/
static void quard_star_machine_init_register_types(void)
{
	// 注册四星机器的类型信息
	type_register_static(&quard_star_machine_typeinfo);
}

// 初始化函数指针类型的变量，用于注册quard_star_machine初始化的类型信息
type_init(quard_star_machine_init_register_types)