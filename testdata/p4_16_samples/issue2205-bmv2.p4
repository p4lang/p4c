#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}


struct Headers {
    ethernet_t eth_hdr;
    H h;
}

struct Meta {
}

bit<8> do_something(inout bit<8> some_input) {
    some_input = 8w1;
    return 1;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> val_0 = 8w3;
    apply {
        h.h.a = val_0 + do_something(val_0);
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(hdr.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {
      b.emit(h);
    } }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
