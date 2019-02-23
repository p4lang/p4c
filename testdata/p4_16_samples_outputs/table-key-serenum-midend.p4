#include <core.p4>
#include <v1model.p4>

header Ethernet {
    bit<48> src;
    bit<48> dest;
    bit<16> type;
}

struct Headers {
    Ethernet eth;
}

parser prs(packet_in p, out Headers h) {
    Ethernet e_0;
    state start {
        p.extract<Ethernet>(e_0);
        transition select(e_0.type) {
            16w0x800: accept;
            16w0x806: accept;
            default: reject;
        }
    }
}

control c(inout Headers h) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.do_act") action do_act() {
    }
    @name("c.tns") table tns_0 {
        key = {
            h.eth.type: exact @name("h.eth.type") ;
        }
        actions = {
            do_act();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        tns_0.apply();
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;

