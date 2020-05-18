/*
Copyright 2020 Orange

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

#include <stdlib.h>
#include "ebpf_runtime_ubpf.h"


#define PCAPOUT "_out.pcap"

pcap_list_t *feed_packets(packet_filter ebpf_filter, pcap_list_t *pkt_list, int debug) {
    pcap_list_t *output_pkts = allocate_pkt_list();
    uint32_t list_len = get_pkt_list_length(pkt_list);
    for (uint32_t i = 0; i < list_len; i++) {
        /* Parse each packet in the list and check the result */
        struct dp_packet dp;
        struct std_meta {
            uint32_t input_port;
            uint32_t packet_length;
            uint32_t output_action;
            uint32_t output_port;
        };
        struct std_meta md;
        pcap_pkt *input_pkt = get_packet(pkt_list, i);
        dp.data = (void *) input_pkt->data;
        dp.size_ = input_pkt->pcap_hdr.len;

        md.input_port = input_pkt->ifindex;
        md.packet_length = dp.size_;
        md.output_port = 0;

        int result = ebpf_filter(&dp, (struct standard_metadata *) &md);
        /* Updating input_pkt's length */
        input_pkt->pcap_hdr.len = dp.size_;
        input_pkt->pcap_hdr.caplen = dp.size_;
        if (result != 0) {
            /* We copy the entire content to emulate an outgoing packet */
            pcap_pkt *out_pkt = copy_pkt(input_pkt);
            out_pkt->ifindex = md.output_port;
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

void *run_and_record_output(packet_filter entry, const char *pcap_base, pcap_list_t *pkt_list, int debug) {
    /* Create an array of packet lists */
    pcap_list_array_t *output_array = allocate_pkt_list_array();
    /* Feed the packets into our "loaded" program */
    pcap_list_t *output_pkts = feed_packets(entry, pkt_list, debug);
    /* Split the output packet list by interface. This destroys the list. */
    output_array = split_and_delete_list(output_pkts, output_array);
    /* Write each list to a separate pcap output file */
    write_pkts_to_pcaps(pcap_base, output_array, debug);
    /* Delete the array, including the data it is holding */
    delete_array(output_array);
}