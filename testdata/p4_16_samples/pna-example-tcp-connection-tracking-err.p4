/*
Copyright 2021-2022 Intel Corporation

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

#include <core.p4>
#include "../pna.p4"


// Very simple PNA program intended to demonstrate one use of an
// add-on-miss table to do a simple form of TCP connection tracking,
// where the per-entry expiration times can be modified in the data
// plane, without the control plane changing them.


typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLength;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

// Masks of the bit positions of some bit flags within the TCP flags
// field.
const bit<8> TCP_URG_MASK = 0x20;
const bit<8> TCP_ACK_MASK = 0x10;
const bit<8> TCP_PSH_MASK = 0x08;
const bit<8> TCP_RST_MASK = 0x04;
const bit<8> TCP_SYN_MASK = 0x02;
const bit<8> TCP_FIN_MASK = 0x01;

// Define names for different expire time profile id values.

const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NOW    = (ExpireTimeProfileId_t) 0;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEW    = (ExpireTimeProfileId_t) 1;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_ESTABLISHED = (ExpireTimeProfileId_t) 2;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEVER  = (ExpireTimeProfileId_t) 3;

//////////////////////////////////////////////////////////////////////
// Struct types for holding user-defined collections of headers and
// metadata in the P4 developer's program.
//////////////////////////////////////////////////////////////////////

struct metadata_t {
}

struct headers_t {
    ethernet_t eth;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t  hdr,
    inout metadata_t meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.eth);
        transition select (hdr.eth.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select (hdr.ipv4.protocol) {
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
}

control PreControlImpl(
    in    headers_t  hdr,
    inout metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
        // No IPsec decryption for this example program, so pre
        // control does nothing.
    }
}

struct ct_tcp_table_hit_params_t {
}

control MainControlImpl(
    inout headers_t  hdr,
    inout metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{
    action drop () {
        drop_packet();
    }

    // Inputs from previous tables (or actions, or in general other P4
    // code) that can modify the behavior of actions of ct_tcp_table.
    bool do_add_on_miss;
    bool update_aging_info;
    bool update_expire_time;
    ExpireTimeProfileId_t new_expire_time_profile_id;

    // Outputs from actions of ct_tcp_table
    bool add_succeeded;
    
    action tcp_syn_packet () {
        do_add_on_miss = true;
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_NEW;
    }
    action tcp_fin_or_rst_packet () {
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_NOW;
    }
    action tcp_other_packets () {
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_ESTABLISHED;
    }

    table set_ct_options {
        key = {
            hdr.tcp.flags: ternary;
        }
        actions = {
            tcp_syn_packet;
            tcp_fin_or_rst_packet;
            tcp_other_packets;
        }
        const entries = {
            TCP_SYN_MASK &&& TCP_SYN_MASK: tcp_syn_packet;
            TCP_FIN_MASK &&& TCP_FIN_MASK: tcp_fin_or_rst_packet;
            TCP_RST_MASK &&& TCP_RST_MASK: tcp_fin_or_rst_packet;
        }
        const default_action = tcp_other_packets;
    }
    
    action ct_tcp_table_hit () {
#ifdef AVOID_IF_INSIDE_ACTION
        // This extern function update_expire_info has exactly the
        // same behavior as the code in the #else part of this #ifdef.
        // It is proposed as an extern function included in the
        // standard pna.p4 include file specifically as a workaround
        // for P4 compilers that do not have full support for if
        // statements, such as the BMv2 back end as of 2022-Apr.

        // Another reason to have such an extern function is as a
        // convenience to P4 developers.  Even if their compiler
        // supports if statements inside of actions, if they want the
        // behavior of update_expire_info, this is less code to write
        // and read.
        update_expire_info(update_aging_info, update_expire_time,
                           new_expire_time_profile_id);
#else
        if (update_aging_info) {
            if (update_expire_time) {
                set_entry_expire_time(new_expire_time_profile_id);
                // This is implicit and automatic part of the behavior
                // of set_entry_expire_time() call:
                //restart_expire_timer();
            } else {
                restart_expire_timer();
            }
            // a target might also support additional statements here
        } else {
            // Do nothing here.  In particular, DO NOT
            // restart_expire_time().  Whatever state the target
            // device uses per-entry to represent the last time this
            // entry was matched is left UNCHANGED.  This can be
            // useful in some connection tracking scenarios,
            // e.g. where one wishes to "star the timer" when a FIN
            // packet arrives, but it should KEEP RUNNING as later
            // packets arrive, without being restarted.

            // a target might also support additional statements here
        }
#endif // AVOID_IF_INSIDE_ACTION
    }

    action ct_tcp_table_miss() {
        if (do_add_on_miss) {
            // This example does not need to use allocate_flow_id(),
            // because no later part of the P4 program uses its return
            // value for anything.
            add_succeeded =
                add_entry(action_name = "ct_tcp_table_hit",  // name of action
                          action_params = (ct_tcp_table_hit_params_t)
                                          {},
                          expire_time_profile_id = new_expire_time_profile_id);
        } else {
            drop_packet();
        }
        // a target might also support additional statements here, e.g.
        // mirror the packet
        // update a counter
        // set receive queue
    }

    table ct_tcp_table {
        /* add_on_miss table is restricted to have all exact match fields */
        key = {
            // other key fields also possible, e.g. VRF
            SelectByDirection(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr):
                exact @name("ipv4_addr_0");
            SelectByDirection(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr):
                exact @name("ipv4_addr_1");
            hdr.ipv4.protocol : exact;
            SelectByDirection(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort):
                exact @name("tcp_port_0");
            SelectByDirection(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort):
                exact @name("tcp_port_1");
        }
        actions = {
            @tableonly   ct_tcp_table_hit;
            @defaultonly ct_tcp_table_miss;
        }

        // New PNA table property 'add_on_miss = true' indicates that
        // this table can use extern function add_entry() in its
        // default (i.e. miss) action to add a new entry to the table
        // from the data plane.
        add_on_miss = true;

        default_idle_timeout_for_data_plane_added_entries = 1;

        // New PNA table property 'idle_timeout_with_auto_delete' is
        // similar to 'idle_timeout' in other architectures, except
        // that entries that have not been matched for their expire
        // time interval will be deleted, without the control plane
        // having to delete the entry.
        idle_timeout_with_auto_delete = false;
        const default_action = ct_tcp_table_miss;
    }

    apply {
        // The following code is here to give an _example_ similar to
        // the desired behavior, but is likely to be implemented in a
        // variety of ways, e.g. one or more P4 table lookups.  It is
        // also likely NOT to be identical to what someone experienced
        // at writing TCP connection tracking code actually wants.

        // The important point is that all of these variables:

        // + do_add_on_miss
        // + update_expire_time
        // + new_expire_time_profile_id

        // are assigned the values we want them to have _before_
        // calling ct_tcp_table.apply() below.  The conditions under
        // which they are assigned different values depends upon the
        // contents of the packet header fields and the direction of
        // the packet, and perhaps some earlier P4 table entries
        // populated by control plane software, but _not_ upon the
        // current entries installed in the ct_tcp_table.

        do_add_on_miss = false;
        update_expire_time = false;
        if ((istd.direction == PNA_Direction_t.HOST_TO_NET) &&
            hdr.ipv4.isValid() && hdr.tcp.isValid())
        {
            set_ct_options.apply();
        }

        // ct_tcp_table is a bidirectional table
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            ct_tcp_table.apply();
        }
    }
}

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t hdr,
    in    metadata_t meta,
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.eth);
    }
}

PNA_NIC(
    MainParserImpl(),
    PreControlImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    // Hoping to make this optional parameter later, but not supported
    // by p4c yet.
    //, PreParserImpl()
    ) main;

