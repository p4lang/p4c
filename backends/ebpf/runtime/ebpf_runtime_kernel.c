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

#include <unistd.h>     // getopt()
#include <ctype.h>      // isprint()
#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <sys/time.h>
#include <net/if.h>
#include "ebpf_kernel.h"
#include "ebpf_runtime_kernel.h"


int open_socket(char *iface_name) {
    struct sockaddr_ll iface;
    int sockfd;
    memset (&iface, 0, sizeof (iface));
    if ((iface.sll_ifindex = if_nametoindex (iface_name)) == 0) {
        perror ("if_nametoindex() failed to obtain interface index ");
        exit (EXIT_FAILURE);
    }
    iface.sll_family = AF_PACKET;
    iface.sll_halen = 6;
    iface.sll_hatype = ARPHRD_VOID;
    sockfd = socket(AF_PACKET, SOCK_RAW|SOCK_NONBLOCK, htons(ETH_P_ALL));
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr *)&iface, sizeof(iface))== -1) {
        perror("Send: Could not bind socket.");
        exit(1);
    }
    return sockfd;
}

void close_sockets(int *sockfds, int num_pcaps) {
    for (int i = 0; i < num_pcaps; i++)
        close(sockfds[i]);
    free(sockfds);
}

int *init_sockets(char *pcap_base, uint16_t num_pcaps){
    int *sockfds = malloc(sizeof(int) * num_pcaps);
    int max_10_uint16  = 5;     // Max size of uint16 in base 10
    for (uint16_t i = 0; i < num_pcaps; i++) {
        char iface_name[max_10_uint16 + 1];
        snprintf(iface_name, max_10_uint16, "%hu", i);
        sockfds[i] = open_socket(iface_name);
    }
    return sockfds;
}

void feed_packets(int *sockfds, pcap_list_t *pkt_list) {
    uint32_t list_len = get_pkt_list_length(pkt_list);
    for (uint32_t i = 0; i < list_len; i++) {
        pcap_pkt *input_pkt = get_packet(pkt_list, i);
        int sockfd = sockfds[input_pkt->ifindex];
        char *data = input_pkt->data;
        int len = input_pkt->pcap_hdr.len;
        if (sendto(sockfd, data, len, 0, 0, sizeof(struct sockaddr_ll)) < 0)
            perror("sendto failed");
    }
}

void run_and_record_output(pcap_list_t *pkt_list, char *pcap_base, uint16_t num_pcaps, int debug) {
    int *sockfds = init_sockets(pcap_base, num_pcaps);
    feed_packets(sockfds, pkt_list);
    close_sockets(sockfds, num_pcaps);
    /* Sleep a bit to allow the remaining processes to finish */
    sleep(2);
}