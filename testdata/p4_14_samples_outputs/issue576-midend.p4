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
    @name("ParserImpl.tmp_hdr") ipv4_t_1 tmp_hdr_1;
    @name("ParserImpl.tmp_hdr_0") ipv4_t_1 tmp_hdr_2;
    bit<160> tmp;
    bit<160> tmp_0;
    @name(".start") state start {
        tmp_hdr_1.setInvalid();
        tmp_hdr_2.setInvalid();
        packet.extract<simpleipv4_t>(hdr.sh.next);
        packet.extract<simpleipv4_t>(hdr.sh.next);
        tmp = packet.lookahead<bit<160>>();
        tmp_hdr_1.setValid();
        tmp_hdr_1.version = tmp[159:156];
        tmp_hdr_1.ihl = tmp[155:152];
        tmp_hdr_1.diffserv = tmp[151:144];
        tmp_hdr_1.totalLen = tmp[143:128];
        tmp_hdr_1.id = tmp[127:112];
        tmp_hdr_1.flags = tmp[111:109];
        tmp_hdr_1.fragOffset = tmp[108:96];
        tmp_hdr_1.ttl = tmp[95:88];
        tmp_hdr_1.protocol = tmp[87:80];
        tmp_hdr_1.hdrChecksum = tmp[79:64];
        tmp_hdr_1.srcAddr = tmp[63:32];
        tmp_hdr_1.dstAddr = tmp[31:0];
        packet.extract<ipv4_t>(hdr.h.next, ((bit<32>)tmp[155:152] << 5) + 32w4294967136);
        tmp_0 = packet.lookahead<bit<160>>();
        tmp_hdr_2.setValid();
        tmp_hdr_2.version = tmp_0[159:156];
        tmp_hdr_2.ihl = tmp_0[155:152];
        tmp_hdr_2.diffserv = tmp_0[151:144];
        tmp_hdr_2.totalLen = tmp_0[143:128];
        tmp_hdr_2.id = tmp_0[127:112];
        tmp_hdr_2.flags = tmp_0[111:109];
        tmp_hdr_2.fragOffset = tmp_0[108:96];
        tmp_hdr_2.ttl = tmp_0[95:88];
        tmp_hdr_2.protocol = tmp_0[87:80];
        tmp_hdr_2.hdrChecksum = tmp_0[79:64];
        tmp_hdr_2.srcAddr = tmp_0[63:32];
        tmp_hdr_2.dstAddr = tmp_0[31:0];
        packet.extract<ipv4_t>(hdr.h.next, ((bit<32>)tmp_0[155:152] << 5) + 32w4294967136);
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
