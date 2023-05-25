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
    @name("ingressImpl.a") action a() {
    }
    @name("ingressImpl.a") action a_1() {
    }
    @name("ingressImpl.a") action a_2() {
    }
    @name("ingressImpl.a") action a_3() {
    }
    @name("ingressImpl.a") action a_4() {
    }
    @name("ingressImpl.a") action a_5() {
    }
    @name("ingressImpl.a") action a_6() {
    }
    @name("ingressImpl.a") action a_7() {
    }
    @name("ingressImpl.a") action a_8() {
    }
    @name("ingressImpl.a_params") action a_params(@name("param") bit<32> param) {
    }
    @name("ingressImpl.a_params") action a_params_1(@name("param") bit<32> param_1) {
    }
    @name("ingressImpl.a_params") action a_params_2(@name("param") bit<32> param_2) {
    }
    @name("ingressImpl.a_params") action a_params_3(@name("param") bit<32> param_3) {
    }
    @name("ingressImpl.a_params") action a_params_4(@name("param") bit<32> param_4) {
    }
    @name("ingressImpl.a_params") action a_params_5(@name("param") bit<32> param_5) {
    }
    @name("ingressImpl.a_params") action a_params_6(@name("param") bit<32> param_6) {
    }
    @name("ingressImpl.a_params") action a_params_7(@name("param") bit<32> param_7) {
    }
    @name("ingressImpl.a_params") action a_params_8(@name("param") bit<32> param_8) {
    }
    @name("ingressImpl.t1") table t1_0 {
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
    @name("ingressImpl.t1b") table t1b_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_1();
            a_params_1();
        }
        default_action = a_1();
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("duplicate_priorities") entries = {
                        const priority=10: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_1(32w1);
                        priority=20: (48w0x2, 16w0x1181) : a_params_1(32w2);
                        priority=30: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_1(32w3);
                        const priority=40: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_1(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_1(32w5);
                        priority=50: (48w0x6, default) : a_params_1(32w6);
        }
    }
    @name("ingressImpl.t2") table t2_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_2();
            a_params_2();
        }
        default_action = a_2();
        largest_priority_wins = false;
        priority_delta = 10;
        entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_2(32w1);
                        priority=21: (48w0x2, 16w0x1181) : a_params_2(32w2);
                        priority=31: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_2(32w3);
                        const priority=41: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_2(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_2(32w5);
                        priority=50: (48w0x6, default) : a_params_2(32w6);
        }
    }
    @name("ingressImpl.t2b") table t2b_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_3();
            a_params_3();
        }
        default_action = a_3();
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_3(32w1);
                        priority=21: (48w0x2, 16w0x1181) : a_params_3(32w2);
                        priority=31: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_3(32w3);
                        const priority=41: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_3(32w4);
                        priority=40: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_3(32w5);
                        priority=50: (48w0x6, default) : a_params_3(32w6);
        }
    }
    @name("ingressImpl.t3") table t3_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_4();
            a_params_4();
        }
        default_action = a_4();
        largest_priority_wins = false;
        entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_4(32w1);
                        priority=12: (48w0x2, 16w0x1181) : a_params_4(32w2);
                        priority=13: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_4(32w3);
                        const priority=14: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_4(32w4);
                        priority=13: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_4(32w5);
                        priority=14: (48w0x6, default) : a_params_4(32w6);
        }
    }
    @name("ingressImpl.t3b") table t3b_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_5();
            a_params_5();
        }
        default_action = a_5();
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") @noWarn("entries_out_of_priority_order") entries = {
                        const priority=11: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_5(32w1);
                        priority=12: (48w0x2, 16w0x1181) : a_params_5(32w2);
                        priority=13: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_5(32w3);
                        const priority=14: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_5(32w4);
                        priority=13: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_5(32w5);
                        priority=14: (48w0x6, default) : a_params_5(32w6);
        }
    }
    @name("ingressImpl.t4") table t4_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_6();
            a_params_6();
        }
        default_action = a_6();
        largest_priority_wins = false;
        entries = {
                        const priority=2001: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_6(32w1);
                        priority=2001: (48w0x2, 16w0x1181) : a_params_6(32w2);
                        priority=2001: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_6(32w3);
                        priority=2001: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_6(32w4);
                        const priority=2001: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_6(32w5);
                        priority=2001: (48w0x6, default) : a_params_6(32w6);
        }
    }
    @name("ingressImpl.t4b") table t4b_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_7();
            a_params_7();
        }
        default_action = a_7();
        largest_priority_wins = false;
        @noWarn("duplicate_priorities") entries = {
                        const priority=2001: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_7(32w1);
                        priority=2001: (48w0x2, 16w0x1181) : a_params_7(32w2);
                        priority=2001: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_7(32w3);
                        priority=2001: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_7(32w4);
                        const priority=2001: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_7(32w5);
                        priority=2001: (48w0x6, default) : a_params_7(32w6);
        }
    }
    @name("ingressImpl.t5") table t5_0 {
        key = {
            hdr.ethernet.src_addr  : exact @name("hdr.ethernet.src_addr");
            hdr.ethernet.ether_type: ternary @name("hdr.ethernet.ether_type");
        }
        actions = {
            a_8();
            a_params_8();
        }
        default_action = a_8();
        priority_delta = 100;
        entries = {
                        priority=1000: (48w0x1, 16w0x1111 &&& 16w0xf) : a_params_8(32w1);
                        priority=900: (48w0x2, 16w0x1181) : a_params_8(32w2);
                        priority=800: (48w0x3, 16w0x1000 &&& 16w0xf000) : a_params_8(32w3);
                        priority=700: (48w0x4, 16w0x210 &&& 16w0x2f0) : a_params_8(32w4);
                        priority=901: (48w0x4, 16w0x10 &&& 16w0x2f0) : a_params_8(32w5);
                        priority=801: (48w0x6, default) : a_params_8(32w6);
        }
    }
    apply {
        t1_0.apply();
        t1b_0.apply();
        t2_0.apply();
        t2b_0.apply();
        t3_0.apply();
        t3b_0.apply();
        t4_0.apply();
        t4b_0.apply();
        t5_0.apply();
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
