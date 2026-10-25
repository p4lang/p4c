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
            h.b: exact @name("h.b");
            h.c: ternary @name("h.c");
            h.d: exact @name("h.d");
        }
        actions = {
            a1();
            NoAction();
        }
        const entries = {
                        (8w0x0, 8w0x1, 8w0x2) : a1(8w1);
                        (8w0x0, 8w0x0 &&& 8w0xf0, 8w0x2) : a1(8w2);
                        (8w0x0, 8w0x2, 8w0x2) : a1(8w3);
                        (8w0x0, 8w0x2, 8w0x3) : a1(8w4);
                        (8w0x1, default, 8w0x4) : a1(8w5);
                        (8w0x1, 8w0x0 &&& 8w0xf, 8w0x4) : a1(8w6);
                        (8w0x1, 8w0x10, 8w0x4) : a1(8w7);
        }
        default_action = NoAction();
    }
    table t2 {
        key = {
            h.b: exact @name("h.b");
        }
        actions = {
            a1();
            NoAction();
        }
        const entries = {
                        8w0x0 : a1(8w1);
                        8w0x1 : a1(8w2);
                        8w0x2 : a1(8w3);
                        8w0x3 : a1(8w4);
                        8w0x4 : a1(8w5);
                        8w0x0 : a1(8w6);
        }
        default_action = NoAction();
    }
    apply {
        t1.apply();
        t2.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
