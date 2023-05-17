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

control c(inout Headers h) {
    @name("c.hasReturned") bool hasReturned;
    @hidden action serenum39() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action serenum41() {
        h.eth.setInvalid();
    }
    @hidden action serenum43() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_serenum39 {
        actions = {
            serenum39();
        }
        const default_action = serenum39();
    }
    @hidden table tbl_serenum41 {
        actions = {
            serenum41();
        }
        const default_action = serenum41();
    }
    @hidden table tbl_serenum43 {
        actions = {
            serenum43();
        }
        const default_action = serenum43();
    }
    apply {
        tbl_act.apply();
        if (h.eth.isValid()) {
            ;
        } else {
            tbl_serenum39.apply();
        }
        if (hasReturned) {
            ;
        } else if (h.eth.type == 16w0x800) {
            tbl_serenum41.apply();
        } else {
            tbl_serenum43.apply();
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;
