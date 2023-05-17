#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    action dummy() {}
    action do_action() {
        h.eth_hdr.eth_type = 16w1;
        if (false) {
            h.eth_hdr.eth_type = 16w2;
        } else {
            exit;
        }
    }
    table simple_table {
        key = {
            32w1  : exact @name("akSTMF") ;
        }
        actions = {
            dummy();
            NoAction();
        }
        default_action = NoAction;
    }
    apply {
        switch (simple_table.apply().action_run) {
            dummy: {
            }
            default: {
                return;
            }
        }

        do_action();

    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

