#include <v1model.p4>


header Ethernet {
    bit<48> src;
    bit<48> dest;
    bit<16> tst;
}

struct Headers {
    Ethernet eth;
}

parser prs(packet_in p, out Headers h) {
    state start {
        transition accept;
    }
}

control c(inout Headers h, inout standard_metadata_t sm) {
    action do_act() {
    }
    table tns {
        key = {
            h.eth.tst[13:4] : exact;
        }
	actions = {
            do_act;
        }
    }

    apply {
        tns.apply();
    }

}

parser p<H>(packet_in _p, out H h);
control ctr<H, SM>(inout H h, inout SM sm);
package top<H, SM>(p<H> _p, ctr<H, SM> _c);

top(prs(), c()) main;
