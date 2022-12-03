#include <linux/ptrace.h>
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/seccomp.h>
#include <uapi/linux/unistd.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/*
 * /sys/kernel/debug/tracing/events/syscalls/sys_enter_execve
 * 通过监控内核中系统调用execve
 * 当系统中调用了这个系统调用的话就会调用mybpfprog函数
 *
 * 编译好程序后执行./demo
 *
 * 然后打开一个新终端,输入一些命令即可看到结果
 */
SEC("tracepoint/syscalls/sys_enter_execve")
int mybpfprog(struct pt_regs *ctx)
{
	char msg[] = "Hello, BPF World!";

	bpf_trace_printk(msg, sizeof(msg));

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
