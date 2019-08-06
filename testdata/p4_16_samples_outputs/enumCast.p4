#include <core.p4>

enum bit<32> X {
    Zero = 0,
    One = 1
}

header B {
    X x;
}

header Opt {
    bit<16> b;
}

struct O {
    B   b;
    Opt opt;
}

parser p(packet_in packet, out O o) {
    state start {
        X x = (X)0;
        X y = 1;
        packet.extract(o.b);
        transition select(o.b.x) {
            X.Zero &&& 0x1: accept;
            default: getopt;
        }
    }
    state getopt {
        packet.extract(o.opt);
        transition accept;
    }
}

parser proto<T>(packet_in p, out T t);
package top<T>(proto<T> _p);
top(p()) main;

