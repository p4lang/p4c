#include <core.p4>
#include <v1model.p4>

header stack_t {
    bit<16> f;
}

struct metadata {
}

struct headers {
    @name(".stack") 
    stack_t[4] stack;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".p1") state p1 {
        packet.extract(hdr.stack[1]);
        packet.extract(hdr.stack[2]);
        transition accept;
    }
    @name(".p2") state p2 {
        packet.extract(hdr.stack[2]);
        packet.extract(hdr.stack[1]);
        transition accept;
    }
    @name(".start") state start {
        packet.extract(hdr.stack[0]);
        transition select(hdr.stack[0].f) {
            16w0xffff: p1;
            default: p2;
        }
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
        packet.emit(hdr.stack[0]);
        packet.emit(hdr.stack[1]);
        packet.emit(hdr.stack[2]);
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

