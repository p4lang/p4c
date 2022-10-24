#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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

struct H {
    ipv4_t inner_ipv4;
    ipv4_t ipv4;
}

struct M {
}

parser P(packet_in b, out H p, inout M meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout H headers, inout M meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

control Eg(inout H hdrs, inout M meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.inner_ipv4.ihl == 4w5, (tuple_0){f0 = hdr.inner_ipv4.version,f1 = hdr.inner_ipv4.ihl,f2 = hdr.inner_ipv4.diffserv,f3 = hdr.inner_ipv4.totalLen,f4 = hdr.inner_ipv4.identification,f5 = hdr.inner_ipv4.flags,f6 = hdr.inner_ipv4.fragOffset,f7 = hdr.inner_ipv4.ttl,f8 = hdr.inner_ipv4.protocol,f9 = hdr.inner_ipv4.srcAddr,f10 = hdr.inner_ipv4.dstAddr}, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.inner_ipv4.ihl == 4w5, (tuple_0){f0 = hdr.inner_ipv4.version,f1 = hdr.inner_ipv4.ihl,f2 = hdr.inner_ipv4.diffserv,f3 = hdr.inner_ipv4.totalLen,f4 = hdr.inner_ipv4.identification,f5 = hdr.inner_ipv4.flags,f6 = hdr.inner_ipv4.fragOffset,f7 = hdr.inner_ipv4.ttl,f8 = hdr.inner_ipv4.protocol,f9 = hdr.inner_ipv4.srcAddr,f10 = hdr.inner_ipv4.dstAddr}, hdr.inner_ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control DP(packet_out b, in H p) {
    apply {
    }
}

V1Switch<H, M>(P(), VerifyChecksumI(), Ing(), Eg(), ComputeChecksumI(), DP()) main;
