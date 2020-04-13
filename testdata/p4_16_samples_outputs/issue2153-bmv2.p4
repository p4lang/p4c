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
        packet.emit(hdr);
    }
}

parser p(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.eth);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    action do_something() {
        stdmeta.egress_spec = 9w1;
    }
    table simple_table {
        key = {
            hdr.h.b: exact;
        }
        actions = {
            NoAction();
            do_something();
        }
        default_action = NoAction;
    }
    apply {
        bit<8> tmp_condition = 8w0;
        stdmeta.egress_spec = 9w0;
        switch (simple_table.apply().action_run) {
            do_something: {
                tmp_condition = 8w1;
            }
        }

        if (tmp_condition > 0) {
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

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

