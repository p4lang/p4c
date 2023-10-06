/*
Copyright 2023 Intel Corporation

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
            hdr.ethernet.ether_type : exact;
        }
        actions = { a; a_params; }
        default_action = a;
        // This table definition should give an error, because it has
        // all key fields exact, and thus does not support priorities
        // for entries.
        entries = {
            priority=10: (0x01, 0x1111) : a_params(1);
                         (0x02, 0x1181) : a_params(2);
                         (0x03, 0x1000) : a_params(3);
                         (0x04, 0x0210) : a_params(4);
                         (0x04, 0x0010) : a_params(5);
                         (0x06, 0x0101) : a_params(6);
        }
    }
    table t2 {
        key = {
            hdr.ethernet.dst_addr : lpm;
        }
        actions = { a; a_params; }
        default_action = a;
        // This table definition should give an error, because it has
        // all key fields lpm, and thus does not support priorities
        // for entries.
        entries = {
            priority=10: (0x01) : a_params(1);
                         (0x02) : a_params(2);
                         (0x03) : a_params(3);
                         (0x04) : a_params(4);
                         (0x04) : a_params(5);
                         (0x06) : a_params(6);
        }
    }
    table t3 {
        key = {
            hdr.ethernet.dst_addr : lpm;
            hdr.ethernet.ether_type: exact;
        }
        actions = { a; a_params; }
        default_action = a;
        // This table definition should give an error, because it has
        // all key fields lpm or exact, and thus does not support
        // priorities for entries.
        entries = {
            priority=10: (0x01, 0x1111) : a_params(1);
                         (0x02, 0x1181) : a_params(2);
                         (0x03, 0x1000) : a_params(3);
                         (0x04, 0x0210) : a_params(4);
                         (0x04, 0x0010) : a_params(5);
                         (0x06, 0x0101) : a_params(6);
        }
    }
    apply {
        t1.apply();
        t2.apply();
        t3.apply();
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
