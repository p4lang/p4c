header ipv4_t_1 {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> id;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header ipv4_t_2 {
    varbit<320> options_ipv4;
}

#include <core.p4>
#include <v1model.p4>

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     id;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    bit<32>     srcAddr;
    bit<32>     dstAddr;
    @length(((bit<32>)ihl << 2 << 3) + 32w4294967136) 
    varbit<320> options_ipv4;
}

struct metadata {
}

struct headers {
    @name(".h") 
    ipv4_t h;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ipv4_t_1 tmp_hdr_1;
    ipv4_t_2 tmp_hdr_2;
    @name(".start") state start {
        packet.extract<ipv4_t_1>(tmp_hdr_1);
        packet.extract<ipv4_t_2>(tmp_hdr_2, ((bit<32>)tmp_hdr_1.ihl << 2 << 3) + 32w4294967136);
        hdr.h.setValid();
        hdr.h.version = tmp_hdr_1.version;
        hdr.h.ihl = tmp_hdr_1.ihl;
        hdr.h.diffserv = tmp_hdr_1.diffserv;
        hdr.h.totalLen = tmp_hdr_1.totalLen;
        hdr.h.id = tmp_hdr_1.id;
        hdr.h.flags = tmp_hdr_1.flags;
        hdr.h.fragOffset = tmp_hdr_1.fragOffset;
        hdr.h.ttl = tmp_hdr_1.ttl;
        hdr.h.protocol = tmp_hdr_1.protocol;
        hdr.h.hdrChecksum = tmp_hdr_1.hdrChecksum;
        hdr.h.srcAddr = tmp_hdr_1.srcAddr;
        hdr.h.dstAddr = tmp_hdr_1.dstAddr;
        hdr.h.options_ipv4 = tmp_hdr_2.options_ipv4;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ipv4_t>(hdr.h);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

