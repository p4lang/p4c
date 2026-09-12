#include <core.p4>


struct Headers {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

control ingress(inout Headers h) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a1") action a1(@name("v") bit<8> v) {
        h.a = v;
    }
    @name("ingress.t_exact") table t_exact_0 {
        key = {
            h.b: exact @name("h.b");
        }
        actions = {
            a1();
            NoAction_1();
        }
        const entries = {
                        8w0x2 : a1(8w1);
                        8w2 : a1(8w2);
        }
        default_action = NoAction_1();
    }
    apply {
        t_exact_0.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
