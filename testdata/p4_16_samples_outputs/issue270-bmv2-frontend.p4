#include <core.p4>
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

action drop() {
}
control VerifyChecksumI(in H hdr, inout M meta) {
    bit<16> inner_cksum_0;
    bit<16> cksum_0;
    bit<16> tmp;
    bit<16> tmp_0;
    bool tmp_1;
    bool tmp_2;
    bool tmp_3;
    bool tmp_4;
    bool tmp_5;
    bool tmp_6;
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum_0;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_0;
    apply {
        tmp = inner_ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
        inner_cksum_0 = tmp;
        tmp_0 = ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        cksum_0 = tmp_0;
        tmp_1 = hdr.inner_ipv4.ihl == 4w5;
        if (!tmp_1) 
            tmp_2 = false;
        else {
            tmp_3 = hdr.inner_ipv4.hdrChecksum != inner_cksum_0;
            tmp_2 = tmp_3;
        }
        if (tmp_2) 
            drop();
        tmp_4 = hdr.ipv4.ihl == 4w5;
        if (!tmp_4) 
            tmp_5 = false;
        else {
            tmp_6 = hdr.ipv4.hdrChecksum != cksum_0;
            tmp_5 = tmp_6;
        }
        if (tmp_5) 
            drop();
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    bit<16> inner_cksum_1;
    bit<16> cksum_1;
    bit<16> tmp_7;
    bit<16> tmp_8;
    bool tmp_9;
    bool tmp_10;
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum_1;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_1;
    apply {
        tmp_7 = inner_ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
        inner_cksum_1 = tmp_7;
        tmp_8 = ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        cksum_1 = tmp_8;
        tmp_9 = hdr.inner_ipv4.ihl == 4w5;
        if (tmp_9) 
            hdr.inner_ipv4.hdrChecksum = inner_cksum_1;
        tmp_10 = hdr.ipv4.ihl == 4w5;
        if (tmp_10) 
            hdr.ipv4.hdrChecksum = cksum_1;
    }
}

control DP(packet_out b, in H p) {
    apply {
    }
}

V1Switch<H, M>(P(), VerifyChecksumI(), Ing(), Eg(), ComputeChecksumI(), DP()) main;
