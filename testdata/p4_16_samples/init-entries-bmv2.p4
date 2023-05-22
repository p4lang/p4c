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
typedef bit<48>  EthernetAddress;

// https://en.wikipedia.org/wiki/Ethernet_frame
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
        // This table definition should cause a warning that the 3rd
        // and 4th entries ahve the same priority, 40.
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
            const priority=10: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=20
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=30
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=40
                  priority=40: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=50
        }
    }
    table t1b {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        // t1b is same as t1, but because it has
        // @noWarn("duplicate_priorities") annotation, no warning
        // should be created for it about duplicate priorities.
        largest_priority_wins = false;
        priority_delta = 10;
 	@noWarn("duplicate_priorities")
        entries = {
            const priority=10: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=20
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=30
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=40
                  priority=40: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=50
        }
    }
    table t2 {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        // This table definition should cause a warning that the 3rd
        // and 4th entries have a different priority order than their
        // source code order.
        entries = {
            const priority=11: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=21
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=31
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=41
                  priority=40: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=50
        }
    }
    table t2b {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        // t2b is same as t2, but because it has
        // @noWarn("entries_out_of_priority_order") annotation, no
        // warning should be created for it about entries out of
        // priority order.
        @noWarn("entries_out_of_priority_order")
        entries = {
            const priority=11: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=21
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=31
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=41
                  priority=40: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=50
        }
    }
    table t3 {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        // t3 should cause warnings that there are both entries with
        // duplicate priorities (2nd and 4th entries, and 3rd and 5th
        // entries), and entries out of priority order (the 3rd and
        // 4th).
        entries = {
            const priority=11: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=12
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=13
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=14
                  priority=13: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=14
        }
    }
    table t3b {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        // t3b is same as t3, but because it has both noWarn
        // annotations, there should be no warnings issues for it
        // about entry priorities.
 	@noWarn("duplicate_priorities")
        @noWarn("entries_out_of_priority_order")
        entries = {
            const priority=11: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=12
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=13
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=14
                  priority=13: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=14
        }
    }
    table t4 {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        // There should definitely be at least one warning about
        // entries with duplicate priorities for this table, where all
        // entries are explicitly given the same priority.
        entries = {
            const priority=2001: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                  priority=2001: (0x02, 0x1181           ) : a_params(2);
                  priority=2001: (0x03, 0x1000 &&& 0xF000) : a_params(3);
                  priority=2001: (0x04, 0x0210 &&& 0x02F0) : a_params(4);
            const priority=2001: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                  priority=2001: (0x06, _                ) : a_params(6);
        }
    }
    table t4b {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        largest_priority_wins = false;
        // t4b is same as t4, but because it has
        // @noWarn("duplicate_priorities") annotation, no warning
        // should be created for it about duplicate priorities.
 	@noWarn("duplicate_priorities")
        entries = {
            const priority=2001: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                  priority=2001: (0x02, 0x1181           ) : a_params(2);
                  priority=2001: (0x03, 0x1000 &&& 0xF000) : a_params(3);
                  priority=2001: (0x04, 0x0210 &&& 0x02F0) : a_params(4);
            const priority=2001: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                  priority=2001: (0x06, _                ) : a_params(6);
        }
    }
    table t5 {
        key = {
            hdr.ethernet.src_addr : exact;
            hdr.ethernet.ether_type : ternary;
        }
        actions = { a; a_params; }
        default_action = a;
        priority_delta = 100;
        // According to the language spec, the priority values as
        // shown by the comments should be calculated by the compiler
        // for the entries that do not have them explicitly, and there
        // should be a warning because the 3rd and 4th entries are out
        // of priority order.
        entries = {
            priority=1000: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                           (0x02, 0x1181           ) : a_params(2); // prio 900
                           (0x03, 0x1000 &&& 0xF000) : a_params(3); // prio 800
                           (0x04, 0x0210 &&& 0x02F0) : a_params(4); // prio 700
            priority= 901: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                           (0x06, _                ) : a_params(6); // prio 801
        }
    }
    apply {
        t1.apply();
        t1b.apply();
        t2.apply();
        t2b.apply();
        t3.apply();
        t3b.apply();
        t4.apply();
        t4b.apply();
        t5.apply();
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
