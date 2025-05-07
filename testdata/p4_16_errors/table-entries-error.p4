#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) { h.a = v; }

    table t1 {
        key = {
            h.b : exact;
            h.c : ternary;
            h.d : exact;
        }
        actions = { a1; NoAction; }
        const entries = {
            (0x00, 0x01,          0x02) : a1(1);
            (0x00, 0x00 &&& 0xf0, 0x02) : a1(2);
            (0x00, 0x02,          0x02) : a1(3);
            (0x00, 0x02,          0x03) : a1(4);
            (0x01, _,             0x04) : a1(5);
            (0x01, 0x00 &&& 0x0f, 0x04) : a1(6);
            (0x01, 0x10,          0x04) : a1(7);
        }
    }

    table t2 {
        key = { h.b : exact; }
        actions = { a1; NoAction; }
        const entries = {
            0x00 : a1(1);
            0x01 : a1(2);
            0x02 : a1(3);
            0x03 : a1(4);
            0x04 : a1(5);
            0x00 : a1(6);
        }
    }

    apply {
        t1.apply();
        t2.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);

top(ingress()) main;
