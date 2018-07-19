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

 */

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_

#include <unistd.h>     // getopt()
#include <ctype.h>      // isprint()
#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#include "test.h"
#include "pcap_util.h"

/**
 * @brief Feed a list packets into an eBPF program.
 * @details This is a mock function emulating the behavior of a running
 * eBPF program. It takes a list of input packets and iteratively parses them
 * using the given imported ebpf_filter function. The output defines whether
 * or not the packet is "dropped." If the packet is not dropped, its content is
 * copied and appended to an output packet list.
 *
 * @param pkt_list A list of input packets running through the filter.
 * @return The list of packets "surviving" the filter function
 */
static pcap_list_t *feed_packets(pcap_list_t *pkt_list, int debug) {
    pcap_list_t *output_pkts = allocate_pkt_list();
    uint64_t list_len = get_pkt_list_length(pkt_list);
    for (uint64_t i = 0; i < list_len; i++) {
        /* Parse each packet in the list and check the result */
        struct sk_buff skb;
        pcap_pkt *input_pkt = get_packet(pkt_list, i);
        skb.data = (void *) input_pkt->data;
        skb.len = input_pkt->pcap_hdr.len;
        int result = ebpf_filter(&skb);
        if (result != 0) {
            /* We copy the entire content to emulate an outgoing packet */
            pcap_pkt *out_pkt = copy_pkt(input_pkt);
            output_pkts = append_packet(output_pkts, out_pkt);
        }
        if (debug)
            printf("Result of the eBPF parsing is: %d\n", result);
    }
    return output_pkts;
}

static void init_ebpf_tables(int debug) {
    /* Initialize the registry of shared tables */
    struct bpf_table* current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Adding table %s\n", current->name);
        BPF_OBJ_PIN(current, current->name);
        current++;
    }
}

static void delete_ebpf_tables(int debug) {

    /* Delete all the remaining tables in the user program */
    struct bpf_table* current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Deleting table %s\n", current->name);
        registry_delete_tbl(current->name);
        current++;
    }
}

#define FEED_PACKETS(pkt_list, debug) feed_packets(pkt_list, debug)
#define INIT_EBPF_TABLES(debug) init_ebpf_tables(debug)
#define DELETE_EBPF_TABLES(debug) delete_ebpf_tables(debug)

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_