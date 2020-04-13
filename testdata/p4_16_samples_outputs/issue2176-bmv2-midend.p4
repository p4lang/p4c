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
        packet.emit<ethernet_t>(hdr.eth);
        packet.emit<H>(hdr.h);
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
    bit<8> tmp;
    bit<8> tmp_0;
    bit<8> tmp_1;
    @name("ingress.do_action_2") action do_action_0() {
        tmp_0 = 8w2;
        tmp_1 = 8w0;
    }
    @hidden action act() {
        tmp = h.h.b;
        tmp_0 = h.h.b;
        tmp_1 = h.h.b;
    }
    @hidden action issue2176bmv2l45() {
        h.h.a = 8w1;
    }
    @hidden action act_0() {
        h.h.b = tmp_1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_do_action {
        actions = {
            do_action_0();
        }
        const default_action = do_action_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue2176bmv2l45 {
        actions = {
            issue2176bmv2l45();
        }
        const default_action = issue2176bmv2l45();
    }
    apply {
        tbl_act.apply();
        tbl_do_action.apply();
        tbl_act_0.apply();
        if (tmp_1 > 8w1) {
            tbl_issue2176bmv2l45.apply();
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

