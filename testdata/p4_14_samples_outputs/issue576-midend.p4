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

header simpleipv4_t {
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

struct metadata {
}

struct headers {
    @name(".h") 
    ipv4_t[2]       h;
    @name(".sh") 
    simpleipv4_t[2] sh;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ipv4_t_1 tmp_hdr_1;
    ipv4_t_1 tmp_hdr_2;
    ipv4_t_1 tmp;
    ipv4_t_1 tmp_0;
    bit<160> tmp_1;
    bit<160> tmp_2;
    @name(".start") state start {
        packet.extract<simpleipv4_t>(hdr.sh.next);
        packet.extract<simpleipv4_t>(hdr.sh.next);
        tmp_1 = packet.lookahead<bit<160>>();
        tmp.setValid();
        tmp.version = tmp_1[159:156];
        tmp.ihl = tmp_1[155:152];
        tmp.diffserv = tmp_1[151:144];
        tmp.totalLen = tmp_1[143:128];
        tmp.id = tmp_1[127:112];
        tmp.flags = tmp_1[111:109];
        tmp.fragOffset = tmp_1[108:96];
        tmp.ttl = tmp_1[95:88];
        tmp.protocol = tmp_1[87:80];
        tmp.hdrChecksum = tmp_1[79:64];
        tmp.srcAddr = tmp_1[63:32];
        tmp.dstAddr = tmp_1[31:0];
        tmp_hdr_1 = tmp;
        packet.extract<ipv4_t>(hdr.h.next, ((bit<32>)tmp_hdr_1.ihl << 2 << 3) + 32w4294967136);
        tmp_2 = packet.lookahead<bit<160>>();
        tmp_0.setValid();
        tmp_0.version = tmp_2[159:156];
        tmp_0.ihl = tmp_2[155:152];
        tmp_0.diffserv = tmp_2[151:144];
        tmp_0.totalLen = tmp_2[143:128];
        tmp_0.id = tmp_2[127:112];
        tmp_0.flags = tmp_2[111:109];
        tmp_0.fragOffset = tmp_2[108:96];
        tmp_0.ttl = tmp_2[95:88];
        tmp_0.protocol = tmp_2[87:80];
        tmp_0.hdrChecksum = tmp_2[79:64];
        tmp_0.srcAddr = tmp_2[63:32];
        tmp_0.dstAddr = tmp_2[31:0];
        tmp_hdr_2 = tmp_0;
        packet.extract<ipv4_t>(hdr.h.next, ((bit<32>)tmp_hdr_2.ihl << 2 << 3) + 32w4294967136);
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
        packet.emit<simpleipv4_t>(hdr.sh[0]);
        packet.emit<simpleipv4_t>(hdr.sh[1]);
        packet.emit<ipv4_t>(hdr.h[0]);
        packet.emit<ipv4_t>(hdr.h[1]);
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

