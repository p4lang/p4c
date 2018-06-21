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

#define FILE_NAME_MAX 256

void usage(char *name) {
    printf("usage: %s file.pcap\n"
           "This program parses a pcap file, feeds the individual packets"
           " into a filter function, and returns the output.\n" , name);
}

int main(int argc, char **argv) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    const unsigned char *packet;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pcap_hdr;
    int ret;
    if (argc != 2) {
        usage(argv[0]);
        fprintf(stderr, "The input trace file is missing."
                 "Expected .pcap file as input argument.\n");
        return EXIT_FAILURE;
    }
    /* Skip over the program name. */
    ++argv; --argc;

    /* Initialize the registry of shared tables */
    struct bpf_map_def* current = tables;
    while (current->name != 0) {
        registry_add(current);
        current++;
    }

    /* Open and read pcap file */
    in_handle = pcap_open_offline(argv[0], errbuf);
    if (in_handle == NULL) {
        pcap_perror(in_handle, "Error: Failed to read pcap file ");
        return EXIT_FAILURE;
    }

    /* Create the output file. */
    char *in_file_name = strdup(argv[0]);
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

    /* Parse each packet in the file and check the result */
    while ((ret = pcap_next_ex(in_handle, &pcap_hdr, &packet)) == 1) {
        struct sk_buff skb;
        skb.data = (void *) packet;
        skb.len = pcap_hdr->len;
        int result = ebpf_filter(&skb);
        if (result)
            pcap_dump((unsigned char *) out_handle, pcap_hdr, packet);
        printf("\nResult of the eBPF parsing is: %d\n", result);
    }
    if (ret == -1)
        pcap_perror(in_handle, "Error: Failed to parse ");

    pcap_close(in_handle);
    pcap_dump_close(out_handle);
    free(out_file_name);
    return EXIT_SUCCESS;
}
