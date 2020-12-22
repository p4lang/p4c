struct Headers {
    bit<8> a;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    apply {
        tmp = h.a;
        {
            @name("ingress.z_0") bit<8> z_0 = h.a;
            @name("ingress.hasReturned_0") bool hasReturned = false;
            @name("ingress.retval_0") bit<8> retval;
            z_0 = 8w3;
            hasReturned = true;
            retval = 8w1;
            h.a = z_0;
            tmp_0 = retval;
        }
        tmp_1 = tmp_0;
        {
            @name("ingress.x_0") bit<8> x_0 = tmp;
            @name("ingress.hasReturned") bool hasReturned_0 = false;
            @name("ingress.retval") bit<8> retval_0;
            hasReturned_0 = true;
            retval_0 = 8w4;
            tmp = x_0;
        }
        h.a = tmp;
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

