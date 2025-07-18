/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */
#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_pmu.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/ipi/fdt_ipi.h>
#include <sbi_utils/reset/fdt_reset.h>


extern struct sbi_platform platform;
static u32 quard_star_hart_index2id[SBI_HARTMASK_MAX_BITS] = { 0 };


/*
 * The fw_platform_init() function is called very early on the boot HART
 * OpenSBI reference firmwares so that platform specific code get chance
 * to update "platform" instance before it is used.
 *
 * The arguments passed to fw_platform_init() function are boot time state
 * of A0 to A4 register. The "arg0" will be boot HART id and "arg1" will
 * be address of FDT passed by previous booting stage.
 *
 * The return value of fw_platform_init() function is the FDT location. If
 * FDT is unchanged (or FDT is modified in-place) then fw_platform_init()
 * can always return the original FDT location (i.e. 'arg1') unmodified.
 */

/**
 * 平台初始化函数，解析设备树（FDT）中的硬件信息并初始化平台数据结构
 *
 * @param arg0 通用参数，存的是hard id
 * @param arg1 传递设备树（FDT）物理地址的参数
 * @param arg2 通用参数，未被使用
 * @param arg3 通用参数，未被使用
 * @param arg4 通用参数，未被使用
 *
 * @return 原始设备树（FDT）指针（物理地址）
 */
unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
				unsigned long arg2, unsigned long arg3,
				unsigned long arg4)
{
	/* 参数说明：
	   arg0: 引导HART的ID 
	   arg1: 设备树(FDT)的物理地址
	   arg2~arg4: 保留参数（未使用）*/
	
	const char *model;
	void *fdt = (void *)arg1;  // 将FDT地址转换为指针
	u32 hartid, hart_count = 0;
	int rc, root_offset, cpus_offset, cpu_offset, len;

	/* 获取设备树根节点 */
	root_offset = fdt_path_offset(fdt, "/");
	if (root_offset < 0)
		goto fail;  // 如果找不到根节点进入失败处理

	/* 从根节点获取平台型号 */
	model = fdt_getprop(fdt, root_offset, "model", &len);
	if (model)
		sbi_strncpy(platform.name, model, sizeof(platform.name));  // 设置平台名称

	/* 处理CPU节点 */
	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		goto fail;  // 找不到CPU节点则失败

	/* 遍历所有CPU子节点 */
	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		/* 解析当前CPU的hart ID */
		rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (rc)
			continue;  // 跳过解析失败的节点

		/* 检查hart ID是否超出最大支持范围 */
		if (SBI_HARTMASK_MAX_BITS <= hartid)
			continue;  // 跳过无效的hart ID

		/* 存储hart ID到索引映射表 */
		quard_star_hart_index2id[hart_count++] = hartid;
	}

	platform.hart_count = hart_count;  // 设置平台核心总数

	/* 返回原始FDT指针 */
	return arg1;

fail:
	/* 失败处理：进入无限等待循环 */
	while (1)
		wfi();  // 等待中断（WFI指令）
}





static int quard_star_early_init(bool cold_boot)
{
	return 0;
}

static int quard_star_final_init(bool cold_boot)
{
	void *fdt;

	if (cold_boot)
		fdt_reset_init();
	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();

	fdt_cpu_fixup(fdt);
	fdt_fixups(fdt);
	fdt_domain_fixup(fdt);

	return 0;
}

static void quard_star_early_exit(void)
{

}

static void quard_star_final_exit(void)
{

}

static int quard_star_domains_init(void)
{
	return fdt_domains_populate(fdt_get_address());
}

static int quard_star_pmu_init(void)
{
	return fdt_pmu_setup(fdt_get_address());
}

static uint64_t quard_star_pmu_xlate_to_mhpmevent(uint32_t event_idx,
					       uint64_t data)
{
	uint64_t evt_val = 0;

	/* data is valid only for raw events and is equal to event selector */
	if (event_idx == SBI_PMU_EVENT_RAW_IDX)
		evt_val = data;
	else {
		/**
		 * Generic platform follows the SBI specification recommendation
		 * i.e. zero extended event_idx is used as mhpmevent value for
		 * hardware general/cache events if platform does't define one.
		 */
		evt_val = fdt_pmu_get_select_value(event_idx);
		if (!evt_val)
			evt_val = (uint64_t)event_idx;
	}

	return evt_val;
}
static u64 quard_star_tlbr_flush_limit(void)
{
	return SBI_PLATFORM_TLB_RANGE_FLUSH_LIMIT_DEFAULT;
}

/**
*	用于在OpenSBI初始化过程中进行特定的操作和配置，
*	在OpenSBI启动过程中，会调用该函数来初始化平台相关的操作和配置，
*	并确保OpenSBI能够正确运行在目标平台上。
 */
const struct sbi_platform_operations platform_ops = {
	.early_init		= quard_star_early_init,             //早期初始化，不需要
	.final_init		= quard_star_final_init,            //最终初始化，需要
	.early_exit		= quard_star_early_exit,            //早期退出，不需要
	.final_exit		= quard_star_final_exit,            //最终退出，不需要
	.domains_init		= quard_star_domains_init,      //从设备树填充域，需要
	.console_init		= fdt_serial_init,              //初始化控制台
	.irqchip_init		= fdt_irqchip_init,             //初始化中断
	.irqchip_exit		= fdt_irqchip_exit,             //中断退出
	.ipi_init		= fdt_ipi_init,
	.ipi_exit		= fdt_ipi_exit,
	.pmu_init		= quard_star_pmu_init,              //需要
	.pmu_xlate_to_mhpmevent = quard_star_pmu_xlate_to_mhpmevent,
	.get_tlbr_flush_limit	= quard_star_tlbr_flush_limit, //需要
	.timer_init		= fdt_timer_init,
	.timer_exit		= fdt_timer_exit,
};

/**
 * 初始化Quard-Star平台的SBI平台结构体
 * 该结构体定义了RISC-V监督二进制接口在Quard-Star硬件平台上的特性与配置
 * @return 返回一个sbi_platform结构体实例
 */
struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "Quard-Star",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= SBI_HARTMASK_MAX_BITS,
	.hart_index2id		= quard_star_hart_index2id,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};