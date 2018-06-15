/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <linux/bpf.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include "bpfinclude/bpf_load.h"
#include "bpfinclude/libbpf.h"

int main(int ac, char **argv)
{
    char filename[256];
    struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};

    snprintf(filename, sizeof(filename), "%s", argv[1]);

    if (setrlimit(RLIMIT_MEMLOCK, &r)) {
        perror("setrlimit(RLIMIT_MEMLOCK, RLIM_INFINITY)");
        return 1;
    }

    if (ac != 2) {
        printf("usage: %s BPF.o\n", argv[0]);
        return 1;
    }

    if (load_bpf_file(filename)) {
        printf("FAILED: %s", bpf_log_buf);
        return 1;
    }
    printf("PASS\n");

    return 0;
}
