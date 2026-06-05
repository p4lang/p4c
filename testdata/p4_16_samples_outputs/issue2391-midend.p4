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
    @hidden action issue2391l55() {
        h.eth.setInvalid();
    }
    @hidden action issue2391l57() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_issue2391l55 {
        actions = {
            issue2391l55();
        }
        const default_action = issue2391l55();
    }
    @hidden table tbl_issue2391l57 {
        actions = {
            issue2391l57();
        }
        const default_action = issue2391l57();
    }
    apply {
        if (h.eth.isValid()) {
            if (h.eth.type == 16w0x800) {
                tbl_issue2391l55.apply();
            } else {
                tbl_issue2391l57.apply();
            }
        } else {
            ;
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;
