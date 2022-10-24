#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.a") action a_1() {
        h.b = 8w0;
    }
    @name("ingress.t") table t_0 {
        key = {
            h.b: exact @name("h.b");
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    apply {
        t_0.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
