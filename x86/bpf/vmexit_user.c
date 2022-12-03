#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <sys/resource.h>
#include "trace_helpers.h"

#define NR_VCPU 4

#define ITERAT_ALL

int main(int argc, char *argv[])
{
	struct bpf_link *link = NULL;
	struct bpf_program *prog;
	struct bpf_object *obj;
	char filename[256];
	int fd;
	int cur;
#ifdef ITERAT_ALL
	int next_key, lookup_key;
#else
	/*
	 * pstree can print out each vcpu thread id
	 * pstree -t -p `vminfo -n demo-3 -p`
	 *  	qemu-system-x86(3286)─┬─{CPU 0/KVM}(3345)
	 *  						  ├─{CPU 1/KVM}(3355)
	 *  						  ├─{CPU 2/KVM}(3366)
	 *  						  ├─{CPU 3/KVM}(3373)
	 */
	int pids[NR_VCPU] = {3345, 3355, 3366, 3373};
	int prev[NR_VCPU] = {0, 0, 0, 0};
	int sum = 0, delta = 0;
	int i;
#endif

	snprintf(filename, sizeof(filename), "%s_kern.o", argv[0]);
	obj = bpf_object__open_file(filename, NULL);
	if (libbpf_get_error(obj)) {
		fprintf(stderr, "ERROR: opening BPF object file failed\n");
		return 0;
	}

	prog = bpf_object__find_program_by_name(obj, "mybpfprog");
	if (!prog) {
		printf("finding a prog in obj file failed\n");
		goto cleanup;
	}

	/* load BPF program */
	if (bpf_object__load(obj)) {
		fprintf(stderr, "ERROR: loading BPF object file failed\n");
		goto cleanup;
	}

	link = bpf_program__attach(prog);
	if (libbpf_get_error(link)) {
		fprintf(stderr, "ERROR: bpf_program__attach failed\n");
		link = NULL;
		goto cleanup;
	}

	/*
	 * iterating over elements in a BPF Map
	 *
	 * use `bpf_map_get_next_key` with a lookup key that doesn't exist in the map
	 * This forces BPF to start from the beginning of the map
	 */
#if 1
	fd = bpf_object__find_map_fd_by_name(obj, "my_map");
	while (1)
	{
#ifdef ITERAT_ALL
		lookup_key = -1; /* key not exsit in map */
		while (bpf_map_get_next_key(fd, &lookup_key, &next_key) == 0)
		{
			bpf_map_lookup_elem(fd, &lookup_key, &cur);
			lookup_key = next_key;
			printf("%d:%d\n", lookup_key, cur);
		}
#else
		for (i = 0; i < NR_VCPU; i++)
		{
			bpf_map_lookup_elem(fd, &pids[i], &cur);
			delta = cur - prev[i];
			sum += delta;
			prev[i] = cur;
		}
		printf("vmexit total: %d\n", sum);
		sum = 0;
#endif
		sleep(1);
	}
#else
	read_trace_pipe();
#endif

cleanup:
	bpf_link__destroy(link);
	bpf_object__close(obj);
	return 0;
}
