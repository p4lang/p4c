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

#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // memcpy()
#include "pcap_util.h"

#define DLT_EN10MB 1        // Ethernet Link Type, see also 'man pcap-linktype'


/* Dynamically-allocated list of packets.
 */
struct pcap_list {
    pcap_pkt **pkts;
    uint32_t len;
};

/* An array of lists of packets */
struct pcap_list_array {
    pcap_list_t **lists;
    uint16_t len;
};

pcap_list_t *append_packet(pcap_list_t *pkt_list, pcap_pkt *pkt) {
    if (!pkt_list)
        /* If the list is not allocated yet, create it */
        pkt_list = allocate_pkt_list();
    pkt_list->len++;
    pkt_list->pkts = realloc(pkt_list->pkts, pkt_list->len * sizeof(pcap_pkt *));
    if (pkt_list->pkts == NULL) {
        fprintf(stderr, "Fatal: Failed to expand the"
            "packet list with size %u !\n", pkt_list->len);
        exit(EXIT_FAILURE);
    }
    pkt_list->pkts[pkt_list->len - 1] = pkt;
    return pkt_list;
}

pcap_list_array_t *insert_list(pcap_list_array_t *pkt_array, pcap_list_t *pkt_list, uint16_t index) {
    if (!pkt_array)
        /* If the array is not allocated yet, create it */
        pkt_array = allocate_pkt_list_array();
    if (index >= pkt_array->len) {
        pkt_array->lists = realloc(pkt_array->lists,
            (index + 1) * sizeof(pcap_list_t *));
        if (pkt_array->lists == NULL) {
            fprintf(stderr, "Fatal: Failed to expand the "
                "list array with size %u !\n", pkt_array->len);
            exit(EXIT_FAILURE);
        }
        /* Set the newly allocated space to zero */
        for (int i = pkt_array->len; i <= index; i++)
            pkt_array->lists[i] = NULL;
        pkt_array->len = index + 1;
    }
    pkt_array->lists[index] = pkt_list;
    return pkt_array;
}

pcap_pkt *get_packet(pcap_list_t *list, uint32_t index) {
    if (list->len < index) {
        fprintf(stderr, "Index %u exceeds list size %u!\n", index, list->len);
        return NULL;
    }
    return list->pkts[index];
}

pcap_list_t *get_list(pcap_list_array_t *array, uint16_t index) {
    if (array->len < index) {
        fprintf(stderr, "Index %u exceeds array size %u!\n", index, array->len);
        return NULL;
    }
    return array->lists[index];
}

uint32_t get_pkt_list_length(pcap_list_t *pkt_list) {
    return pkt_list->len;
}

uint16_t get_list_array_length(pcap_list_array_t *pkt_list_array) {
    return pkt_list_array->len;
}


pcap_list_t *allocate_pkt_list() {
    pcap_list_t *pkt_list = calloc(1, sizeof(pcap_list_t));
    return pkt_list;
}

pcap_list_array_t *allocate_pkt_list_array() {
    pcap_list_array_t *pkt_list_arr = calloc(1, sizeof(pcap_list_array_t));
    return pkt_list_arr;
}

void delete_list(pcap_list_t *pkt_list) {
    for(uint32_t i = 0; i < pkt_list->len; i++) {
        free(pkt_list->pkts[i]->data);
        /* Set the data pointer to NULL, to mitigate duplicate frees */
        pkt_list->pkts[i]->data = NULL;
        free(pkt_list->pkts[i]);
    }
    free(pkt_list->pkts);
    free(pkt_list);
}

void delete_array(pcap_list_array_t *pkt_list_array) {
    for(uint32_t i = 0; i < pkt_list_array->len; i++)
        if (pkt_list_array->lists[i])
            delete_list(pkt_list_array->lists[i]);
    free(pkt_list_array->lists);
    free(pkt_list_array);
}

pcap_list_t *read_pkts_from_pcap(const char *pcap_file_name, iface_index index) {
    struct pcap_pkthdr *pcap_hdr;
    const unsigned char *tmp_pkt;
    char errbuf[PCAP_ERRBUF_SIZE];
    int ret;
    pcap_t *in_handle;
    in_handle = pcap_open_offline(pcap_file_name, errbuf);
    if (in_handle == NULL) {
        fprintf(stderr, "Failed to open pcap file! %s \n", pcap_file_name );
        perror("pcap_open_offline");
        return NULL;
    }
    pcap_list_t *pkt_list = allocate_pkt_list();
    /* Fill the packet list with packets */
    while ((ret = pcap_next_ex(in_handle, &pcap_hdr, &tmp_pkt)) == 1) {
        /* Save the data we extracted from the pcap buffer */
        pcap_pkt *pkt = calloc(1, sizeof(pcap_pkt));
        pkt->data = calloc(pcap_hdr->len, 1);
        memcpy(pkt->data, tmp_pkt, pcap_hdr->len);
        /* Also save the header and the interface "index" */
        pkt->pcap_hdr = *pcap_hdr;
        pkt->ifindex = index;
        pkt_list = append_packet(pkt_list, pkt);
    }
    if (ret == -1)
        pcap_perror(in_handle, "Error: Failed to parse data");
    pcap_close(in_handle);
    return pkt_list;
}

int write_pkts_to_pcap(const char *pcap_file_name, const pcap_list_t *list) {
    pcap_t *in_handle;
    pcap_dumper_t *out_handle;
    in_handle = pcap_open_dead(DLT_EN10MB, UINT16_MAX);
    if (in_handle == NULL) {
        fprintf(stderr, "Error: Failed to open pcap file!\n");
        return EXIT_FAILURE;
    }
    /* Open pcap output file and store its pointer in the handles */
    out_handle = pcap_dump_open(in_handle, pcap_file_name);
    if (out_handle == NULL) {
        pcap_perror(in_handle, "Error: Failed to create pcap output file ");
        pcap_close(in_handle);
        return EXIT_FAILURE;
    }
    for (uint32_t i = 0; i < list->len; i++)
        pcap_dump((unsigned char *)out_handle,
         &list->pkts[i]->pcap_hdr, list->pkts[i]->data);
    pcap_close(in_handle);
    pcap_dump_close(out_handle);
    return EXIT_SUCCESS;
}

pcap_list_t *merge_and_delete_lists(pcap_list_array_t *array, pcap_list_t *merged_list) {
    /* Fill the master list by copying over the individual packet descriptors */
    for (uint32_t i = 0; i < array->len; i++) {
            for (uint32_t j = 0; j < array->lists[i]->len; j++)
                merged_list = append_packet(
                    merged_list, array->lists[i]->pkts[j]);
        /* We do not need the previous list anymore */
        free(array->lists[i]->pkts);
        free(array->lists[i]);
    }
    /* Destroy the input array (but keep its data) */
    free(array->lists);
    free(array);
    return merged_list;
}

pcap_list_array_t *split_and_delete_list(pcap_list_t *input_list, pcap_list_array_t *result_arr) {
    if (input_list->len == 0)
        return result_arr;

    /* Find the maximum interface value in the list */
    uint16_t max_index = 0;
    for (uint32_t i = 0; i < input_list->len; i++)
        if (input_list->pkts[i]->ifindex > max_index)
            max_index = input_list->pkts[i]->ifindex;

    /* Allocate as many lists as the maximum index */
    for (int i = 0; i <= max_index; i++) {
        pcap_list_t *pkt_list = allocate_pkt_list();
        result_arr = insert_list(result_arr, pkt_list, i);
    }

    /* Fill each list with its respective packets */
    for (uint32_t i = 0; i < input_list->len; i++)
        append_packet(result_arr->lists[input_list->pkts[i]->ifindex], input_list->pkts[i]);

    /* Destroy the input list (but keep its data) */
    free(input_list->pkts);
    free(input_list);
    return result_arr;
}

pcap_pkt *copy_pkt(pcap_pkt *src_pkt) {
    pcap_pkt *new_pkt = malloc(sizeof(pcap_pkt));
    uint32_t datalen = src_pkt->pcap_hdr.len;
    new_pkt->data = malloc(datalen);
    memcpy(new_pkt->data, src_pkt->data, datalen);
    new_pkt->pcap_hdr = src_pkt->pcap_hdr;
    new_pkt->ifindex = src_pkt->ifindex;
    return new_pkt;
}

/* Rank packets based on the timestamp of the pcap header */
static int compare_pkt_time(const void *s1, const void *s2) {
  pcap_pkt *p1 = *(pcap_pkt **)s1;
  pcap_pkt *p2 = *(pcap_pkt **)s2;
  return p1->pcap_hdr.ts.tv_usec - p2->pcap_hdr.ts.tv_usec;
}

void sort_pcap_list(pcap_list_t *pkt_list) {
    /* Sort the master list and return the meta information*/
    qsort(pkt_list->pkts, pkt_list->len, sizeof(pcap_pkt *), compare_pkt_time);
}

char *generate_pcap_name(const char *pcap_base, int index, const char *suffix) {
    int max_10_uint32  = 10;     // Max size of uint32 in base 10
    /* Dynamic string length plus max decimal representation of uint16_t */
    int file_length = strlen(pcap_base) + strlen(suffix) + max_10_uint32 + 1;
    char *pcap_name = malloc(file_length);
    int offset = snprintf(pcap_name, file_length,"%s%hu%s",
                    pcap_base, index, suffix);
    if (offset >= file_length) {
        fprintf(stderr, "Name %s%d%s too long.\n", pcap_base, index, suffix);
        exit(EXIT_FAILURE);
    }
    return pcap_name;
}