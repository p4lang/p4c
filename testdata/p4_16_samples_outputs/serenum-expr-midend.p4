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
    @hidden action serenumexpr39() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action serenumexpr41() {
        h.eth.setInvalid();
    }
    @hidden action serenumexpr43() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_serenumexpr39 {
        actions = {
            serenumexpr39();
        }
        const default_action = serenumexpr39();
    }
    @hidden table tbl_serenumexpr41 {
        actions = {
            serenumexpr41();
        }
        const default_action = serenumexpr41();
    }
    @hidden table tbl_serenumexpr43 {
        actions = {
            serenumexpr43();
        }
        const default_action = serenumexpr43();
    }
    apply {
        tbl_act.apply();
        if (!h.eth.isValid()) {
            tbl_serenumexpr39.apply();
        }
        if (!hasReturned) {
            if (h.eth.type == 16w0x800) {
                tbl_serenumexpr41.apply();
            } else {
                tbl_serenumexpr43.apply();
            }
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;

