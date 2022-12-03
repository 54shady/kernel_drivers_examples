# 如何定位内核空指针崩溃问题

加载模块(insmod hello.ko)后系统崩溃的日志如下

```shell
[  574.755500] Unable to handle kernel NULL pointer dereference at virtual address 00000000
[  574.763511] pgd = dc268000
[  574.766174] [00000000] *pgd=00000000
[  574.769704] Internal error: Oops: 5 [#1] PREEMPT SMP ARM
[  574.774924] Modules linked in: hello(O+) mali_kbase
[  574.779756] CPU: 3 PID: 1083 Comm: insmod Tainted: G           O 3.10.0 #25
[  574.786599] task: dc0c4440 ti: dea54000 task.ti: dea54000
[  574.791911] PC is at hello_init+0x10/0x2c [hello]
[  574.796541] LR is at do_one_initcall+0x34/0xc8
[  574.800911] pc : [<bf02b010>]    lr : [<c00086a4>]    psr: 600f0013
[  574.800911] sp : dea55f18  ip : 00000000  fp : 00000bd4
[  574.812190] r10: 00000000  r9 : dea54000  r8 : c0cfb000
[  574.817325] r7 : 00000000  r6 : dea54000  r5 : bf02b000  r4 : 00000000
[  574.823740] r3 : 00000000  r2 : 0000000d  r1 : bf029028  r0 : bf02b000
[  574.830157] Flags: nZCv  IRQs on  FIQs on  Mode SVC_32  ISA ARM  Segment user
[  574.837169] Control: 10c5387d  Table: 1c26806a  DAC: 00000015
```

从上面可以知道错误处在hello模块的hello_init函数里

错误发生是PC指针指向hello_init+0x10行

```shell
[  574.791911] PC is at hello_init+0x10/0x2c [hello]
```

从反汇编得到相应代码方法如下

```shell
arm-eabi-objdump your_module.ko -D > output.dis
```

比如我是通过下面这样得到的

```shell
/home/zeroway/3288/src/3288_4.4/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-objdump hello.ko -D > hello.dis
```

在反汇编文件里找到相应的位置

```shell
00000000 <init_module>:
   0:	e92d4010 	push	{r4, lr}
   4:	e3a04000 	mov	r4, #0
   8:	e59f1014 	ldr	r1, [pc, #20]	; 24 <init_module+0x24>
   c:	e3a0200d 	mov	r2, #13
  10:	e5d43000 	ldrb	r3, [r4]
```

错误地址是hello_init,也就是init_module的地方

	r4 为变量t
	push {r4, lr} 是压栈操作
	move r4, #0 对应 t = NULL
	ldr r1, [pc, #20] 这里打印__FUNCTION__
	mov r2, #13 是对应行号
	ldrb r3, [r4] 这里将r4存入r3,但是这里r4是空指针,所以这里崩溃了

对应hello_init如下,其中printk是代码的第13行

```c
static int __init hello_init(void)
{
	struct test *t = NULL;

	printk("%s, %d, tmp = %s\n", __FUNCTION__, __LINE__, t->name);
	return 0;
}
```
