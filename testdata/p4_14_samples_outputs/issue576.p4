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
    @length(((bit<32>)ihl << 2) * 8 - 160) 
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
    ipv4_t_1 tmp_hdr;
    ipv4_t_1 tmp_hdr_0;
    @name(".start") state start {
        packet.extract(hdr.sh.next);
        packet.extract(hdr.sh.next);
        tmp_hdr = packet.lookahead<ipv4_t_1>();
        packet.extract(hdr.h.next, (bit<32>)(((bit<32>)tmp_hdr.ihl << 2) * 8 - 160));
        tmp_hdr_0 = packet.lookahead<ipv4_t_1>();
        packet.extract(hdr.h.next, (bit<32>)(((bit<32>)tmp_hdr_0.ihl << 2) * 8 - 160));
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
        packet.emit(hdr.sh);
        packet.emit(hdr.h);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

