/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#ifndef P4C_PSA_H
#define P4C_PSA_H

#include <stdbool.h>

typedef __u32 PortId_t;
typedef __u64 Timestamp_t;
typedef __u8  ClassOfService_t;
typedef __u16 CloneSessionId_t;
typedef __u32 MulticastGroup_t;
typedef __u16 EgressInstance_t;

// Instead of using enum we define PSA_PacketPath_t as __u8 to save memory.
typedef __u8 PSA_PacketPath_t;

static const PSA_PacketPath_t NORMAL = 0;  /// Packet received by ingress that is none of the cases below.
static const PSA_PacketPath_t NORMAL_UNICAST = 1;  /// Normal packet received by egress which is unicast
static const PSA_PacketPath_t NORMAL_MULTICAST = 2;  /// Normal packet received by egress which is multicast
static const PSA_PacketPath_t CLONE_I2E = 3;  /// Packet created via a clone operation in ingress, destined for egress.
static const PSA_PacketPath_t CLONE_E2E = 4;  /// Packet created via a clone operation in egress, destined for egress.
static const PSA_PacketPath_t RESUBMIT = 5;  /// Packet arrival is the result of a resubmit operation
static const PSA_PacketPath_t RECIRCULATE = 6;  /// Packet arrival is the result of a recirculate operation

// Instead of using enum we define ParserError_t as __u8 to save memory.
typedef __u8 ParserError_t;
static const ParserError_t NoError = 0;  /// No error.
static const ParserError_t PacketTooShort = 1;  /// Not enough bits in packet for 'extract'.
static const ParserError_t NoMatch = 2;  /// 'select' expression has no matches.
static const ParserError_t StackOutOfBounds = 3;  /// Reference to invalid element of a header stack.
static const ParserError_t HeaderTooShort = 4;  /// Extracting too many bits into a varbit field.
static const ParserError_t ParserTimeout = 5;  /// Parser execution time limit exceeded.
static const ParserError_t ParserInvalidArgument = 6;  /// Parser operation was called with a value
                                                       /// not supported by the implementation

enum PSA_MeterColor_t { RED, GREEN, YELLOW };

/*
 * INGRESS data types
 */
struct psa_ingress_parser_input_metadata_t {
    PortId_t                 ingress_port;  // taken from xdp_md or __sk_buff
    PSA_PacketPath_t         packet_path;   // taken from psa_global_metadata
} __attribute__((aligned(4)));

struct psa_ingress_input_metadata_t {
    // All of these values are initialized by the architecture before
    // the Ingress control block begins executing.
    PortId_t            ingress_port;  // taken from xdp_md or __sk_buff
    PSA_PacketPath_t    packet_path;        // taken from psa_global_metadata
    Timestamp_t         ingress_timestamp;  // taken from bpf helper
    ParserError_t       parser_error;       // local to ingress parser
} __attribute__((aligned(4)));;

struct psa_ingress_output_metadata_t {
    // The comment after each field specifies its initial value when the
    // Ingress control block begins executing.
    MulticastGroup_t         multicast_group;  // 0, set in ingress as global metadata
    PortId_t                 egress_port;      // initial value is undefined, set in ingress as global metadata
    ClassOfService_t         class_of_service; // 0, set in ingress as global metadata
    bool                     clone;            // false, set in ingress/egress as global metadata
    CloneSessionId_t         clone_session_id; // initial value is undefined, set in ingress/egress as global metadata
    bool                     drop;             // true, set in ingress/egress as global metadata
    bool                     resubmit;         // false, local to ingress?
} __attribute__((aligned(4)));

/*
 * EGRESS data types
 */

struct psa_egress_parser_input_metadata_t {
    PortId_t            egress_port;   // taken from xdp_md or __sk_buff
    PSA_PacketPath_t    packet_path;        // taken from psa_global_metadata
} __attribute__((aligned(4)));

struct psa_egress_input_metadata_t {
    ClassOfService_t    class_of_service;  // taken from global metadata
    PortId_t            egress_port;       // taken taken from xdp_md or __sk_buff
    PSA_PacketPath_t    packet_path;       // taken from global metadata
    EgressInstance_t    instance;          // instance comes from the PacketReplicationEngine
                                           // set by PRE as global metadata,
                                           // in Egress taken from global metadata
    Timestamp_t         egress_timestamp;  // taken from bpf_helper
    ParserError_t       parser_error;           // local to egress pipeline
} __attribute__((aligned(4)));

struct psa_egress_output_metadata_t {
    // The comment after each field specifies its initial value when the
    // Egress control block begins executing.
    bool                     clone;         // false, set in egress as global metadata
    CloneSessionId_t         clone_session_id; // initial value is undefined, set in egress as global metadata
    bool                     drop;          // false, set in egress as global metadata
} __attribute__((aligned(4)));

struct psa_egress_deparser_input_metadata_t {
    PortId_t                 egress_port;       // local to egress pipeline, set by in egress
} __attribute__((aligned(4)));

/*
 * Opaque struct to be used to share global PSA metadata fields between eBPF program attached to Ingress and Egress.
 * The size of this struct must be less than 32 bytes.
 */
struct psa_global_metadata {
    PSA_PacketPath_t packet_path;  /// set by eBPF program as helper variable, read by ingress/egress
    EgressInstance_t instance;  /// set by PRE, read by Egress
    __u8             mark;         /// packet mark set by PSA/eBPF programs. Used to differentiate between packets processed by PSA/eBPF from other packets.
    bool             pass_to_kernel;   /// internal metadata, forces sending packet up to kernel stack
} __attribute__((aligned(4)));

struct clone_session_entry {
    __u32 egress_port;
    __u16 instance;
    __u8  class_of_service;
    __u8  truncate;
    __u16 packet_length_bytes;
} __attribute__((aligned(4)));

#endif //P4C_PSA_H
