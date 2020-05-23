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
    @hidden action serenumtypedef42() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action serenumtypedef44() {
        h.eth.setInvalid();
    }
    @hidden action serenumtypedef46() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_serenumtypedef42 {
        actions = {
            serenumtypedef42();
        }
        const default_action = serenumtypedef42();
    }
    @hidden table tbl_serenumtypedef44 {
        actions = {
            serenumtypedef44();
        }
        const default_action = serenumtypedef44();
    }
    @hidden table tbl_serenumtypedef46 {
        actions = {
            serenumtypedef46();
        }
        const default_action = serenumtypedef46();
    }
    apply {
        tbl_act.apply();
        if (!h.eth.isValid()) {
            tbl_serenumtypedef42.apply();
        }
        if (!hasReturned) {
            if (h.eth.type == 16w0x800) {
                tbl_serenumtypedef44.apply();
            } else {
                tbl_serenumtypedef46.apply();
            }
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;

