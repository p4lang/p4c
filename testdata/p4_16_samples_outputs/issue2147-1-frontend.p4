control ingress(inout bit<8> h) {
    @name("ingress.tmp") bit<8> tmp_0;
    @name("ingress.b") bit<8> b_0;
    @name("ingress.a") action a() {
        b_0 = tmp_0;
        tmp_0 = b_0;
    }
    apply {
        tmp_0 = h;
        a();
        h = tmp_0;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;
