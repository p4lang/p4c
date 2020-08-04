control ingress(inout bit<8> h) {
    action a(inout bit<7> b) {
        h[0:0] = 0;
    }
    apply {
        bit<7> tmp = h[7:1];
        a(tmp);
        h[7:1] = tmp;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top(ingress()) main;
