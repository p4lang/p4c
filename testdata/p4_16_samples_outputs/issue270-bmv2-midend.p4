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

struct tuple_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control VerifyChecksumI(in H hdr, inout M meta) {
    bit<16> tmp_3;
    bit<16> tmp_4;
    @name(".drop") action drop_0() {
    }
    @name(".drop") action drop_3() {
    }
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum;
    @name("ipv4_checksum") Checksum16() ipv4_checksum;
    @hidden table tbl_drop {
        actions = {
            drop_0();
        }
        const default_action = drop_0();
    }
    @hidden table tbl_drop_0 {
        actions = {
            drop_3();
        }
        const default_action = drop_3();
    }
    apply {
        tmp_3 = inner_ipv4_checksum.get<tuple_0>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
        tmp_4 = ipv4_checksum.get<tuple_0>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        if (hdr.inner_ipv4.ihl == 4w5 && hdr.inner_ipv4.hdrChecksum != tmp_3) 
            tbl_drop.apply();
        if (hdr.ipv4.ihl == 4w5 && hdr.ipv4.hdrChecksum != tmp_4) 
            tbl_drop_0.apply();
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    bit<16> tmp_5;
    bit<16> tmp_6;
    @name("inner_ipv4_checksum") Checksum16() inner_ipv4_checksum_2;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_2;
    apply {
        tmp_5 = inner_ipv4_checksum_2.get<tuple_0>({ hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.diffserv, hdr.inner_ipv4.totalLen, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.fragOffset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.protocol, hdr.inner_ipv4.srcAddr, hdr.inner_ipv4.dstAddr });
        tmp_6 = ipv4_checksum_2.get<tuple_0>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        if (hdr.inner_ipv4.ihl == 4w5) 
            hdr.inner_ipv4.hdrChecksum = tmp_5;
        if (hdr.ipv4.ihl == 4w5) 
            hdr.ipv4.hdrChecksum = tmp_6;
    }
}

control DP(packet_out b, in H p) {
    apply {
    }
}

V1Switch<H, M>(P(), VerifyChecksumI(), Ing(), Eg(), ComputeChecksumI(), DP()) main;
