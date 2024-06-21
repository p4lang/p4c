#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> f;
}

struct headers {
    ethernet_t ethernet;
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("MyIngress.x") bit<16> x_0_inlined_do_round;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_0;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_1;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_2;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_3;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_4;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_5;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_6;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_7;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_8;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_9;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_10;
    @name("MyIngress.x") bit<16> x_0_inlined_do_round_11;
    @name("MyIngress.x") bit<16> x_0;
    @name("MyIngress.do_n_rounds") action do_n_rounds() {
        x_0 = hdr.ethernet.f;
        x_0_inlined_do_round = x_0;
        x_0_inlined_do_round = x_0_inlined_do_round + 16w65535 & x_0_inlined_do_round;
        x_0 = x_0_inlined_do_round;
        x_0_inlined_do_round_0 = x_0;
        x_0_inlined_do_round_0 = x_0_inlined_do_round_0 + 16w65535 & x_0_inlined_do_round_0;
        x_0 = x_0_inlined_do_round_0;
        x_0_inlined_do_round_1 = x_0;
        x_0_inlined_do_round_1 = x_0_inlined_do_round_1 + 16w65535 & x_0_inlined_do_round_1;
        x_0 = x_0_inlined_do_round_1;
        x_0_inlined_do_round_2 = x_0;
        x_0_inlined_do_round_2 = x_0_inlined_do_round_2 + 16w65535 & x_0_inlined_do_round_2;
        x_0 = x_0_inlined_do_round_2;
        x_0_inlined_do_round_3 = x_0;
        x_0_inlined_do_round_3 = x_0_inlined_do_round_3 + 16w65535 & x_0_inlined_do_round_3;
        x_0 = x_0_inlined_do_round_3;
        x_0_inlined_do_round_4 = x_0;
        x_0_inlined_do_round_4 = x_0_inlined_do_round_4 + 16w65535 & x_0_inlined_do_round_4;
        x_0 = x_0_inlined_do_round_4;
        x_0_inlined_do_round_5 = x_0;
        x_0_inlined_do_round_5 = x_0_inlined_do_round_5 + 16w65535 & x_0_inlined_do_round_5;
        x_0 = x_0_inlined_do_round_5;
        x_0_inlined_do_round_6 = x_0;
        x_0_inlined_do_round_6 = x_0_inlined_do_round_6 + 16w65535 & x_0_inlined_do_round_6;
        x_0 = x_0_inlined_do_round_6;
        x_0_inlined_do_round_7 = x_0;
        x_0_inlined_do_round_7 = x_0_inlined_do_round_7 + 16w65535 & x_0_inlined_do_round_7;
        x_0 = x_0_inlined_do_round_7;
        x_0_inlined_do_round_8 = x_0;
        x_0_inlined_do_round_8 = x_0_inlined_do_round_8 + 16w65535 & x_0_inlined_do_round_8;
        x_0 = x_0_inlined_do_round_8;
        x_0_inlined_do_round_9 = x_0;
        x_0_inlined_do_round_9 = x_0_inlined_do_round_9 + 16w65535 & x_0_inlined_do_round_9;
        x_0 = x_0_inlined_do_round_9;
        x_0_inlined_do_round_10 = x_0;
        x_0_inlined_do_round_10 = x_0_inlined_do_round_10 + 16w65535 & x_0_inlined_do_round_10;
        x_0 = x_0_inlined_do_round_10;
        x_0_inlined_do_round_11 = x_0;
        x_0_inlined_do_round_11 = x_0_inlined_do_round_11 + 16w65535 & x_0_inlined_do_round_11;
        x_0 = x_0_inlined_do_round_11;
        hdr.ethernet.f = x_0;
    }
    apply {
        if (hdr.ethernet.isValid()) {
            do_n_rounds();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
