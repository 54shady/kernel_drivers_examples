# Unable to handle kernel paging request at virtual address

## 实例1

如下是出错后的日志,删除了不必要的日志

	[ 5338.393253] Unable to handle kernel paging request at virtual address 207475b7
	[ 5338.423842] pgd = c0004000
	[ 5338.426503] [207475b7] *pgd=00000000
	[ 5338.430030] Internal error: Oops: 5 [#1] PREEMPT SMP ARM
	[ 5338.440159] CPU: 0 PID: 30 Comm: kworker/0:1 Not tainted 3.10.0 #3
	[ 5338.446241] Workqueue: events hub_tt_work
	[ 5338.450191] task: dd998fc0 ti: dd9b8000 task.ti: dd9b8000
	[ 5338.455498] PC is at hcd_clear_tt_buffer_complete+0x24/0x58
	[ 5338.460974] LR is at DWC_SPINLOCK_IRQSAVE+0xc/0x14
	[ 5338.465682] pc : [<c05a03d0>]    lr : [<c05a6ca0>]    psr: 200f0093
	[ 5338.465682] sp : dd9b9eb0  ip : 00000000  fp : 00100100
	[ 5338.476957] r10: 00000001  r9 : c129e26c  r8 : 00000410
	[ 5338.482090] r7 : c129e274  r6 : c129e268  r5 : dbb95838  r4 : dcf06600
	[ 5338.488502] r3 : 2074756f  r2 : 00002f10  r1 : 00002f10  r0 : a00f0013
	[ 5338.494916] Flags: nzCv  IRQs off  FIQs on  Mode SVC_32  ISA ARM  Segment kernel
	[ 5338.502180] Control: 10c5387d  Table: 18da006a  DAC: 00000015
	...
	[ 5339.159699] [<c05a03d0>] (hcd_clear_tt_buffer_complete+0x24/0x58) from [<c0551a50>] (hub_tt_work+0x110/0x14c)
	[ 5339.169449] [<c0551a50>] (hub_tt_work+0x110/0x14c) from [<c005296c>] (process_one_work+0x29c/0x458)
	[ 5339.178341] [<c005296c>] (process_one_work+0x29c/0x458) from [<c0052cbc>] (worker_thread+0x194/0x2d4)
	[ 5339.187402] [<c0052cbc>] (worker_thread+0x194/0x2d4) from [<c0058350>] (kthread+0xa0/0xac)
	[ 5339.195527] [<c0058350>] (kthread+0xa0/0xac) from [<c000e4a0>] (ret_from_fork+0x14/0x34)
	[ 5339.203479] Code: eb001a33 e5953018 e3530000 0a000008 (e5d32048)
	[ 5339.209465] ---[ end trace c1a36ec13ecf501d ]---
	[ 5339.214001] Kernel panic - not syncing: Fatal exception
	[ 5339.219140] CPU1: stopping
	[ 5339.221806] CPU: 1 PID: 0 Comm: swapper/1 Tainted: G      D      3.10.0 #3
	[ 5339.228570] [<c0014c1c>] (unwind_backtrace+0x0/0x11c) from [<c001218c>] (show_stack+0x10/0x14)
	[ 5339.237033] [<c001218c>] (show_stack+0x10/0x14) from [<c0013800>] (handle_IPI+0x154/0x2c8)
	[ 5339.245154] [<c0013800>] (handle_IPI+0x154/0x2c8) from [<c0008558>] (gic_handle_irq+0x54/0x5c)
	[ 5339.253617] [<c0008558>] (gic_handle_irq+0x54/0x5c) from [<c000e000>] (__irq_svc+0x40/0x70)
	[ 5339.261820] Exception stack(0xdd8b9f30 to 0xdd8b9f78)
	[ 5339.266782] 9f20:                                     dd8b9f78 00001531 485fdeb8 000004ef
	[ 5339.274816] 9f40: c2cc4260 00000000 474e7100 000004ef c19c4154 dd8b8000 c19c4164 00000000
	[ 5339.282850] 9f60: 0db80263 dd8b9f78 c007ab88 c0699188 600e0013 ffffffff
	[ 5339.289346] [<c000e000>] (__irq_svc+0x40/0x70) from [<c0699188>] (cpuidle_enter_state+0x54/0xec)
	[ 5339.297981] [<c0699188>] (cpuidle_enter_state+0x54/0xec) from [<c0699390>] (cpuidle_idle_call+0x170/0x280)
	[ 5339.307468] [<c0699390>] (cpuidle_idle_call+0x170/0x280) from [<c000f178>] (arch_cpu_idle+0x8/0x38)
	[ 5339.316360] [<c000f178>] (arch_cpu_idle+0x8/0x38) from [<c00793d4>] (cpu_idle_loop+0x1b8/0x224)
	[ 5339.324910] [<c00793d4>] (cpu_idle_loop+0x1b8/0x224) from [<c007944c>] (freezing_slow_path+0x0/0x80)
	[ 5339.333881] CPU3: stopping
	[ 5339.336544] CPU: 3 PID: 0 Comm: swapper/3 Tainted: G      D      3.10.0 #3
	[ 5339.343304] [<c0014c1c>] (unwind_backtrace+0x0/0x11c) from [<c001218c>] (show_stack+0x10/0x14)
	[ 5339.351767] [<c001218c>] (show_stack+0x10/0x14) from [<c0013800>] (handle_IPI+0x154/0x2c8)
	[ 5339.359887] [<c0013800>] (handle_IPI+0x154/0x2c8) from [<c0008558>] (gic_handle_irq+0x54/0x5c)
	[ 5339.368349] [<c0008558>] (gic_handle_irq+0x54/0x5c) fro  [<c000e000>] (__irq_svc+0x40/0x70)
	[ 5339.376551] Exception stack(0xdd8bdf30 to 0xdd8bdf78)
	[ 5339.381514] df20:                                     dd8bdf78 00001531 485fdeb8 000004ef
	[ 5339.389548] df40: c2cd6260 00000000 47e717c6 000004ef c19c4154 dd8bc000 c19c4164 00000000
	[ 5339.397581] df60: 0db80263 dd8bdf78 c007ab88 c0699188 600f0013 ffffffff
	[ 5339.404083] [<c000e000>] (__irq_svc+0x40/0x70) from [<c0699188>] (cpuidle_enter_state+0x54/0xec)
	[ 5339.412717] [<c0699188>] (cpuidle_enter_state+0x54/0xec) from [<c0699390>] (cpuidle_idle_call+0x170/0x280)
	[ 5339.422204] [<c0699390>] (cpuidle_idle_call+0x170/0x280) from [<c000f178>] (arch_cpu_idle+0x8/0x38)
	[ 5339.431094] [<c000f178>] (arch_cpu_idle+0x8/0x38) from [<c00793d4>] (cpu_idle_loop+0x1b8/0x224)
	[ 5339.439642] [<c00793d4>] (cpu_idle_loop+0x1b8/0x224) from [<c007944c>] (freezing_slow_path+0x0/0x80)
	[ 5340.367135] SMP: failed to stop secondary CPUs
	...
	[ 5340.649732] Rebooting in 5 seconds..

其中可知PC当前在hcd_clear_tt_buffer_complete这个函数的0x24字节,该函数总大小为0x58个字节

	[ 5338.455498] PC is at hcd_clear_tt_buffer_complete+0x24/0x58

PC位于0xc05a03d0

	[ 5338.465682] pc : [<c05a03d0>]    lr : [<c05a6ca0>]    psr: 200f0093

另外可以通过查询System.map可知hcd_clear_tt_buffer_complete的地址如下

	c05a03ac T hcd_clear_tt_buffer_complete

通过以上信息就能计算出PC

	pc = 0xc05a03ac + 0x24 = 0xc05a03d0

通过得知PC指针位置后在GDB里找到代码位置[如何使用GDB调试内核](./android_gdb.md)

	(gdb) b *0xc05a03d0

查看这部分代码的反汇编

	(gdb) disassemble 0xc05a03d0

## 反编译某一模块或是函数

反编译某一函数(hcd_clear_tt_buffer_complete开始地址为0xc05a03ac 到 0xc05a03ac + 0x58(0xc05A0404) )

	arm-linux-gnueabihf-objdump -d --start-address=0xc05a03ac --stop-address=0xc05A0404 vmlinux

反编译模块,使用函数所在模块编译出来的built-in.o也能反编译

	arm-linux-gnueabihf-objdump -D drivers/usb/dwc_otg_310/built-in.o > dwc_otg.dis
