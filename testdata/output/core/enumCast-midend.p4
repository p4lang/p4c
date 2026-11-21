#include <core.p4>

header B {
    bit<32> x;
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
        packet.extract<B>(o.b);
        transition select(o.b.x) {
            32w0 &&& 32w0x1: accept;
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
