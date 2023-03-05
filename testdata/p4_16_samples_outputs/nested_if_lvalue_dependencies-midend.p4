#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            default: accept;
        }
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("MyIngress.x") bit<16> x_0;
    @name("MyIngress.y") bit<16> y_0;
    @name("MyIngress.z") bit<16> z_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.ipv4_forward") action ipv4_forward() {
        x_0 = hdr.ipv4.identification;
        y_0 = hdr.ipv4.hdrChecksum;
        z_0 = hdr.ipv4.totalLen;
        if (hdr.ipv4.identification > 16w0) {
            x_0 = 16w1;
            y_0 = 16w2;
            z_0 = 16w6;
            y_0 = 16w4;
            x_0 = 16w5;
            z_0 = 16w28;
        }
        hdr.ipv4.totalLen = z_0 + x_0 + y_0;
    }
    @name("MyIngress.drop") action drop() {
    }
    @name("MyIngress.ipv4_lpm") table ipv4_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            ipv4_forward();
            drop();
            NoAction_1();
        }
        size = 1024;
        default_action = NoAction_1();
    }
    apply {
        ipv4_lpm_0.apply();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
