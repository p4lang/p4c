#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<16> etype_t;
typedef bit<48> EthernetAddress;
header ethernet_h {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    etype_t         ether_type;
}

struct headers_t {
    ethernet_h ethernet;
}

struct metadata_t {
}

parser ingressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t umd, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t umd) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t umd, inout standard_metadata_t stdmeta) {
    bit<32> x = 6;
    action a() {
    }
    action a_params(bit<32> param) {
        x = param;
    }
    table t1 {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
                        const priority=10: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=40: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t1b {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("duplicate_priorities") entries = {
                        const priority=10: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=40: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t2 {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
                        const priority=11: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=40: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t2b {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=40: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t3 {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        entries = {
                        const priority=11: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=13: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t3b {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=13: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    table t4 {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        entries = {
                        const priority=2001: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        priority=2001: (0x2, 0x1181) : a_params(2);
                        priority=2001: (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        priority=2001: (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        const priority=2001: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        priority=2001: (0x6, default) : a_params(6);
        }
    }
    table t4b {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") entries = {
                        const priority=2001: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        priority=2001: (0x2, 0x1181) : a_params(2);
                        priority=2001: (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        priority=2001: (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        const priority=2001: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        priority=2001: (0x6, default) : a_params(6);
        }
    }
    table t5 {
        key = {
            hdr.ethernet.src_addr  : exact;
            hdr.ethernet.ether_type: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        priority_delta = 100;
        entries = {
                        priority=1000: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=901: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
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

control egressImpl(inout headers_t hdr, inout metadata_t umd, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t umd) {
    apply {
    }
}

control egressDeparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr.ethernet);
    }
}

V1Switch(ingressParserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), egressDeparserImpl()) main;
