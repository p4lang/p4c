#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

bit<8> f(inout bit<8> x, in bit<8> z) {
    return 8w4;
}

bit<8> g(inout bit<8> z) {
    z = 8w3;
    return 8w1;
}

control ingress(inout Headers h) {
    action a() { h.b = 0; }
    table t {
        key = { h.b: exact; }
        actions = { a; }
        default_action = a;
    }
    apply {
        f(h.a, t.apply().hit ? h.a : h.b);
        f(h.a, g(h.a));
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);

top(ingress()) main;
