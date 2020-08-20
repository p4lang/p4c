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
    @name("MyIngress.do_n_rounds") action do_n_rounds(inout bit<16> x_0) {
        {
            bit<16> x = x_0;
            x = x + 16w65535 & x;
            x_0 = x;
        }
        {
            bit<16> x_2 = x_0;
            x_2 = x_2 + 16w65535 & x_2;
            x_0 = x_2;
        }
        {
            bit<16> x_3 = x_0;
            x_3 = x_3 + 16w65535 & x_3;
            x_0 = x_3;
        }
        {
            bit<16> x_4 = x_0;
            x_4 = x_4 + 16w65535 & x_4;
            x_0 = x_4;
        }
        {
            bit<16> x_5 = x_0;
            x_5 = x_5 + 16w65535 & x_5;
            x_0 = x_5;
        }
        {
            bit<16> x_6 = x_0;
            x_6 = x_6 + 16w65535 & x_6;
            x_0 = x_6;
        }
        {
            bit<16> x_7 = x_0;
            x_7 = x_7 + 16w65535 & x_7;
            x_0 = x_7;
        }
        {
            bit<16> x_8 = x_0;
            x_8 = x_8 + 16w65535 & x_8;
            x_0 = x_8;
        }
        {
            bit<16> x_9 = x_0;
            x_9 = x_9 + 16w65535 & x_9;
            x_0 = x_9;
        }
        {
            bit<16> x_10 = x_0;
            x_10 = x_10 + 16w65535 & x_10;
            x_0 = x_10;
        }
        {
            bit<16> x_11 = x_0;
            x_11 = x_11 + 16w65535 & x_11;
            x_0 = x_11;
        }
        {
            bit<16> x_12 = x_0;
            x_12 = x_12 + 16w65535 & x_12;
            x_0 = x_12;
        }
        {
            bit<16> x_13 = x_0;
            x_13 = x_13 + 16w65535 & x_13;
            x_0 = x_13;
        }
    }
    apply {
        if (hdr.ethernet.isValid()) {
            do_n_rounds(hdr.ethernet.f);
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

