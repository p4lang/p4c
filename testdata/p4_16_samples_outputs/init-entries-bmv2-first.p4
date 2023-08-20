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
        pkt.extract<ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t umd) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t umd, inout standard_metadata_t stdmeta) {
    bit<32> x = 32w6;
    action a() {
    }
    action a_params(bit<32> param) {
        x = param;
    }
    table t1 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
                        const priority=10: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=20: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=30: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=40: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=50: (48w0x6, default) : a_params(32w6);
        }
    }
    table t1b {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("duplicate_priorities") entries = {
                        const priority=10: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=20: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=30: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=40: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=50: (48w0x6, default) : a_params(32w6);
        }
    }
    table t2 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=21: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=31: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=41: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=50: (48w0x6, default) : a_params(32w6);
        }
    }
    table t2b {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=21: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=31: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=41: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=50: (48w0x6, default) : a_params(32w6);
        }
    }
    table t3 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=12: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=13: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=14: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=13: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=14: (48w0x6, default) : a_params(32w6);
        }
    }
    table t3b {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=12: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=13: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        const priority=14: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=13: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=14: (48w0x6, default) : a_params(32w6);
        }
    }
    table t4 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        entries = {
                        const priority=2001: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=2001: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=2001: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        priority=2001: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        const priority=2001: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=2001: (48w0x6, default) : a_params(32w6);
        }
    }
    table t4b {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") entries = {
                        const priority=2001: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=2001: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=2001: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        priority=2001: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        const priority=2001: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=2001: (48w0x6, default) : a_params(32w6);
        }
    }
    table t5 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        priority_delta = 100;
        entries = {
                        priority=1000: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params(32w1);
                        priority=900: (48w0x2, 16w0x1181) : a_params(32w2);
                        priority=800: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params(32w3);
                        priority=700: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params(32w4);
                        priority=901: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params(32w5);
                        priority=801: (48w0x6, default) : a_params(32w6);
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
        pkt.emit<ethernet_h>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(ingressParserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), egressDeparserImpl()) main;
