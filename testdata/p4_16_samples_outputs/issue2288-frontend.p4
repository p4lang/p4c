#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<8> tmp_2;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.tmp_4") bit<8> tmp_4;
    @name("ingress.x_0") bit<8> x;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.z_1") bit<8> z;
    @name("ingress.hasReturned_0") bool hasReturned_0;
    @name("ingress.retval_0") bit<8> retval_0;
    @name("ingress.x_1") bit<8> x_2;
    @name("ingress.hasReturned") bool hasReturned_3;
    @name("ingress.retval") bit<8> retval_3;
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
    apply {
        tmp = h.a;
        tmp_0 = t_0.apply().hit;
        if (tmp_0) {
            tmp_1 = h.a;
        } else {
            tmp_1 = h.b;
        }
        tmp_2 = tmp_1;
        x = tmp;
        hasReturned = false;
        hasReturned = true;
        retval = 8w4;
        tmp = x;
        h.a = tmp;
        tmp_3 = h.a;
        z = h.a;
        hasReturned_0 = false;
        z = 8w3;
        hasReturned_0 = true;
        retval_0 = 8w1;
        h.a = z;
        tmp_4 = retval_0;
        x_2 = tmp_3;
        hasReturned_3 = false;
        hasReturned_3 = true;
        retval_3 = 8w4;
        tmp_3 = x_2;
        h.a = tmp_3;
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

