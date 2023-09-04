/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef P4C_PNA_H
#define P4C_PNA_H

#include <stdbool.h>

// pna.p4 information

typedef __u32 PortId_t;
typedef __u64 Timestamp_t;
typedef __u8  ClassOfService_t;
typedef __u16 CloneSessionId_t;
typedef __u32 MulticastGroup_t;
typedef __u16 EgressInstance_t;
typedef __u8  MirrorSlotId_t;
typedef __u16 MirrorSessionId_t;

// Instead of using enum we define ParserError_t as __u8 to save memory.
typedef __u8 ParserError_t;
static const ParserError_t NoError = 0;          // No error.
static const ParserError_t PacketTooShort = 1;   // Not enough bits in packet for 'extract'.
static const ParserError_t NoMatch = 2;          // 'select' expression has no matches.
static const ParserError_t StackOutOfBounds = 3; // Reference to invalid element of a header stack.
static const ParserError_t HeaderTooShort = 4;   // Extracting too many bits into a varbit field.
static const ParserError_t ParserTimeout = 5;    // Parser execution time limit exceeded.
static const ParserError_t ParserInvalidArgument = 6; // Parser operation was called with a value
                                                      // not supported by the implementation

enum PNA_Source_t { FROM_HOST, FROM_NET };

enum PNA_MeterColor_t { RED, GREEN, YELLOW };

enum MirrorType { NO_MIRROR, PRE_MODIFY, POST_MODIFY };

struct pna_main_parser_input_metadata_t {
    bool                     recirculated;
    PortId_t                 input_port;    // taken from xdp_md or __sk_buff
} __attribute__((aligned(4)));

struct pna_main_input_metadata_t {
    // All of these values are initialized by the architecture before
    // the control block begins executing.
    bool                     recirculated;
    Timestamp_t              timestamp;         // taken from bpf helper
    ParserError_t            parser_error;      // local to parser
    ClassOfService_t         class_of_service;  // 0, set in control as global metadata
    PortId_t                 input_port;
} __attribute__((aligned(4)));;

struct pna_main_output_metadata_t {
    // The comment after each field specifies its initial value when the
    // control block begins executing.
    ClassOfService_t         class_of_service;
} __attribute__((aligned(4)));

/*
 * Opaque struct to be used to share global PNA metadata fields.
 * The size of this struct must be less than 32 bytes.
 */
struct pna_global_metadata {
    bool             recirculated;
    bool             drop; // NOTE : no drop field in PNA metadata, so we keep drop state as internal metadata.
    PortId_t         egress_port;
    enum MirrorType  mirror_type;
    MirrorSlotId_t   mirror_slot_id;
    MirrorSessionId_t mirror_session_id;
    __u8             mark;
    bool             pass_to_kernel; // internal metadata, forces sending packet up to kernel stack
} __attribute__((aligned(4)));

struct clone_session_entry {
    __u32 egress_port;
    __u16 instance;
    __u8  class_of_service;
    __u8  truncate;
    __u16 packet_length_bytes;
} __attribute__((aligned(4)));

#define send_to_port(x) (compiler_meta__->egress_port = x)
#define drop_packet() (compiler_meta__->drop = true)

// structures and functions for tc backend

struct p4tc_table_entry_act_bpf_params__local {
    u32 pipeid;
    u32 tblid;
} __attribute__((preserve_access_index));

struct p4tc_table_entry_act_bpf {
    u32 act_id;
    u8 params[124];
};

extern struct p4tc_table_entry_act_bpf *bpf_skb_p4tc_tbl_read(
    struct __sk_buff *skb, struct p4tc_table_entry_act_bpf_params__local *params, void *key,
    const u32 key__sz) __ksym;
extern struct p4tc_table_entry_act_bpf *bpf_xdp_p4tc_tbl_lookup(
    struct xdp_md *skb, struct p4tc_table_entry_act_bpf_params__local *params, void *key,
    const u32 key__sz) __ksym;

/* Start generic kfunc interface to any extern */
struct p4tc_ext_bpf_params {
    u32 ext_id;
    u8 params[124]; // extern specific params if any
};

struct __attribute__((__packed__)) p4tc_ext_bpf_res {
    u32 ext_id;
    u8 params[124]; // extern specific values if any
};

// Generic api indirection for all externs which would hook into p4tc core code
// or directly into kernel modules that could be written in C/RUST by user
extern struct p4tc_ext_bpf_res *
bpf_skb_p4tc_run_extern(struct __sk_buff *skb,
                        struct p4tc_ext_bpf_params *params) __ksym;
extern struct p4tc_ext_bpf_res *
bpf_xdp_p4tc_run_extern(struct xdp_md *skb,
                        struct p4tc_ext_bpf_params *params) __ksym;

/* end generic kfunc interface to any extern */

/* per extern specifics start */

/* in this case it is PNA so  we have these helpers like below
   "is_net_port_skb" but for user specific externs the caller should
    be doing similar flow:
    a) populating p4tc_ext_bpf_params struct with their proper
    parametrization  of pipeline id, extern id, and extern specific params
    b) receiving a response and retrieving their extern-specific data/return
    codes from res->params
*/

#define EXTERN_IS_NET_PORT 1234
#define EXTERN_IS_HOST_PORT 4567

static bool is_net_port_skb(struct __sk_buff *skb, u32 ifindex)
{
    struct p4tc_ext_bpf_params param = {};
    u32 *index = (u32 *)param.params;
    struct p4tc_ext_bpf_res *res;
    u32 *direction;

    param.ext_id = EXTERN_IS_NET_PORT;
    *index = ifindex;

    res = bpf_skb_p4tc_run_extern(skb, &param);
    /* NULL means if index wasn't found, and this will default to host */
    if (!res)
        return false;

    direction = (u32 *)res->params;

    return *direction == FROM_NET;
}

static bool is_host_port_skb(struct __sk_buff *skb, u32 ifindex)
{
    struct p4tc_ext_bpf_params param = {};
    u32 *index = (u32 *)param.params;
    struct p4tc_ext_bpf_res *res;
    u32 *direction;

    param.ext_id = EXTERN_IS_HOST_PORT;
    *index = ifindex;

    res = bpf_skb_p4tc_run_extern(skb, &param);
    /* NULL means if index wasn't found, and this will default to host */
    if (!res)
        return true;

    direction = (u32 *)res->params;

    return *direction == FROM_HOST;
}

static bool is_net_port_xdp(struct xdp_md *skb, u32 ifindex)
{
    struct p4tc_ext_bpf_params param = {};
    u32 *index = (u32 *)param.params;
    struct p4tc_ext_bpf_res *res;
    u32 *direction;

    param.ext_id = EXTERN_IS_NET_PORT;
    *index = ifindex;

    res = bpf_xdp_p4tc_run_extern(skb, &param);
    /* NULL means if index wasn't found, and this will default to host */
    if (!res)
        return false;

    direction = (u32 *)res->params;

    return *direction == FROM_NET;
}

static bool is_host_port_xdp(struct xdp_md *skb, u32 ifindex)
{
    struct p4tc_ext_bpf_params param = {};
    u32 *index = (u32 *)param.params;
    struct p4tc_ext_bpf_res *res;
    u32 *direction;

    param.ext_id = EXTERN_IS_HOST_PORT;
    *index = ifindex;

    res = bpf_xdp_p4tc_run_extern(skb, &param);
    /* NULL means if index wasn't found, and this will default to host */
    if (!res)
        return true;

    direction = (u32 *)res->params;

    return *direction == FROM_HOST;
}

#endif /* P4C_PNA_H */
