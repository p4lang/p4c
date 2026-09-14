#include <core.p4>


struct Headers {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) {
        h.a = v;
    }
    table t_exact {
        key = {
            h.b: exact @name("h.b");
        }
        actions = {
            a1();
            NoAction();
        }
        const entries = {
                        8w0x2 : a1(8w1);
                        8w2 : a1(8w2);
        }
        default_action = NoAction();
    }
    apply {
        t_exact.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
