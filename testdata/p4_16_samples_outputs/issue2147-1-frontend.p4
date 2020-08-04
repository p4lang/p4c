control ingress(inout bit<8> h) {
    bit<8> tmp_0;
    @name("ingress.a") action a(inout bit<8> b) {
    }
    apply {
        tmp_0 = h;
        a(tmp_0);
        h = tmp_0;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;

