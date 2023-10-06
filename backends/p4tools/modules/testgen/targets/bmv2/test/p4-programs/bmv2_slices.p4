#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<8> index;
}

header H {
    bit<32>  a;
}

struct headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
	    hdr.eth_hdr.index = 8w12;
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.eth_hdr.index[1:0] = 2w3;
    	if (h.eth_hdr.index ==8w15)
        	h.eth_hdr.index = 0;
}
}

control vrfy(inout headers h, inout Meta m) { apply {
} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
    	pkt.emit(h.eth_hdr);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

