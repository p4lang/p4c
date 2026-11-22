control ingress(inout bit<8> h) {
    @name("ingress.tmp") bit<7> tmp_0;
    @name("ingress.b") bit<7> b_0;
    @name("ingress.a") action a() {
        b_0 = tmp_0;
        h[0:0] = 1w0;
        tmp_0 = b_0;
    }
    apply {
        tmp_0 = h[7:1];
        a();
        h[7:1] = tmp_0;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;
