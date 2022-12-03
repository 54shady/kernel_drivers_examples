#include <linux/ptrace.h>
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/seccomp.h>
#include <uapi/linux/unistd.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

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

	value = bpf_map_lookup_elem(&my_map, &pid);
	if (value)
	{
		/* increse vmexit in each call */
		exit_count = ++(*value);
		bpf_trace_printk(fmt, sizeof(fmt), pid, exit_count, *value);
	}
	bpf_map_update_elem(&my_map, &pid, &exit_count, BPF_ANY);

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
