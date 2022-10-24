#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @name("prs.e") Ethernet e_0;
    state start {
        e_0.setInvalid();
        p.extract<Ethernet>(e_0);
        transition select(e_0.type) {
            16w0x800: accept;
            16w0x806: accept;
            default: reject;
        }
    }
}

control c(inout Headers h, inout standard_metadata_t sm) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.do_act") action do_act(@name("type") bit<32> type_1) {
        sm.instance_type = type_1;
    }
    @name("c.tns") table tns_0 {
        key = {
            h.eth.type: exact @name("h.eth.type");
        }
        actions = {
            do_act();
            @defaultonly NoAction_1();
        }
        const entries = {
                        16w0x800 : do_act(32w0x800);
                        16w0x8100 : do_act(32w0x8100);
        }
        default_action = NoAction_1();
    }
    apply {
        tns_0.apply();
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H, SM>(inout H h, inout SM sm);
package top<H, SM>(p<H> _p, ctr<H, SM> _c);
top<Headers, standard_metadata_t>(prs(), c()) main;
