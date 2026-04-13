header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    H h;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @hidden action issue22881l22() {
        tmp = h.h.a;
        h.h.a = 8w3;
        h.h.b = tmp | 8w1;
    }
    @hidden table tbl_issue22881l22 {
        actions = {
            issue22881l22();
        }
        const default_action = issue22881l22();
    }
    apply {
        tbl_issue22881l22.apply();
    }
}

control i(inout Headers h);
package top(i _i);
top(ingress()) main;
