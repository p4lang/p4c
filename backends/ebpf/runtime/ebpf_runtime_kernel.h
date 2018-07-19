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

#include <unistd.h>     // getopt()
#include <ctype.h>      // isprint()
#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#include "test.h"
#include "pcap_util.h"

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_

static inline pcap_list_t *feed_packets(pcap_list_t *pkt_list, int debug) {
    pcap_list_t *output_pkts = allocate_pkt_list();
    return output_pkts;
}

#define FEED_PACKETS(pkt_list, debug) \
    feed_packets(pkt_list, debug)
#define INIT_EBPF_TABLES(debug)
#define DELETE_EBPF_TABLES(debug)

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_KERNEL_H_