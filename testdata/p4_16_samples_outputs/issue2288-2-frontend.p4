struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.z_0") bit<8> z;
    @name("ingress.x_0") bit<8> x;
    apply {
        z = 8w3;
        h.a = z;
        tmp_0 = h.a;
        x = tmp_0;
        tmp_0 = x;
        h.a = tmp_0;
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
