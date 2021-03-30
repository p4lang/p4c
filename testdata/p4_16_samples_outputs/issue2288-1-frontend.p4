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
    apply {
        tmp = h.h.a;
        {
            @name("ingress.w_0") bit<8> w_0;
            @name("ingress.hasReturned_0") bool hasReturned = false;
            @name("ingress.retval_0") bit<8> retval;
            w_0 = 8w3;
            hasReturned = true;
            retval = 8w1;
            h.h.a = w_0;
            tmp_0 = retval;
        }
        {
            @name("ingress.x_0") bit<8> x_0 = tmp;
            @name("ingress.y_0") bit<8> y_0 = tmp_0;
            @name("ingress.z_0") bit<8> z_0;
            @name("ingress.hasReturned") bool hasReturned_0 = false;
            @name("ingress.retval") bit<8> retval_0;
            z_0 = x_0 | y_0;
            hasReturned_0 = true;
            retval_0 = 8w4;
            h.h.b = z_0;
        }
    }
}

control i(inout Headers h);
package top(i _i);
top(ingress()) main;

