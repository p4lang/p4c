#include <core.p4>

header Foo {
    bit<32> data;
}

struct H {
    Foo h;
}

parser P(packet_in pkt, out H hdr) {
    state start {
	pkt.extract(hdr.h);
        transition accept;
    }
}

control C(inout H hdr) {
    bool h_is_valid;
    table t {
	key = {
            h_is_valid: exact;
        }
        actions = {
            NoAction;
        }
    }
    apply {
        h_is_valid = hdr.h.isValid();
        t.apply();
    }
}

parser PARSER(packet_in pkt, out H hdr);
control CTRL(inout H hdr);
package top(PARSER _p, CTRL _c);
top(P(), C()) main;
