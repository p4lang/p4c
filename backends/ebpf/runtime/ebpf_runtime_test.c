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
#include "ebpf_test.h"
#include "ebpf_runtime_test.h"

#define PCAPOUT "_out.pcap"

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
pcap_list_t *feed_packets(packet_filter ebpf_filter, pcap_list_t *pkt_list, int debug) {
    pcap_list_t *output_pkts = allocate_pkt_list();
    uint32_t list_len = get_pkt_list_length(pkt_list);
    for (uint32_t i = 0; i < list_len; i++) {
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

void write_pkts_to_pcaps(const char *pcap_base, pcap_list_array_t *output_array, int debug) {
    uint16_t arr_len = get_list_array_length(output_array);
    for (uint16_t i = 0; i < arr_len; i++) {
        char *pcap_out_name = generate_pcap_name(pcap_base, i, PCAPOUT);
        if (debug)
            printf("Processing output file: %s\n", pcap_out_name);
        pcap_list_t *out_list = get_list(output_array, i);
        write_pkts_to_pcap(pcap_out_name, out_list);
        free(pcap_out_name);
    }
}

void *run_and_record_output(packet_filter ebpf_filter, const char *pcap_base, pcap_list_t *pkt_list, int debug) {
    /* Create an array of packet lists */
    pcap_list_array_t *output_array = allocate_pkt_list_array();
    /* Feed the packets into our "loaded" program */
    pcap_list_t *output_pkts = feed_packets(ebpf_filter, pkt_list, debug);
    /* Split the output packet list by interface. This destroys the list. */
    output_array = split_and_delete_list(output_pkts, output_array);
    /* Write each list to a separate pcap output file */
    write_pkts_to_pcaps(pcap_base, output_array, debug);
    /* Delete the array, including the data it is holding */
    delete_array(output_array);
}

void init_ebpf_tables(int debug) {
    /* Initialize the registry of shared tables */
    struct bpf_table* current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Adding table %s\n", current->name);
        BPF_OBJ_PIN(current, current->name);
        current++;
    }
}

void delete_ebpf_tables(int debug) {
    /* Delete all the remaining tables in the user program */
    struct bpf_table* current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Deleting table %s\n", current->name);
        registry_delete_tbl(current->name);
        current++;
    }
}