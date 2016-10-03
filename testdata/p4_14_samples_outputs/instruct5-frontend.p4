#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

header data2_t {
    bit<16> x1;
    bit<16> x2;
}

struct metadata {
}

struct headers {
    @name("data") 
    data_t     data;
    @name("extra") 
    data2_t[4] extra;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_extra") state parse_extra {
        packet.extract<data2_t>(hdr.extra.next);
        transition select(hdr.extra.last.x1) {
            16w1 &&& 16w1: parse_extra;
            default: accept;
        }
    }
    @name("start") state start {
        packet.extract<data_t>(hdr.data);
        transition select(hdr.data.b1) {
            8w0x0: parse_extra;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("noop") action noop_0() {
    }
    @name("push1") action push1_0() {
        hdr.extra.push_front(1);
    }
    @name("push2") action push2_0() {
        hdr.extra.push_front(2);
    }
    @name("test1") table test1_0() {
        actions = {
            noop_0();
            push1_0();
            push2_0();
            NoAction();
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction();
    }
    apply {
        test1_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<data2_t[4]>(hdr.extra);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
