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

/* The runtime emulates a loaded eBPF program. It parses a set of given pcap files
 and exposes an interface to feed packets to the emulated eBPF filter. */

#include <unistd.h>     // getopt
#include <ctype.h>      // isprint
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "test.h"
#ifdef CONTROL_PLANE
#include "control.h"
#endif

#define FILE_NAME_MAX 256
#define PCAPIN  "_in.pcap"
#define PCAPOUT "_out.pcap"
#define DELIM   "*"

static int debug = 0;

void usage(char *name) {
    fprintf(stderr, "This program expects a pcap file pattern, "
            "extracts all the packets out of the matched files,"
            "then feeds the individual packets into a filter function, "
            "and returns the output.\n");
    fprintf(stderr, "Usage: %s [-d] -f file.pcap [-n num_files]\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-d: Turn on debug messages\n");
    fprintf(stderr, "\t-f: The input pcap file\n");
    fprintf(stderr, "\t-n: Specifies the number of input files\n");
    exit(EXIT_FAILURE);
}

/* Struct containing packet content and its metadata.
 For now this is only the pcap header and the input interface.
 */
typedef struct {
    char *data;
    struct pcap_pkthdr pcap_hdr;
    uint32_t ifindex;
} pcap_pkt;

/* Carries a list of packets and the total length as information.
 Used as input stream for the packet parsing interface.
 */
typedef struct {
    pcap_pkt *list;
    uint64_t len;
} pcap_pkt_list;

/* Rank packets based on the timestamp of the pcap header */
int compare(const void *s1, const void *s2) {
  pcap_pkt *p1 = (pcap_pkt *)s1;
  pcap_pkt *p2 = (pcap_pkt *)s2;
  return p1->pcap_hdr.ts.tv_usec > p2->pcap_hdr.ts.tv_usec;
}

static pcap_pkt_list *sort_packets(pcap_pkt_list *pkt_lists, int num_lists) {
    /* Get the total length of all individual lists combined first */
    uint64_t total_len = 0;
    for (int i = 0; i < num_lists; i++)
        total_len+= pkt_lists[i].len;
    /* Allocate the "master" list */
    pcap_pkt_list *sorted_list = malloc(sizeof(pcap_pkt_list));
    pcap_pkt *full_pkts = malloc(sizeof(pcap_pkt) * total_len);

    /* Fill the master list by copying over the individual packets */
    uint64_t offset = 0;
    for (int i = 0; i < num_lists; i++) {
        memcpy(&full_pkts[offset], pkt_lists[i].list, sizeof(pcap_pkt) *pkt_lists[i].len);
        offset += pkt_lists[i].len;
        /* We do not need the temporary allocated list anymore */
        free(pkt_lists[i].list);
    }
    /* Sort the master list and return the meta information*/
    qsort(full_pkts, total_len, sizeof(pcap_pkt), compare);
    sorted_list->list = full_pkts;
    sorted_list->len = total_len;
    return sorted_list;
}

static void fill_packets(pcap_pkt_list *pkt_list, int num_files, pcap_t **in_handles) {
    /* Extract all pcap packets per output file */
    for (int i = 0; i < num_files; i++) {
        struct pcap_pkthdr *pcap_hdr;
        const unsigned char *packet;
        char errbuf[PCAP_ERRBUF_SIZE];
        int ret;
        FILE *pcap_in_fp = pcap_file(in_handles[i]);
        uint64_t pkt_index = 0;
        /* Count packets in the pcap file for allocation*/
        while ((ret = pcap_next_ex(in_handles[i], &pcap_hdr, &packet)) == 1)
            pkt_index++;
        if (ret == -1) {
            pcap_perror(in_handles[i], "Error: Failed to parse ");
            exit(EXIT_FAILURE);
        }
        /* Reset the file pointer and reopen the pcap handle */
        fseek(pcap_in_fp, 0, 0);
        in_handles[i] = pcap_fopen_offline(pcap_in_fp, errbuf);
        if (in_handles[i] == NULL) {
            fprintf(stderr, "Error: Failed to reopen pcap file!\n");
            exit(EXIT_FAILURE);
        }
        /* Allocate a list of packets and reset the index */
        pcap_pkt *pkts = malloc(sizeof(pcap_pkt) * pkt_index);
        pkt_index = 0;
        /* Fill the packet list while incrementing the counter */
        while ((ret = pcap_next_ex(in_handles[i], &pcap_hdr, &packet)) == 1) {
            /* Save the data we extracted from the pcap buffer */
            pkts[pkt_index].data = malloc(pcap_hdr->len);
            memcpy(pkts[pkt_index].data, packet, pcap_hdr->len);
            /* Also save the header and the interface "index" */
            pkts[pkt_index].pcap_hdr = *pcap_hdr;
            pkts[pkt_index].ifindex = i;
            pkt_index++;
        }
        if (ret == -1) {
            pcap_perror(in_handles[i], "Error: Failed to parse ");
            exit(EXIT_FAILURE);
        }
        /* Store the pointer to the list in the packet container */
        pkt_list[i].list = pkts;
        pkt_list[i].len = pkt_index;
    }
}

static void free_pkt_list(pcap_pkt_list *pkt_list) {
    for (uint64_t i = 0; i < pkt_list->len; i++) {
        free(pkt_list->list[i].data);
    }
    free(pkt_list->list);
    free(pkt_list);
}

static pcap_pkt_list *get_sorted_pkts(pcap_t **in_handles, int num_files) {
    pcap_pkt_list pcap_pkt_lists[num_files];
    /* Fill the packet lists with packets from the pcap files */
    fill_packets(pcap_pkt_lists, num_files, in_handles);
    /* Merge, sort, and return the list of the parsed packets */
    return sort_packets(pcap_pkt_lists, num_files);
}

static void feed_packets(pcap_dumper_t **out_handles, pcap_pkt_list *pkt_list, int num_pcaps) {
    for (uint64_t i = 0; i < pkt_list->len; i++) {
        struct pcap_pkthdr *pcap_hdr = &pkt_list->list[i].pcap_hdr;
        pcap_dumper_t *out_handle = out_handles[pkt_list->list[i].ifindex];
        /* Parse each packet in the file and check the result */
        struct sk_buff skb;
        skb.data = (void *) pkt_list->list[i].data;
        skb.len = pkt_list->list[i].pcap_hdr.len;
        int result = ebpf_filter(&skb);
        if (result)
            pcap_dump((unsigned char *)out_handle, pcap_hdr, skb.data);
        if (debug)
            printf("\nResult of the eBPF parsing is: %d\n", result);
    }
}

static void init_output_handles(char *first, char *second, pcap_t **in_handles, pcap_dumper_t **out_handles, int num_files) {
    for (int i = 0; i < num_files; i++) {
        char pcap_out_name[FILE_NAME_MAX];
        memset(pcap_out_name, 0, FILE_NAME_MAX);
        if (second !=NULL) {
            snprintf(pcap_out_name, FILE_NAME_MAX, "%s%d"PCAPOUT, first, i);
            /* We actually matched, create the filename */
        } else {
            /* No delimiter, try to create the file by replacing the input pattern */
            char *replace = strstr(first, PCAPIN);
            int offset =(replace - first);
            memcpy(pcap_out_name, first, offset);
            memcpy(&pcap_out_name[offset], PCAPOUT, strlen(PCAPOUT));
        }
        if (debug)
            printf("Opening output file: %s\n", pcap_out_name);
        /* Open pcap output file and store its pointer in the handles */
        out_handles[i] = pcap_dump_open(in_handles[i], pcap_out_name);
        if (out_handles[i] == NULL) {
            pcap_perror(in_handles[i], "Error: Failed to create pcap output file ");
            pcap_close(in_handles[i]);
            exit(EXIT_FAILURE);
        }
    }
}

static void init_input_handles(char *first, char *second, pcap_t **in_handles, int num_files) {
    char errbuf[PCAP_ERRBUF_SIZE];
    for (int i = 0; i < num_files; i++) {
        char pcap_in_name[FILE_NAME_MAX];
        memset(pcap_in_name, 0, FILE_NAME_MAX);
        if (second !=NULL)
            /* We actually matched, create the filename */
            snprintf(pcap_in_name, FILE_NAME_MAX, "%s%d"PCAPIN, first, i);
        else
            /* Just copy the filename into the buffer */
            memcpy(pcap_in_name, first, FILE_NAME_MAX);
        if (debug)
            printf("Processing input file: %s\n", pcap_in_name);
        if (access( pcap_in_name, F_OK) == -1) {
            fprintf(stderr,"File %s does not exist!\n",  pcap_in_name);
            exit(EXIT_FAILURE);
        }
        /* Open pcap input file and store its pointer in the handles */
        in_handles[i] = pcap_open_offline(pcap_in_name, errbuf);
        if (in_handles[i] == NULL) {
            fprintf(stderr, "Error: Failed to read pcap file %s \n", errbuf);
            exit(EXIT_FAILURE);
        }
    }
}

static void init_pcap_handles(char *pcap_base, pcap_t **in_handles, pcap_dumper_t **out_handles, int num_files) {
    /* Store first half until DELIM */
    char *first = strtok(pcap_base, DELIM);
    /* Use the rest of the string to check if we actually matched */
    char *second = strtok(NULL, DELIM);
    /* Init input handles */
    init_input_handles(first, second, in_handles, num_files);
    /* Init output handles */
    init_output_handles(first, second, in_handles, out_handles, num_files);
}

static void close_pcap_handles(pcap_t **in_handles, pcap_dumper_t **out_handles, int num_files) {
    for (int i = 0; i < num_files; i++) {
        pcap_close(in_handles[i]);
        pcap_dump_close(out_handles[i]);
    }
}

int main(int argc, char **argv) {
    char *base_pcap = NULL;
    int num_pcaps = 0;
    int c;
    opterr = 0;

    while ((c = getopt (argc, argv, "dn:f:")) != -1) {
        switch (c) {
            case 'd':
            debug = 1;
            break;
            case 'n':
            num_pcaps = (int)strtol(optarg, (char **)NULL, 10);
            break;
            case 'f':
            base_pcap = optarg;
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
    if (!base_pcap) {
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
    initialize_tables();
    /* Run all commands specified in the control file */
    run_generated_cmds();
#endif

    /* Initialize the pcap files based on the provided pattern. */
    pcap_dumper_t *out_handles[num_pcaps];
    pcap_t *in_handles[num_pcaps];
    init_pcap_handles(base_pcap, in_handles, out_handles, num_pcaps);
    /* Retrieve a sorted list of packets from all files. */
    pcap_pkt_list *pkt_list = get_sorted_pkts(in_handles, num_pcaps);
    /* Feed the files into our loaded program. */
    feed_packets(out_handles, pkt_list, num_pcaps);
    /* Cleanup. */
    close_pcap_handles(in_handles, out_handles, num_pcaps);
    free_pkt_list(pkt_list);
    return EXIT_SUCCESS;
}
