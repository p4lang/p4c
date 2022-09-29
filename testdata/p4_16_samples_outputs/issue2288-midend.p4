#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.a") action a_1() {
        h.b = 8w0;
    }
    @name("ingress.t") table t_0 {
        key = {
            h.b: exact @name("h.b") ;
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    @hidden action act() {
        tmp_3 = h.a;
        h.a = 8w3;
        h.a = tmp_3;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        t_0.apply();
        tbl_act.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

