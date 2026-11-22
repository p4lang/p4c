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

parser L3(packet_in b, inout h hdr) {
    bit<16> etherType = hdr.ether.etherType;
    state start {
        transition select(etherType) {
            0x800: h0;
            0x8100: i;
            default: accept;
        }
    }
    state h0 {
        b.extract(hdr.h);
        transition accept;
    }
    state i {
        b.extract(hdr.i);
        etherType = hdr.i.etherType;
        transition start;
    }
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    L3() l3;
    state start {
        b.extract(hdr.ether);
        l3.apply(b, hdr);
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
        b.emit(hdr);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
