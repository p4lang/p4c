/*
 * Copyright 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

typedef bit<16> etype_t;
typedef bit<48> EthernetAddress;

header ethernet_h {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    etype_t ether_type;
}

struct headers_t {
    ethernet_h    ethernet;
}

struct metadata_t {
}

parser ingressParserImpl(
    packet_in pkt,
    out headers_t hdr,
    inout metadata_t umd,
    inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(
    inout headers_t hdr,
    inout metadata_t umd)
{
    apply { }
}

control ingressImpl(
    inout headers_t hdr,
    inout metadata_t umd,
    inout standard_metadata_t stdmeta)
{
    bit<32> x = 6;
    action a() {}
    action a_params(bit<32> param) {
        x = param;
    }
    table t1 {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = true;
        priority_delta = 10;
        // This table definition should give an error, because
        // according to the P4_16 language specification, the third
        // and later entries would be assigned a negative priority
        // value, which is not supported.
        entries = {
            priority=12: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                         (0x02, 0x1181           ) : a_params(2); // priority 2
                         (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority -8
                         (0x04, 0x0210 &&& 0x02F0) : a_params(4);
                         (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                         (0x06, _                ) : a_params(6);
        }
    }
    apply {
        t1.apply();
    }
}

control egressImpl(
    inout headers_t hdr,
    inout metadata_t umd,
    inout standard_metadata_t stdmeta)
{
    apply { }
}

control updateChecksum(
    inout headers_t hdr,
    inout metadata_t umd)
{
    apply { }
}

control egressDeparserImpl(
    packet_out pkt,
    in headers_t hdr)
{
    apply {
        pkt.emit(hdr.ethernet);
    }
}

V1Switch(ingressParserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         egressDeparserImpl()) main;
