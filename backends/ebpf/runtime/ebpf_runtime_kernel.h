/*
Copyright 2018 VMware, Inc.

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

/*
 * Runtime operations specific to the kernel target. Opens a raw socket per
 * interface, launches a tcpdump packet listener, and writes packets to a
 * virtual interface. The successful output is recorded by tcpdump and written
 * to file.
 */

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_

#include "pcap_util.h"
#include "ebpf_runtime_kernel.h"

void run_and_record_output(pcap_list_t *pkt_list, char *pcap_base, uint16_t num_pcaps, int debug);

#define RUN(ebpf_filter, pcap_base, num_pcaps, input_list, debug) \
    run_and_record_output(input_list, pcap_base, num_pcaps, debug)
#define INIT_EBPF_TABLES(debug)
#define DELETE_EBPF_TABLES(debug)

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_
