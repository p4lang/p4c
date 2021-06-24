struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.z_0") bit<8> z;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.x_0") bit<8> x;
    @name("ingress.hasReturned_0") bool hasReturned_0;
    @name("ingress.retval_0") bit<8> retval_0;
    apply {
        z = h.a;
        hasReturned = false;
        z = 8w3;
        hasReturned = true;
        retval = 8w1;
        h.a = z;
        tmp = retval;
        tmp_0 = h.a;
        x = tmp_0;
        hasReturned_0 = false;
        hasReturned_0 = true;
        retval_0 = 8w4;
        tmp_0 = x;
        h.a = tmp_0;
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

