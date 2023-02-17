#include <core.p4>
#define V1MODEL_VERSION 20200408
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
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    @name(".start") state start {
        packet.extract<stack_t>(hdr.stack[32w0]);
        transition select(hdr.stack[32w0].f) {
            16w0xffff: start1;
            default: accept;
        }
    }
    state start1 {
        packet.extract<stack_t>(hdr.stack[32w1]);
        transition select(hdr.stack[32w1].f) {
            16w0xffff: start2;
            default: accept;
        }
    }
    state start2 {
        packet.extract<stack_t>(hdr.stack[32w2]);
        transition select(hdr.stack[32w2].f) {
            16w0xffff: start3;
            default: accept;
        }
    }
    state start3 {
        packet.extract<stack_t>(hdr.stack[32w3]);
        transition select(hdr.stack[32w3].f) {
            16w0xffff: start4;
            default: accept;
        }
    }
    state start4 {
        transition stateOutOfBound;
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
        packet.emit<stack_t>(hdr.stack[0]);
        packet.emit<stack_t>(hdr.stack[1]);
        packet.emit<stack_t>(hdr.stack[2]);
        packet.emit<stack_t>(hdr.stack[3]);
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
