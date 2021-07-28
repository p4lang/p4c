#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Parsed_packet {
    ethernet_t eth;
    H          h;
}

struct Metadata {
}

control deparser(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<Parsed_packet>(hdr);
    }
}

parser p(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Parsed_packet h, inout Metadata m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.val_0") bit<8> val;
    @name("ingress.val_1") bit<8> val_3;
    @name("ingress.val_2") bit<8> val_4;
    @name("ingress.do_action_2") action do_action_0() {
        val = tmp;
        val_3 = tmp_0;
        val_4 = tmp_1;
        val_3 = 8w2;
        val_4 = 8w0;
        tmp = val;
        tmp_0 = val_3;
        tmp_1 = val_4;
    }
    apply {
        tmp = h.h.b;
        tmp_0 = h.h.b;
        tmp_1 = h.h.b;
        do_action_0();
        h.h.b = tmp_1;
        if (h.h.b > 8w1) {
            h.h.a = 8w1;
        }
    }
}

control egress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vrfy(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

control update(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Parsed_packet, Metadata>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

