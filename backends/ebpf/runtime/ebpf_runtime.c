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
#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#include "test.h"
#ifdef CONTROL_PLANE
#include "control.h"
#endif
#include "pcap_util.h"

#define FILE_NAME_MAX 256
#define MAX_10_UINT16 5;
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
    fprintf(stderr, "Usage: %s [-d] -f file.pcap -n num_pcaps\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-d: Turn on debug messages\n");
    fprintf(stderr, "\t-f: The input pcap file\n");
    fprintf(stderr, "\t-n: Specifies the number of input pcap files\n");
    exit(EXIT_FAILURE);
}


/**
 * @brief Create a pcap file name from a given base name, interface index,
 * and suffix. Return value must be deallocated after usage.
 * @param pcap_base The file base name.
 * @param index The index of the file, represent an interface.
 * @param suffix  Filename suffix (e.g., _in.pcap)
 * @return An allocated string containing the file name.
 */
static char *generate_pcap_name(const char *pcap_base, int index, const char *suffix) {
    /* Dynamic string length plus max decimal representation of uint16_t */
    int file_length = strlen(pcap_base) + strlen(suffix) + MAX_10_UINT16;
    char *pcap_name = malloc(file_length);
    int offset = snprintf(pcap_name, file_length,"%s%d%s",
                    pcap_base, index, suffix);
    if (offset >= FILE_NAME_MAX) {
        fprintf(stderr, "Name %s%d%s too long.\n", pcap_base, index, suffix);
        exit(EXIT_FAILURE);
    }
    return pcap_name;
}

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
            pcap_pkt *out_pkt = copy_pkt(input_pkt);
            output_pkts = append_packet(output_pkts, out_pkt);
        }
        if (debug)
            printf("eBPF filter decision is: %d\n", result);
    }
    return output_pkts;
}

static void write_pkts_to_pcaps(const char *pcap_base, pcap_list_array_t *output_array) {
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

static void *run_and_record_output(const char *pcap_base, pcap_list_t *pkt_list) {
    /* Create an array of packet lists */
    pcap_list_array_t *output_array = allocate_pkt_list_array();
    /* Feed the packets into our "loaded" program */
    pcap_list_t *output_pkts = feed_packets(pkt_list);
    /* Split the output packet list by interface. This destroys the list. */
    output_array = split_and_delete_list(output_pkts, output_array);
    /* Write each list to a separate pcap output file */
    write_pkts_to_pcaps(pcap_base, output_array);
    /* Delete the array, including the data it is holding */
    delete_array(output_array);
}

static pcap_list_t *get_packets(const char *pcap_base, uint16_t num_pcaps, pcap_list_t *merged_list) {
    pcap_list_array_t *tmp_list_array = allocate_pkt_list_array();
    /* Retrieve a list for each file and append it to the temporary array */
    for (uint16_t i = 0; i < num_pcaps; i++) {
        char *pcap_in_name = generate_pcap_name(pcap_base, i, PCAPIN);
        if (debug)
            printf("Processing input file: %s\n", pcap_in_name);
        pcap_list_t *pkt_list = read_pkts_from_pcap(pcap_in_name, i);
        tmp_list_array = insert_list(tmp_list_array, pkt_list, i);
        free(pcap_in_name);
    }
    /* Merge the array into a newly allocated list. This destroys the array. */
    return merge_and_delete_lists(tmp_list_array, merged_list);
}

void launch_runtime(const char *pcap_name, uint16_t num_pcaps) {
    if (num_pcaps == 0)
        return;
    /* Initialize the list of input packets */
    pcap_list_t *input_list = allocate_pkt_list();

    /* Create the basic pcap filename from the input */
    const char *suffix = strrchr(pcap_name, DELIM);
    if (suffix == NULL) {
        fprintf(stderr, "Expected a filename with delimiter \"%c\"."
            "Not found in %s! Exiting...\n", DELIM, pcap_name);
        exit(EXIT_FAILURE);
    }
    int baselen = suffix - pcap_name;
    char pcap_base[baselen + 1];
    snprintf(pcap_base, baselen + 1 , "%s", pcap_name);

    /* Open all matching pcap files retrieve a merged list of packets */
    input_list = get_packets(pcap_base, num_pcaps, input_list);
    /* Sort the list */
    sort_pcap_list(input_list);
    /* Run the "program" and retrieve output lists */
    run_and_record_output(pcap_base, input_list);
    /* Delete the list of input packets */
    delete_list(input_list);
}

int main(int argc, char **argv) {
    const char *pcap_name = NULL;
    int num_pcaps = -1;
    int c;
    opterr = 0;

    while ((c = getopt (argc, argv, "dn:f:")) != -1) {
        switch (c) {
            case 'd':
            debug = 1;
            break;
            case 'n':
                num_pcaps = (int)strtol(optarg, (char **)NULL, 10);
                if (num_pcaps < 0 || num_pcaps > UINT16_MAX) {
                    fprintf(stderr,
                        "Number of inputs out of bounds! Maximum is %d\n",
                        UINT16_MAX);
                    return EXIT_FAILURE;
                }
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
                    fprintf(stderr,"Unknown option character `\\x%x'.\n",
                        optopt);
                    return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    /* Check if there was actually any file or number input */
    if (!pcap_name || num_pcaps == -1) {
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
    init_tables();
    /* Run all commands specified in the control file */
    setup_control_plane();
#endif

    launch_runtime(pcap_name, num_pcaps);

    /* Delete all the remaining tables in the user program */
    current = tables;
    while (current->name != NULL) {
        if (debug)
            printf("Deleting table %s\n", current->name);
        registry_delete_tbl(current->name);
        current++;
    }
    return EXIT_SUCCESS;
}
