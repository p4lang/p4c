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
 * Allocates a packet list and fills it with the packets from the
 * supplied pcap file. All data in the capture file is copied to the new list.
 * Each packet is assigned the given interface index as meta-information.
 * A list allocated by this function should subsequently be freed by
 * delete_list().
 *
 * @param pcap_file_name The exact name of the pcap file.
 * @param index Interface index of the file.
 *
 * @return A handle to the allocated list. Null if the read operation
 * was unsuccessful.
 */
pcap_list_t *read_pkts_from_pcap(const char *pcap_file_name, iface_index index);

/**
 * @brief Write a list of packets to a pcap file.
 * @details Iteratively dumps the packets to the given filename.
 *
 * @param pcap_file_name Exact name of the file to create and write to.
 * @param pkt_list List of packets to write.
 *
 * @return EXIT_FAILURE if operation fails.
 */
int write_pkts_to_pcap(const char *pcap_file_name, const pcap_list_t *pkt_list);

/**
 * @brief Merges a given list array into a single list.
 * @details Expects a handle to an array list and allocates a single list from
 * it. The array and its data structures are subsequently destroyed.
 *
 * @param array The array containing the lists to split
 * @param merged_list The output lists to fill with packets.
 * @return A single merged list contained all packets. Null if the operation
 * the fails.
 */
pcap_list_t *merge_and_delete_lists(pcap_list_array_t *array, pcap_list_t *merged_list);

/**
 * @brief Splits a list of packets by interface.
 * @details Splits the given list by the interface indicator in the
 * packet structure. The input list is destroyed and for each interface a
 * new list is allocated.
 *
 * @param input_list A list containing packets.
 * @param result_arr The array to fill with lists.

 * @return The updated array containing the new lists.
 */
pcap_list_array_t *split_and_delete_list(pcap_list_t *input_list, pcap_list_array_t *result_arr);

/**
 * @brief Appends a  packet to a given list of packets.
 * @details This function takes a pointer to a packet and appends it to the
 * given list. If the list is Null, it is allocated. For each new packet
 * to append, the list is expanded by the size of the packet pointer.
 *
 * @param pkt_list List descriptor. Can be Null
 * @param pkt Pointer of the packet to append.
 *
 * @return Returns the updated list. This function causes an
 * exit if allocation fails.
 */
pcap_list_t *append_packet(pcap_list_t *pkt_list, pcap_pkt *pkt);

/**
 * @brief Inserts a packet list in the array at a given index.
 * @details This function takes a pointer to a packet list and inserts it at
 * the given index. If the list array is Null, it is allocated. For each index
 * out of the boundaries of the array, the array list is expanded by the size
 * of the index.
 *
 * @param pkt_array The descriptor of the array. Can be Null
 * @param pkt_list The list to insert.
 * @param index The index specifying where to insert the array.
 * @return Returns the updated list array. This function causes
 * an exit if reallocation fails.
 */
pcap_list_array_t *insert_list(pcap_list_array_t *pkt_array, pcap_list_t *pkt_list, uint16_t index);

/**
 * @brief Allocates a packet list.
 *
 * @return The packet list.
 */
pcap_list_t *allocate_pkt_list();


/**
 * @brief Allocates a packet list array.
 *
 * @return The list array.
 */
pcap_list_array_t *allocate_pkt_list_array();

/**
 * @brief Get the length of the packet list.
 *
 * @param pkt_list A packet list.
 * @return Length of the list.
 */
uint32_t get_pkt_list_length(pcap_list_t *pkt_list);

/**
 * @brief Get the length of the packet list array.
 *
 * @param pkt_list A list array.
 * @return Length of the list array.
 */
uint16_t get_list_array_length(pcap_list_array_t *pkt_list_array);

/**
 * @brief Get a packet from a given index.
 *
 * @param pkt_list A list.
 * @param index Index of the packet to retrieve.
 *
 * @return The list. Null if the index is out of bounds.
 */
pcap_pkt *get_packet(pcap_list_t *list, uint32_t index);

/**
 * @brief Get a list from a given index.
 *
 * @param pkt_list A list array.
 * @param index Index of the list to retrieve.
 *
 * @return The list array. Null if the index is out of bounds.
 */
pcap_list_t *get_list(pcap_list_array_t *list, uint16_t index);

/**
 * @brief Copies a packet in full, including its referenced data.
 * @details Allocates a new packet and copies all data and structures
 * contained in the reference packet. Both packets remain allocated.
 * @param src_pkt The packet to copy.
 *
 * @return An allocated copy of the packet.
 */
pcap_pkt *copy_pkt(pcap_pkt *src_pkt);

/**
 * @brief Completely erases the list.
 * @details Deletes the given list. Also deletes the data referenced by the
 * pcap packets in the list.
 *
 * @param pkt_list The list to delete.
 */
void delete_list(pcap_list_t *pkt_list);

/**
 * @brief Completely erases a list array and the lists in the array.
 * @details Deletes the given array. Also deletes the list as well as the
 * data referenced by the pcap packets in the list.
 *
 * @param pkt_list_array A list array.
 */
void delete_array(pcap_list_array_t *pkt_list_array);

/**
 * @brief Sort a list in place.
 * @details Sorts a given list by the timestamp of the packets contained in it.
 * This alters the order in place, the input list is permanently modified.
 *
 * @param pkt_list A list.
 */
void sort_pcap_list(pcap_list_t *pkt_list);

/**
 * @brief Create a pcap file name from a given base name, interface index,
 * and suffix. Return value must be deallocated after usage.
 * @param pcap_base The file base name.
 * @param index The index of the file, represent an interface.
 * @param suffix  Filename suffix (e.g., _in.pcap)
 * @return An allocated string containing the file name.
 */
char *generate_pcap_name(const char *pcap_base, int index, const char *suffix);

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_PCAP_UTIL_H_