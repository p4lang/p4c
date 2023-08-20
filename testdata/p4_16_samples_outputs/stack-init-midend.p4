header H<T> {
    bit<32> b;
    T       t;
}

header H_0 {
    bit<32> b;
    bit<32> t;
}

control c(out bit<32> r) {
    H_0 ih;
    @name("c.s") H_0[3] s_0;
    @hidden action stackinit8() {
        ih.setInvalid();
        s_0[0].setInvalid();
        s_0[1].setInvalid();
        s_0[2].setInvalid();
        s_0[0] = (H_0){b = 32w0,t = 32w1};
        s_0[1] = (H_0){b = 32w2,t = 32w3};
        s_0[2] = ih;
        r = s_0[0].b + s_0[0].t + s_0[1].b + s_0[1].t + s_0[2].b;
    }
    @hidden table tbl_stackinit8 {
        actions = {
            stackinit8();
        }
        const default_action = stackinit8();
    }
    apply {
        tbl_stackinit8.apply();
    }
}

control C(out bit<32> r);
package top(C _c);
top(c()) main;
