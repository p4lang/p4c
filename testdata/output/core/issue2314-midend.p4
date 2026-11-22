#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> a;
}

header I {
    bit<16> etherType;
}

struct h {
    ethernet_t ether;
    H          h;
    I          i;
}

struct m {
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    @name("MyParser.l3.etherType") bit<16> l3_etherType;
    state start {
        b.extract<ethernet_t>(hdr.ether);
        l3_etherType = hdr.ether.etherType;
        transition L3_start_0;
    }
    state L3_start_0 {
        transition select(l3_etherType) {
            16w0x800: L3_h0;
            16w0x8100: L3_i;
            default: start_1;
        }
    }
    state L3_h0 {
        b.extract<H>(hdr.h);
        transition start_1;
    }
    state L3_i {
        b.extract<I>(hdr.i);
        l3_etherType = hdr.i.etherType;
        transition L3_start_0;
    }
    state start_1 {
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
        b.emit<ethernet_t>(hdr.ether);
        b.emit<H>(hdr.h);
        b.emit<I>(hdr.i);
    }
}

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
