#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8>  a;
}

struct Headers {
    ethernet_t eth_hdr;
    H    h;
}

struct Meta {}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<128> tmp_key = 128w2;
    action do_action(inout bit<8> val) {
        if (val > 8w10) {
            val = 8w2;
            return;
        } else{
            val = 8w3;
        }

        return;
        val = 8w4;
    }
    table simple_table {
        key = {
            tmp_key             : exact @name("bKiScA") ;
        }
        actions = {
            do_action(h.h.a);
        }
    }
    apply {
        simple_table.apply();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h);
        transition accept;
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

