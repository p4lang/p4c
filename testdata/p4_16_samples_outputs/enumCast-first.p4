#include <core.p4>

enum bit<32> X {
    Zero = 32w0,
    One = 32w1
}

enum bit<8> E1 {
    e1 = 8w0,
    e2 = 8w1,
    e3 = 8w2
}

enum bit<8> E2 {
    e1 = 8w10,
    e2 = 8w11,
    e3 = 8w12
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
        X x = (X)32w0;
        bit<32> z = 32w1;
        bit<32> z1 = X.One;
        bool bb;
        E1 a = E1.e1;
        E2 b = E2.e2;
        bb = a == b;
        bb = bb && a == 8w0;
        bb = bb && b == 8w0;
        a = (E1)b;
        a = (E1)8w1;
        a = (E1)8w21;
        packet.extract<B>(o.b);
        transition select(o.b.x) {
            X.Zero &&& 32w0x1: accept;
            default: getopt;
        }
    }
    state getopt {
        packet.extract<Opt>(o.opt);
        transition accept;
    }
}

parser proto<T>(packet_in p, out T t);
package top<T>(proto<T> _p);
top<O>(p()) main;
