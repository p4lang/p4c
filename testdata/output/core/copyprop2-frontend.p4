#include <core.p4>

header Foo {
    bit<32> data;
}

struct H {
    Foo h;
}

parser P(packet_in pkt, out H hdr) {
    state start {
        pkt.extract<Foo>(hdr.h);
        transition accept;
    }
}

control C(inout H hdr) {
    @name("C.h_is_valid") bool h_is_valid_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.t") table t_0 {
        key = {
            h_is_valid_0: exact @name("h_is_valid");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        h_is_valid_0 = hdr.h.isValid();
        t_0.apply();
    }
}

parser PARSER(packet_in pkt, out H hdr);
control CTRL(inout H hdr);
package top(PARSER _p, CTRL _c);
top(P(), C()) main;
