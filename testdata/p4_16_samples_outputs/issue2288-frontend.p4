#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.x_0") bit<8> x;
    @name("ingress.x_1") bit<8> x_2;
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
        tmp = h.a;
        tmp_0 = t_0.apply().hit;
        x = tmp;
        tmp = x;
        h.a = tmp;
        tmp_3 = h.a;
        x_2 = tmp_3;
        tmp_3 = x_2;
        h.a = tmp_3;
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
