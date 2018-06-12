#include <string.h>
#include <libgen.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "test.h"

#define PATH_MAX 4096

void usage() {
    printf("Missing input trace file. Expected .pcap file as input\n");
}

int main(int argc, char **argv) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    const unsigned char *packet;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pcap_hdr;
    static char out_file[PATH_MAX];
    /* Skip over the program name. */
    ++argv; --argc;
    if (argc != 1) {
        usage();
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
        fprintf(stderr, "Error: Failed to read pcap file: %s\n", errbuf);
        return EXIT_FAILURE;
    }

    /* Create the output file. */
    char *in_file = strdup(argv[0]);
    char *in_dir = dirname(in_file);
    memcpy(out_file, in_dir, strlen(in_dir));
    memcpy(out_file + strlen(in_dir), "/pcap0_out.pcap", strlen("/pcap0_out.pcap"));
    out_handle = pcap_dump_open(in_handle, out_file);
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
        // if (!result)  for now we do not check for the result
        pcap_dump((unsigned char *) out_handle, pcap_hdr, packet);
        printf("\nResult of the eBPF parsing is: %d\n", result );
    }
    pcap_close(in_handle);
    pcap_dump_close(out_handle);
    return EXIT_SUCCESS;
}

