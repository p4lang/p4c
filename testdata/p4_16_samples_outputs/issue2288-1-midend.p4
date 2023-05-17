header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    H h;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @hidden action act() {
        tmp = h.h.a;
        h.h.a = 8w3;
        h.h.b = tmp | 8w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control i(inout Headers h);
package top(i _i);
top(ingress()) main;
