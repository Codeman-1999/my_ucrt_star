# 本代码回被加载到flash执行，主要工作是加载opensbi的固件和设备树，
# 然后跳转到DRAM处执行opensbi的代码


	.macro loop,cunt
    li		t1,	0xffff
    li		t2,	\cunt
1:
	nop
	addi    t1, t1, -1
	bne		t1, x0, 1b
    li		t1,	0xffff
	addi    t2, t2, -1
	bne		t2, x0, 1b
	.endm

# 一个字一个字的循环加载固件到 DRAM处
	.macro load_data,_src_start,_dst_start,_dst_end
	bgeu	\_dst_start, \_dst_end, 2f
1:
	lw      t0, (\_src_start)
	sw      t0, (\_dst_start)
	addi    \_src_start, \_src_start, 4
	addi    \_dst_start, \_dst_start, 4
	bltu    \_dst_start, \_dst_end, 1b
2:
	.endm

	.section .text
	.globl _start
	.type _start,@function

_start:
	//load opensbi_fw.bin 
	//[0x20200000:0x20400000] --> [0x80000000:0x80200000]
    li		a0,	0x202
	slli	a0,	a0, 20      //a0 = 0x20200000
    li		a1,	0x800
	slli	a1,	a1, 20      //a1 = 0x80000000
    li		a2,	0x802
	slli	a2,	a2, 20      //a2 = 0x80200000
	load_data a0,a1,a2

	//load qemu_sbi.dtb
	//[0x20080000:0x20100000] --> [0x82200000:0x82280000]
    li		a0,	0x2008
	slli	a0,	a0, 16       //a0 = 0x20080000
    li		a1,	0x822
	slli	a1,	a1, 20       //a1 = 0x82200000
    li		a2,	0x8228
	slli	a2,	a2, 16       //a2 = 0x82280000
	load_data a0,a1,a2

	//load trusted_fw.bin
	//[0x20400000:0x20800000] --> [0xBF800000:0xBFC00000]
    li		a0,	0x204
	slli	a0,	a0, 20      //a0 = 0x20400000
    li		a1,	0xbf8
	slli	a1,	a1, 20      //a1 = 0xbf800000
    li		a2,	0xbfc
	slli	a2,	a2, 20      //a2 = 0xbfc00000
	load_data a0,a1,a2
	
	# //load qemu_uboot.dtb
	# //[0x20100000:0x20180000] --> [0x82000000:0x82080000]
    # li		a0,	0x201
	# slli	a0,	a0, 20       //a0 = 0x20100000
	# li		a1,	0x820
	# slli	a1,	a1, 20       //a1 = 0x82000000
    # li		a2,	0x8208
	# slli	a2,	a2, 16       //a2 = 0x82080000
	# load_data a0,a1,a2

	//load os.bin
	//[0x20800000:0x20C00000] --> [0x80200000:0x80600000]
    li		a0,	0x208
	slli	a0,	a0, 20      //a0 = 0x20800000
    li		a1,	0x802
	slli	a1,	a1, 20      //a1 = 0x80200000
    li		a2,	0x808
	slli	a2,	a2, 20      //a2 = 0x80800000
	load_data a0,a1,a2


    csrr    a0, mhartid
    li		t0,	0x0     
	beq		a0, t0, _no_wait
	loop	0x1000
_no_wait:
    li		a1,	0x822
	slli	a1,	a1, 20       //a1 = 0x82200000
    li	    t0,	0x800
	slli	t0,	t0, 20       //t0 = 0x80000000
    jr      t0

    .end