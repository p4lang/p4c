#include <core.p4>

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
    bool hasReturned;
    @hidden action act() {
        hasReturned = true;
    }
    @hidden action act_0() {
        hasReturned = false;
    }
    @hidden action act_1() {
        h.eth.setInvalid();
    }
    @hidden action act_2() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        if (!h.eth.isValid()) 
            tbl_act_0.apply();
        if (!hasReturned) 
            if (h.eth.type == 16w0x800) 
                tbl_act_1.apply();
            else 
                tbl_act_2.apply();
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;

