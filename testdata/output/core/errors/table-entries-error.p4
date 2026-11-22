#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) {
        h.a = v;
    }
    table t1 {
        key = {
            h.b: exact;
            h.c: ternary;
            h.d: exact;
        }
        actions = {
            a1;
            NoAction;
        }
        const entries = {
                        (0x0, 0x1, 0x2) : a1(1);
                        (0x0, 0x0 &&& 0xf0, 0x2) : a1(2);
                        (0x0, 0x2, 0x2) : a1(3);
                        (0x0, 0x2, 0x3) : a1(4);
                        (0x1, default, 0x4) : a1(5);
                        (0x1, 0x0 &&& 0xf, 0x4) : a1(6);
                        (0x1, 0x10, 0x4) : a1(7);
        }
    }
    table t2 {
        key = {
            h.b: exact;
        }
        actions = {
            a1;
            NoAction;
        }
        const entries = {
                        0x0 : a1(1);
                        0x1 : a1(2);
                        0x2 : a1(3);
                        0x3 : a1(4);
                        0x4 : a1(5);
                        0x0 : a1(6);
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
