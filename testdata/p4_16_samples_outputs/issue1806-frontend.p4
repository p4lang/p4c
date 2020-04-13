#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("c.do_act") action do_act() {
    }
    @name("c.tns") table tns_0 {
        key = {
            h.eth.tst[13:4]: exact @name("h.eth.tst[13:4]") ;
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
control ctr<H, SM>(inout H h, inout SM sm);
package top<H, SM>(p<H> _p, ctr<H, SM> _c);
top<Headers, standard_metadata_t>(prs(), c()) main;

