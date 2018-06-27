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

/* The runtime emulates a loaded eBPF program and provides an interface to feed
   packets to the emulated eBPF filter. */



#include <string.h>
#include <libgen.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "test.h"
#include <unistd.h>
#include <ctype.h>
#define FILE_NAME_MAX 256

static int debug = 0;

void usage(char *name) {
    fprintf(stderr, "This program parses a pcap file, "
            "feeds the individual packets into a filter function, "
            "and returns the output.\n");
    fprintf(stderr, "Usage: %s [-d] -f file.pcap\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-d: Turn on debug messages\n");
    fprintf(stderr, "\t-f: The input pcap file\n");
    exit(EXIT_FAILURE);
}

static int process_packets(pcap_t *in_handle, pcap_dumper_t *out_handle) {
    struct pcap_pkthdr *pcap_hdr;
    const unsigned char *packet;
    int ret;
    while ((ret = pcap_next_ex(in_handle, &pcap_hdr, &packet)) == 1) {
        /* Parse each packet in the file and check the result */
        struct sk_buff skb;
        skb.data = (void *) packet;
        skb.len = pcap_hdr->len;
        int result = ebpf_filter(&skb);
        if (result)
            pcap_dump((unsigned char *) out_handle, pcap_hdr, packet);
        if (debug)
            printf("\nResult of the eBPF parsing is: %d\n", result);
    }
    return ret;
}

int main(int argc, char **argv) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    char *input_pcap = NULL;
    int c;

    opterr = 0;

    while ((c = getopt (argc, argv, "df:")) != -1) {
        switch (c) {
            case 'd':
            debug = 1;
            break;
            case 'f':
            input_pcap = optarg;
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
    if (!input_pcap) {
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
    /* Open and read pcap file */
    in_handle = pcap_open_offline(input_pcap, errbuf);
    if (in_handle == NULL) {
        pcap_perror(in_handle, "Error: Failed to read pcap file ");
        return EXIT_FAILURE;
    }

    /* Create the output file. */
    char *in_file_name = strdup(input_pcap);
    char *in_dir = dirname(in_file_name);
    int in_dir_len = strlen(in_dir);
    /* length of the input directory plus the filename max length */
    char *out_file_name = calloc(in_dir_len + FILE_NAME_MAX , 1);
    snprintf(out_file_name, in_dir_len + FILE_NAME_MAX, "%s/pcap0_out.pcap", in_dir);

    /* Open the output file */
    out_handle = pcap_dump_open(in_handle, out_file_name);
    if (out_handle == NULL) {
        pcap_perror(in_handle, "Error: Failed to create pcap output file ");
        pcap_close(in_handle);
        free(out_file_name);
        return EXIT_FAILURE;
    }

    /* Set the default action for the userspace hash tables */
    initialize_tables();
    int ret = process_packets(in_handle, out_handle);
    if (ret == -1)
        pcap_perror(in_handle, "Error: Failed to parse ");
    pcap_close(in_handle);
    pcap_dump_close(out_handle);
    free(out_file_name);
    return EXIT_SUCCESS;
}
