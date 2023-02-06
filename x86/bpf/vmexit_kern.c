#include <linux/ptrace.h>
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/seccomp.h>
#include <uapi/linux/unistd.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/*
 * 自定义一个hash类型的bpfmap
 * 用进程pid作为key
 * 每个进程的vmexit次数作为value
 */
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__type(key, int);
	__type(value, int);
	__uint(max_entries, 1000);
} my_map SEC(".maps");

SEC("tracepoint/kvm/kvm_exit")
int mybpfprog(struct pt_regs *ctx)
{
	int *value;
	int exit_count = 0;
	char fmt[] = "pid: %d %d %d\n";
	int pid;

	pid = bpf_get_current_pid_tgid();

	/* 用pid这个作为key在bpfmap里查找对应的value */
	value = bpf_map_lookup_elem(&my_map, &pid);
	if (value)
	{
		/* increse vmexit in each call */
		exit_count = ++(*value);
		bpf_trace_printk(fmt, sizeof(fmt), pid, exit_count, *value);
	}

	/* 将pid作为key, exit_count作为value进行更新bpfmap */
	bpf_map_update_elem(&my_map, &pid, &exit_count, BPF_ANY);

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
