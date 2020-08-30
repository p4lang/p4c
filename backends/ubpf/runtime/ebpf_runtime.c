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

#include <unistd.h>     // getopt()
#include <ctype.h>      // isprint()
#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#ifdef CONTROL_PLANE
#include "control.h"
#endif
#include "../../ebpf/runtime/pcap_util.h"

#define PCAPIN  "_in.pcap"
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
    RUN(entry, pcap_base, num_pcaps, input_list, debug);
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
    if (!pcap_name || num_pcaps == -1)
        usage(argv[0]);

#ifdef CONTROL_PLANE
    init_tables();
    /* Run all commands specified in the control file */
    setup_control_plane();
#endif

    launch_runtime(pcap_name, num_pcaps);

    return EXIT_SUCCESS;
}
