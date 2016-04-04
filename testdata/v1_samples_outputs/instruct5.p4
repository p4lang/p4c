#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
        packet.extract(hdr.extra.next);
        transition select(hdr.extra.last.x1) {
            16w1 &&& 16w1: parse_extra;
            default: accept;
        }
    }
    @name("start") state start {
        packet.extract(hdr.data);
        transition select(hdr.data.b1) {
            8w0x0: parse_extra;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("noop") action noop() {
    }
    @name("push1") action push1() {
        hdr.extra.push_front(1);
    }
    @name("push2") action push2() {
        hdr.extra.push_front(2);
    }
    @name("test1") table test1() {
        actions = {
            noop;
            push1;
            push2;
        }
        key = {
            hdr.data.f1: exact;
        }
    }

    apply {
        test1.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
        packet.emit(hdr.extra);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
