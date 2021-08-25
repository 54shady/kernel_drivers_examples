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

int main(int argc, char *argv[])
{
	struct bpf_link *link = NULL;
	struct bpf_program *prog;
	struct bpf_object *obj;
	int key, fd, progs_fd;
	const char *section;
	char filename[256];

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

	progs_fd = bpf_object__find_map_fd_by_name(obj, "progs");
	if (progs_fd < 0) {
		fprintf(stderr, "ERROR: finding a map in obj file failed\n");
		goto cleanup;
	}

	bpf_object__for_each_program(prog, obj) {
		section = bpf_program__section_name(prog);
		/* register only syscalls to PROG_ARRAY */
		if (sscanf(section, "kprobe/%d", &key) != 1)
			continue;

		fd = bpf_program__fd(prog);
		bpf_map_update_elem(progs_fd, &key, &fd, BPF_ANY);
	}

	read_trace_pipe();

cleanup:
	bpf_link__destroy(link);
	bpf_object__close(obj);
	return 0;
}
