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

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    @name("ingress.tmp_condition") bit<8> tmp_condition_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_something") action do_something() {
        stdmeta.egress_spec = 9w1;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            hdr.h.b: exact @name("hdr.h.b");
        }
        actions = {
            NoAction_1();
            do_something();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp_condition_0 = 8w0;
        stdmeta.egress_spec = 9w0;
        switch (simple_table_0.apply().action_run) {
            do_something: {
                tmp_condition_0 = 8w1;
            }
            default: {
            }
        }
        if (tmp_condition_0 > 8w0) {
            hdr.h.a = 8w0;
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
