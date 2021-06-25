header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    H h;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.w_0") bit<8> w;
    @name("ingress.hasReturned_0") bool hasReturned;
    @name("ingress.retval_0") bit<8> retval;
    @name("ingress.x_0") bit<8> x;
    @name("ingress.y_0") bit<8> y;
    @name("ingress.z_0") bit<8> z;
    @name("ingress.hasReturned") bool hasReturned_0;
    @name("ingress.retval") bit<8> retval_0;
    apply {
        tmp = h.h.a;
        hasReturned = false;
        w = 8w3;
        hasReturned = true;
        retval = 8w1;
        h.h.a = w;
        tmp_0 = retval;
        x = tmp;
        y = tmp_0;
        hasReturned_0 = false;
        z = x | y;
        hasReturned_0 = true;
        retval_0 = 8w4;
        h.h.b = z;
    }
}

control i(inout Headers h);
package top(i _i);
top(ingress()) main;

