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
    ipv4_t_1 tmp_hdr_0;
    ipv4_t_1 tmp;
    bit<160> tmp_0;
    @name(".start") state start {
        tmp_0 = packet.lookahead<bit<160>>();
        tmp.setValid();
        tmp.version = tmp_0[159:156];
        tmp.ihl = tmp_0[155:152];
        tmp.diffserv = tmp_0[151:144];
        tmp.totalLen = tmp_0[143:128];
        tmp.id = tmp_0[127:112];
        tmp.flags = tmp_0[111:109];
        tmp.fragOffset = tmp_0[108:96];
        tmp.ttl = tmp_0[95:88];
        tmp.protocol = tmp_0[87:80];
        tmp.hdrChecksum = tmp_0[79:64];
        tmp.srcAddr = tmp_0[63:32];
        tmp.dstAddr = tmp_0[31:0];
        tmp_hdr_0 = tmp;
        packet.extract<ipv4_t>(hdr.h, ((bit<32>)tmp_hdr_0.ihl << 2 << 3) + 32w4294967136);
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

