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

/*
 * A pcap utility library which provides several functions to read from and
 * write packets to pcap files.
 */

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_PCAP_UTIL_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_PCAP_UTIL_H_

#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include <stdint.h>     // uint32_t, uint16_t

/* Interfaces are named by integers */
typedef uint16_t iface_index;

/* A network packet.
   Contains packet content, timestamp and the interface where
   the packet has been received/sent.
 */
typedef struct {
    char *data;
    struct pcap_pkthdr pcap_hdr;
    iface_index ifindex;
} pcap_pkt;

struct pcap_list;
struct pcap_list_array;
typedef struct pcap_list pcap_list_t;
typedef struct pcap_list_array pcap_list_array_t;

/**
 * @brief Retrieve packets from a pcap file.
 * @details Retrieves a list of packets from a given pcap file.
 * This function allocates a pcap_list handle and fills it
 * iteratively with the packets contained in the capture. All data in the
 * capture file is copied to the new list.
 * Each packet is assigned the given interface index as meta-information.
 * A list allocated by this function should subsequently be freed by
 * delete_list().
 *
 * @param pcap_file_name The exact name of the pcap file.
 * @param index Interface index of the file.
 *
 * @return A handle to the allocated list. NULL if the read operation
 * was unsuccessful.
 */
pcap_list_t *read_pkts_from_pcap(const char *pcap_file_name, iface_index index);

/**
 * @brief Write a list of packets to a pcap file.
 * @details Opens a "dead" pcap handle as Ethernet device and
 * iteratively dumps the packets to the given filename.
 *
 * @param pcap_file_name Exact name of the file to create and write to.
 * @param pkt_list List of packets to write.
 *
 * @return EXIT_FAILURE if operation fails.
 */
int write_pkts_to_pcap(const char *pcap_file_name, const pcap_list_t *pkt_list);

/**
 * @brief Merges a given list array into a single list.
 * @details Expects a handle to an array list and allocates a single cohesive
 * list from it. The array and its data structures are subsequently destroyed.
 *
 *
 * @param array The array containing n lists.
 * @return A single merged list contained all packets. NULL if the operation
 * the fails.
 */
pcap_list_t *merge_pcap_lists(pcap_list_array_t *array);

/**
 * @brief Returns an array of lists from a single list split by interface.
 * @details Splits the given list by the interface indicator in the
 * packet structure. The input list is destroyed and for each interface a
 * new list is allocated.
 *
 * @param pkt_list A list containing packets.
 * @return Array containing the new lists.
 */
pcap_list_array_t *split_list_by_interface(pcap_list_t *pkt_list);

/**
 * @brief Appends a reference to a packet to a given list of packets.
 * @details This function takes a pointer to a packet and appends it to the
 * given list. If the list does not exist, it is allocated. For each new packet
 * to append, the list is expanded by the size of the packet pointer.
 *
 * @param pkt_list List handle descriptor. Can be null
 * @param pkt Pointer of the packet to append.
 *
 * @return Returns the updated handle of the list. This function causes an
 * exit if allocation fails.
 */
pcap_list_t *append_packet(pcap_list_t *pkt_list, pcap_pkt *pkt);

/**
 * @brief Appends a reference to a packet list to a given list array.
 * @details This function takes a pointer to a packet list and appends it to the
 * given list. If the list array does not exist, it is allocated. For each new
 * packet to append, the array list is expanded by the size of the packet
 * pointer.
 *
 * @param pkt_array List handle descriptor. Can be null
 * @param pkt_list Pointer of the packet list to append.
 *
 * @return Returns the updated handle of the list array. This function causes
 * an exit if allocation fails.
 */
pcap_list_array_t *append_list(pcap_list_array_t *pkt_array, pcap_list_t *pkt_list);

/**
 * @brief Allocates a packet list and returns the handle.
 *
 * @return The handle to the packet list.
 */
pcap_list_t *allocate_pkt_list();


/**
 * @brief Allocates a packet list and returns the handle.
 *
 * @return The handle to the list array.
 */
pcap_list_array_t *allocate_pkt_list_array();

/**
 * @brief Get the length of the packet list.
 *
 * @param pkt_list Handle to a list.
 * @return Length of the list.
 */
uint32_t get_pkt_list_length(pcap_list_t *pkt_list);

/**
 * @brief Get the length of the packet list array.
 *
 * @param pkt_list Handle to a list array.
 * @return Length of the list array.
 */
uint16_t get_list_array_length(pcap_list_array_t *pkt_list_array);

/**
 * @brief Get the reference to a packet from a given index.
 *
 * @param pkt_list Handle to a list.
 * @param index Index of the packet to retrieve.
 *
 * @return The reference to the list. NULL if the index is out of bounds.
 */
pcap_pkt *get_packet(pcap_list_t *list, uint32_t index);

/**
 * @brief Get the reference to a list from a given index.
 *
 * @param pkt_list Handle to a list array.
 * @param index Index of the list to retrieve.
 *
 * @return The reference to the list array. NULL if the index is out of bounds.
 */
pcap_list_t *get_list(pcap_list_array_t *list, uint16_t index);

/**
 * @brief Completely erases the list.
 * @details Deletes the given list. Also deletes the data referenced by the
 * pcap packets in the list.
 *
 * @param pkt_list Handle to a list.
 */
void delete_list(pcap_list_t *pkt_list);

/**
 * @brief Completely erases a list array and its lists.
 * @details Deletes the given array. Also deletes the list as well as the
 * data referenced by the pcap packets in the list.
 *
 * @param pkt_list Handle to a list array.
 */
void delete_array(pcap_list_array_t *pkt_list_array);

/**
 * @brief Sort a list in place.
 * @details Sorts a given list by the timestamp of the packets contained in it.
 * This alters the order in place, the input list is permanently modified.
 *
 * @param pkt_list Handle to a list.
 */
void sort_pcap_list(pcap_list_t *pkt_list);

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_PCAP_UTIL_H_