#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    apply {
        if (true) @likely {
            h.a = 0;
        }
        if (true) @unlikely {
            h.b = 0;
        }
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);

top(ingress()) main;
