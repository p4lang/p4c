#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<32> index;
}

struct headers {
    ethernet_t[3] eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr[1]);
        pkt.extract(hdr.eth_hdr[2]);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32> a = 0;
    bit<32> b = 0;
    bit<32> c = 0;
    apply {
        if (h.eth_hdr[0].isValid()) {
	        a = 1;
            h.eth_hdr[1].index = 0;
            h.eth_hdr[2].index = 0;
        } else {
            a = 2;
            h.eth_hdr[1].index = 1;
            h.eth_hdr[2].index = 2;
        }
        h.eth_hdr.push_front(1);
        if (h.eth_hdr[0].isValid() && !(h.eth_hdr[1].isValid()) && !(h.eth_hdr[2].isValid()) &&
            h.eth_hdr[0].index == 2)
            b = 1;
        else
            b = 2;
        h.eth_hdr.pop_front(2);
        if (!(h.eth_hdr[0].isValid()) && !(h.eth_hdr[1].isValid()) && h.eth_hdr[2].isValid() &&
            h.eth_hdr[2].index == 2)
	        c = 1;
        else
	        c = 2;
    }
}

control vrfy(inout headers h, inout Meta m) { apply {
} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
    	pkt.emit(h.eth_hdr[0]);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

