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

/* The runtime emulates a loaded eBPF program. It parses a set of given
  pcap files and executes a C program that processes packets extracted from
  the capture files. The C program takes a packet structure (commonly an skbuf)
  and returns an action value for each packet.
 */

#include <unistd.h>     // getopt()
#include <ctype.h>      // isprint()
#include "test.h"
#ifdef CONTROL_PLANE
#include "control.h"
#endif
#include "pcap_util.h"

#define FILE_NAME_MAX 256
#define PCAPIN  "_in.pcap"
#define PCAPOUT "_out.pcap"
#define DELIM   '_'

static int debug = 0;

void usage(char *name) {
    fprintf(stderr, "This program expects a pcap file pattern, "
            "extracts all the packets out of the matched files"
            "in the order given by the packet time,"
            "then feeds the individual packets into a filter function, "
            "and returns the output.\n");
    fprintf(stderr, "Usage: %s [-d] -f file.pcap [-n num_files]\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-d: Turn on debug messages\n");
    fprintf(stderr, "\t-f: The input pcap file\n");
    fprintf(stderr, "\t-n: Specifies the number of input files\n");
    exit(EXIT_FAILURE);
}

static pcap_list_t *get_packets(const char *pcap_base, uint16_t num_files) {
    pcap_list_array_t *tmp_list_array = allocate_pkt_list_array();
    /* Retrieve a list for each file and append it to the temporary array */
    for (uint16_t i = 0; i < num_files; i++) {
        char pcap_in_name[FILE_NAME_MAX];
        snprintf(pcap_in_name, FILE_NAME_MAX, "%s%d%s", pcap_base, i, PCAPIN);
        if (debug)
            printf("Processing input file: %s\n", pcap_in_name);
        pcap_list_t *pkt_list = read_pkts_from_pcap(pcap_in_name, i);
        tmp_list_array = append_list(tmp_list_array, pkt_list);
    }
    /* Merge the array into a newly allocated list. This destroys the array. */
    pcap_list_t *merged_list = merge_pcap_lists(tmp_list_array);
    return merged_list;
}

static pcap_list_t *feed_packets(pcap_list_t *pkt_list) {
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
            pcap_pkt *out_pkt = malloc(sizeof(pcap_pkt));
            out_pkt->data = malloc(skb.len);
            memcpy(out_pkt->data, skb.data, skb.len);
            out_pkt->pcap_hdr = input_pkt->pcap_hdr;
            out_pkt->ifindex = input_pkt->ifindex;
            output_pkts = append_packet(output_pkts, out_pkt);
        }
        if (debug)
            printf("Result of the eBPF parsing is: %d\n", result);
    }
    return output_pkts;
}

static void write_pkts_to_pcaps(const char *pcap_base, pcap_list_array_t *output_array) {
    uint16_t arr_len = get_list_array_length(output_array);
    for (uint16_t i = 0; i < arr_len; i++) {
        char pcap_out_name[FILE_NAME_MAX];
        snprintf(pcap_out_name, FILE_NAME_MAX, "%s%d%s", pcap_base, i, PCAPOUT);
        if (debug)
            printf("Processing output file: %s\n", pcap_out_name);
        pcap_list_t *out_list = get_list(output_array, i);
        write_pkts_to_pcap(pcap_out_name, out_list);
    }
}

static pcap_list_array_t *process_input_list(pcap_list_t *pkt_list) {
    /* Feed the packets into our "loaded" program */
    pcap_list_t *output_pkts = feed_packets(pkt_list);
    /* Split the output packet list by interface. This destroys the list. */
    return split_list_by_interface(output_pkts);
}

void launch_runtime(const char *pcap_name, uint16_t num_pcaps) {
    if (num_pcaps == 0)
        return;
    /* Our pcap handles, do not forget to delete them after */
    pcap_list_t *pkt_list_handle = allocate_pkt_list();
    pcap_list_array_t *output_array_handle = allocate_pkt_list_array();

    /* Create the basic pcap filename from the input */
    const char *suffix = strrchr(pcap_name, DELIM);
    int baselen = suffix - pcap_name;
    char pcap_base[baselen];
    memcpy(pcap_base, pcap_name, baselen);
    pcap_base[baselen] = '\0';

    /* Open all matching pcap files retrieve a merged list of packets */
    pkt_list_handle = get_packets(pcap_base, num_pcaps);
    /* Sort the list */
    sort_pcap_list(pkt_list_handle);
    /* Run the "program" and retrieve output lists */
    output_array_handle = process_input_list(pkt_list_handle);
    /* Write each list to a separate pcap output file */
    write_pkts_to_pcaps(pcap_base, output_array_handle);
    /* Delete the handles, including the data they hold */
    delete_array(output_array_handle);
    delete_list(pkt_list_handle);
}

int main(int argc, char **argv) {
    const char *pcap_name = NULL;
    uint16_t num_pcaps = 0;
    int c;
    opterr = 0;

    while ((c = getopt (argc, argv, "dn:f:")) != -1) {
        switch (c) {
            case 'd':
            debug = 1;
            break;
            case 'n':
                num_pcaps = (uint16_t)strtol(optarg, (char **)NULL, 10);
            break;
            case 'f':
                pcap_name = optarg;
            break;
            case '?':
            if (optopt == 'f')
                fprintf(stderr, "The input trace file is missing. "
                 "Expected .pcap file as input argument.\n");
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,"Unknown option character `\\x%x'.\n", optopt);
                return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    /* Check if there was actually any file input. */
    if (!pcap_name) {
        usage(argv[0]);
    }

    /* Initialize the registry of shared tables */
    struct bpf_table* current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Adding table %s\n", current->name);
        BPF_OBJ_PIN(current, current->name);
        current++;
    }

#ifdef CONTROL_PLANE
    /* Set the default action for the userspace hash tables */
    init_table_defaults();
    /* Run all commands specified in the control file */
    init_tables();
#endif

    launch_runtime(pcap_name, num_pcaps);
    return EXIT_SUCCESS;
}
