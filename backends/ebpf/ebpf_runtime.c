#include <string.h>
#include <libgen.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "test.h"


void usage() {
    printf("This program parses a pcap file, feeds the individual packets into a filter function, and returns the output.\n");
}

int main(int argc, char **argv) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    const unsigned char *packet;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pcap_hdr;
    int ret;
    /* Skip over the program name. */
    ++argv; --argc;
    if (argc != 1) {
        usage();
        fprintf(stderr, "The input trace file is missing."
                 "Expected .pcap file as input argument.\n");
        return EXIT_FAILURE;
    }

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
    char *in_file = strdup(argv[0]);
    char *in_dir = dirname(in_file);
    int in_dir_len = strlen(in_dir);
    /* length of the input directory plus the expected length of the pcap name */
    char *out_file = calloc(in_dir_len + strlen("/pcapxxxx_out.pcap") , 1);
    memcpy(out_file, in_dir, in_dir_len);
    memcpy(out_file + in_dir_len, "/pcap0_out.pcap", strlen("/pcap0_out.pcap"));

    /* Open the output file */
    out_handle = pcap_dump_open(in_handle, out_file);
    if (out_handle == NULL) {
        pcap_perror(in_handle, "Error: Failed to create pcap file ");
        pcap_close(in_handle);
        free(out_file);
        return EXIT_FAILURE;
    }

    /* Parse each packet in the file and check the result*/
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
    free(out_file);
    return EXIT_SUCCESS;
}
