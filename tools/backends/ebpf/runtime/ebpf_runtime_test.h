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
 * Runtime operations specific to the test target. Runs a provided filter
 * function, records the output, and writes it to a physical file. Does not use
 * any Linux networking tools and only operates on custom hash maps.
 */

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_

#include "pcap_util.h"
#include "ebpf_test.h"

typedef int (*packet_filter)(SK_BUFF* s);

void *run_and_record_output(packet_filter ebpf_filter, const char *pcap_base, pcap_list_t *pkt_list, int debug);
void init_ebpf_tables(int debug);
void delete_ebpf_tables(int debug);

#define RUN(ebpf_filter, pcap_base, num_pcaps, input_list, debug) \
    run_and_record_output(ebpf_filter, pcap_base, input_list, debug)
#define INIT_EBPF_TABLES(debug) init_ebpf_tables(debug)
#define DELETE_EBPF_TABLES(debug) delete_ebpf_tables(debug)

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_
