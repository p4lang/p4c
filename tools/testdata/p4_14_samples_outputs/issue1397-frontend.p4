#include <core.p4>
#include <v1model.p4>

header hdr1_t {
    bit<16> totalLen;
}

header hdr2_t {
    bit<8> f1;
}

header byte_hdr_t {
    bit<8> f1;
}

struct metadata {
}

struct headers {
    @name(".hdr1") 
    hdr1_t        hdr1;
    @name(".hdr2") 
    hdr2_t        hdr2;
    @name(".byte_hdr") 
    byte_hdr_t[3] byte_hdr;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_1bytes") state parse_1bytes {
        packet.extract<byte_hdr_t>(hdr.byte_hdr[0]);
        transition accept;
    }
    @name(".parse_2bytes") state parse_2bytes {
        packet.extract<byte_hdr_t>(hdr.byte_hdr[1]);
        transition accept;
    }
    @name(".parse_3bytes") state parse_3bytes {
        packet.extract<byte_hdr_t>(hdr.byte_hdr[2]);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<hdr1_t>(hdr.hdr1);
        packet.extract<hdr2_t>(hdr.hdr2);
        transition select(hdr.hdr1.totalLen) {
            16w3: accept;
            16w4: parse_1bytes;
            16w5: parse_2bytes;
            16w6: parse_3bytes;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr1_t>(hdr.hdr1);
        packet.emit<hdr2_t>(hdr.hdr2);
        packet.emit<byte_hdr_t>(hdr.byte_hdr[2]);
        packet.emit<byte_hdr_t>(hdr.byte_hdr[1]);
        packet.emit<byte_hdr_t>(hdr.byte_hdr[0]);
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

