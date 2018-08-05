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


#define FILE_NAME_MAX 256
#define MAX_10_UINT16 5
#define PCAPOUT "_out.pcap"

/**
 * @brief Create a pcap file name from a given base name, interface index,
 * and suffix. Return value must be deallocated after usage.
 * @param pcap_base The file base name.
 * @param index The index of the file, represent an interface.
 * @param suffix  Filename suffix (e.g., _in.pcap)
 * @return An allocated string containing the file name.
 */
char *generate_pcap_name(const char *pcap_base, int index, const char *suffix) {
    /* Dynamic string length plus max decimal representation of uint16_t */
    int file_length = strlen(pcap_base) + strlen(suffix) + MAX_10_UINT16 + 1;
    char *pcap_name = malloc(file_length);
    int offset = snprintf(pcap_name, file_length,"%s%d%s",
                    pcap_base, index, suffix);
    if (offset >= FILE_NAME_MAX) {
        fprintf(stderr, "Name %s%d%s too long.\n", pcap_base, index, suffix);
        exit(EXIT_FAILURE);
    }
    return pcap_name;
}

void run_tcpdump(char *filename, char *interface) {
    int len = 64 + strlen(filename) + strlen(interface);
    char *cmd = malloc(len);
    /* Write to file "filename" and listen on interface "interface" */
    snprintf(cmd, len, "tcpdump -w %s -i %s &", filename, interface);
    int ret = system(cmd);
    if (ret < 0)
        perror("tcpdump failed:");
    free(cmd);
}

void kill_tcpdump() {
    int ret = system("pkill --ns $$ tcpdump");
    if (ret < 0)
        perror("killing tcpdump failed:");
}

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
    for (int i = 0; i < num_pcaps; i++) {
        char iface_name[MAX_10_UINT16 + 1];
        snprintf(iface_name, MAX_10_UINT16, "%d", i);
        sockfds[i] = open_socket(iface_name);
        /* Start up tcpdump while we open each socket */
        run_tcpdump(generate_pcap_name(pcap_base, i, PCAPOUT), iface_name);
    }
    /* Wait a bit to let tcpdump initialize */
    sleep(2);
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
    /* Wait a bit to let tcpdump finish recording */
    sleep(2);
    close_sockets(sockfds, num_pcaps);
    kill_tcpdump();
}