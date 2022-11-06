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

#include <core.p4>
#define V1MODEL_VERSION 20200408
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
    varbit<320> options_ipv4;
}

struct metadata {
}

struct headers {
    @name(".h")
    ipv4_t h;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.tmp_hdr") ipv4_t_1 tmp_hdr_0;
    bit<160> tmp;
    @name(".start") state start {
        tmp_hdr_0.setInvalid();
        tmp = packet.lookahead<bit<160>>();
        tmp_hdr_0.setValid();
        tmp_hdr_0.version = tmp[159:156];
        tmp_hdr_0.ihl = tmp[155:152];
        tmp_hdr_0.diffserv = tmp[151:144];
        tmp_hdr_0.totalLen = tmp[143:128];
        tmp_hdr_0.id = tmp[127:112];
        tmp_hdr_0.flags = tmp[111:109];
        tmp_hdr_0.fragOffset = tmp[108:96];
        tmp_hdr_0.ttl = tmp[95:88];
        tmp_hdr_0.protocol = tmp[87:80];
        tmp_hdr_0.hdrChecksum = tmp[79:64];
        tmp_hdr_0.srcAddr = tmp[63:32];
        tmp_hdr_0.dstAddr = tmp[31:0];
        packet.extract<ipv4_t>(hdr.h, ((bit<32>)tmp[155:152] << 5) + 32w4294967136);
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
