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
    @name("MyIngress.x") bit<16> x_1;
    @name("MyIngress.x") bit<16> x_2;
    @name("MyIngress.x") bit<16> x_3;
    @name("MyIngress.x") bit<16> x_4;
    @name("MyIngress.x") bit<16> x_5;
    @name("MyIngress.x") bit<16> x_6;
    @name("MyIngress.x") bit<16> x_7;
    @name("MyIngress.x") bit<16> x_8;
    @name("MyIngress.x") bit<16> x_9;
    @name("MyIngress.x") bit<16> x_10;
    @name("MyIngress.x") bit<16> x_11;
    @name("MyIngress.x") bit<16> x_12;
    @name("MyIngress.x") bit<16> x_13;
    @name("MyIngress.x") bit<16> x_26;
    @name("MyIngress.do_n_rounds") action do_n_rounds() {
        x_26 = hdr.ethernet.f;
        x_1 = x_26;
        x_1 = x_1 + 16w65535 & x_1;
        x_26 = x_1;
        x_2 = x_26;
        x_2 = x_2 + 16w65535 & x_2;
        x_26 = x_2;
        x_3 = x_26;
        x_3 = x_3 + 16w65535 & x_3;
        x_26 = x_3;
        x_4 = x_26;
        x_4 = x_4 + 16w65535 & x_4;
        x_26 = x_4;
        x_5 = x_26;
        x_5 = x_5 + 16w65535 & x_5;
        x_26 = x_5;
        x_6 = x_26;
        x_6 = x_6 + 16w65535 & x_6;
        x_26 = x_6;
        x_7 = x_26;
        x_7 = x_7 + 16w65535 & x_7;
        x_26 = x_7;
        x_8 = x_26;
        x_8 = x_8 + 16w65535 & x_8;
        x_26 = x_8;
        x_9 = x_26;
        x_9 = x_9 + 16w65535 & x_9;
        x_26 = x_9;
        x_10 = x_26;
        x_10 = x_10 + 16w65535 & x_10;
        x_26 = x_10;
        x_11 = x_26;
        x_11 = x_11 + 16w65535 & x_11;
        x_26 = x_11;
        x_12 = x_26;
        x_12 = x_12 + 16w65535 & x_12;
        x_26 = x_12;
        x_13 = x_26;
        x_13 = x_13 + 16w65535 & x_13;
        x_26 = x_13;
        hdr.ethernet.f = x_26;
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
