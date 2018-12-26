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
    @name(".start") state start {
        packet.extract<simpleipv4_t>(hdr.sh.next);
        packet.extract<simpleipv4_t>(hdr.sh.next);
        tmp = packet.lookahead<ipv4_t_1>();
        tmp_hdr_1 = tmp;
        packet.extract<ipv4_t>(hdr.h.next, ((bit<32>)tmp_hdr_1.ihl << 2 << 3) + 32w4294967136);
        tmp_0 = packet.lookahead<ipv4_t_1>();
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
        packet.emit<simpleipv4_t[2]>(hdr.sh);
        packet.emit<ipv4_t[2]>(hdr.h);
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

