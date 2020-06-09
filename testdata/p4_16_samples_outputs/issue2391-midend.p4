#include <core.p4>

typedef bit<16> Base_t;
typedef Base_t Base1_t;
typedef Base1_t Base2_t;
typedef Base2_t EthT;
header Ethernet {
    bit<48> src;
    bit<48> dest;
    EthT    type;
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
    @hidden action issue2391l47() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action issue2391l49() {
        h.eth.setInvalid();
    }
    @hidden action issue2391l51() {
        h.eth.type = 16w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue2391l47 {
        actions = {
            issue2391l47();
        }
        const default_action = issue2391l47();
    }
    @hidden table tbl_issue2391l49 {
        actions = {
            issue2391l49();
        }
        const default_action = issue2391l49();
    }
    @hidden table tbl_issue2391l51 {
        actions = {
            issue2391l51();
        }
        const default_action = issue2391l51();
    }
    apply {
        tbl_act.apply();
        if (!h.eth.isValid()) {
            tbl_issue2391l47.apply();
        }
        if (!hasReturned) {
            if (h.eth.type == 16w0x800) {
                tbl_issue2391l49.apply();
            } else {
                tbl_issue2391l51.apply();
            }
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;

