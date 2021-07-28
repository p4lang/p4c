#include <core.p4>
#include <v1model.p4>


header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8>  a;
    bit<32> b;
    bit<64> c;
}

struct Headers {
    ethernet_t    eth_hdr;
    H    h;
}

struct Meta {}

bit<16> do_thing() {
    return (bit<16>)16w4;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<64> y = 64w3;
    bit<64> z = 64w2;
    action iuJze(in bit<8> hyhe) {
        h.h.c = 64w2;
        y = (bit<64>)do_thing();
        h.h.c = y;
    }
    apply {
        iuJze(8w2);
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

