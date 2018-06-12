#include <string.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "test.h"

int main(int argc, char **argv) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    const unsigned char *packet;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pcap_hdr;

    /* Skip over the program name. */
    ++argv; --argc;
    if (argc != 1) {
        fprintf(stderr, "Missing input trace file, exiting...\n");
        return EXIT_FAILURE;
    }

    /* Initialize the registry of shared tables */
    struct bpf_map_def* current = tables;
    while (current->name != 0) {
         if (BPF_OBJ_PIN(current, current->name) != 0) {
            fprintf(stderr, "Error: Failed to register table.\n");
            return EXIT_FAILURE;
         }
        current++;
    }

    /* Open and read pcap file */
    in_handle = pcap_open_offline(argv[0], errbuf);
    if (in_handle == NULL) {
        fprintf(stderr, "Error: Failed to read pcap file: %s\n", errbuf);
        return EXIT_FAILURE;
    }

    /* Create the output file. */
    out_handle = pcap_dump_open(in_handle, "out.pcap");
    if (out_handle == NULL) {
        fprintf(stderr, "Error: Failed to create pcap file: %s\n", errbuf);
        pcap_close(in_handle);
        return EXIT_FAILURE;
    }

    /* Parse each packet in the file and check the result*/
    while ((packet = pcap_next(in_handle, pcap_hdr)) != NULL) {
        struct sk_buff skb;
        skb.data = (void *) packet;
        skb.len = pcap_hdr->len;
        int result = ebpf_filter(&skb);
        printf("\nebpf result is: %d\n", result);
        if (!result)
            pcap_dump((unsigned char *) out_handle, pcap_hdr, packet);
    }

    pcap_close(in_handle);
    pcap_dump_close(out_handle);
    return EXIT_SUCCESS;
}

