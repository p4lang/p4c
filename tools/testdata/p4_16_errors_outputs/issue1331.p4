#include <core.p4>

header ethernet {
    bit<112> data;
}

parser P(packet_in pkt);
control C(packet_in pkt, inout ethernet ether);
package S(C c);
parser MyP(packet_in pkt) {
    state start {
        transition accept;
    }
}

control MyC(packet_in pkt, inout ethernet ether)(P p) {
    action a() {
    }
    table t {
        key = {
        }
        actions = {
            a;
        }
    }
    apply {
        pkt.extract(ether);
        p.apply(pkt);
        t.apply();
    }
}

S(MyC(MyP())) main;

