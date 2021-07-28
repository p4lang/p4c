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


struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

action dummy1(inout bit<48> dst, inout bit<16> type) {
	bool c = true;
	bool c1 = false;
	if(c) {
		type = type + 16w0;
		if(c1) {
			dst = 48w1;		
		}	
		else {
			dst = 48w2;		
		}
	} else {
		type = 16w3;
	}
}

action dummy(inout Headers val1) { 
	dummy1(val1.eth_hdr.dst_addr, val1.eth_hdr.eth_type);
	val1.eth_hdr.dst_addr = val1.eth_hdr.dst_addr + 48w3;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action() {
        if (h.eth_hdr.eth_type == 1) {
            return;
        }
        h.eth_hdr.src_addr = 48w1;
        // this serves as a barrier
        dummy(h);
    }

    apply {
        h.eth_hdr.src_addr = 48w2;
        h.eth_hdr.dst_addr = 48w2;
        h.eth_hdr.eth_type = 16w2;

        simple_action();
    }
}
control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
