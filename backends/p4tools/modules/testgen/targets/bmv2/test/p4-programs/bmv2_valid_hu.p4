#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<32> index;
}

header H1 {
    bit<8> a;
} 

header H2 {
    bit<16> b;
}

header_union HU1 {
    H1 h1;
    H2 h2;
}

struct headers {
    ethernet_t eth_hdr;
    HU1 hu;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        if (!h.eth_hdr.isValid()) {
            h.eth_hdr = {0, 0, 0, 0};
        }

        h.hu.h2.setValid();
        h.hu.h2.b = 3;
        h.hu.h1.setValid();
        h.hu.h1.a = 1;
        if (h.hu.h2.isValid()) 
		    h.hu.h2.b = 1;
        if (h.eth_hdr.index > 5 ) {
            h.hu.h2.setValid();
            h.hu.h2.b = 10;
        } else {
            h.hu.h1.setValid();
            h.hu.h1.a = 20;
        }
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {}
 }

control update(inout headers hdr, inout Meta m) { apply {    
}}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.hu);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

