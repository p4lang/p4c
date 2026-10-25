header H<T> {
    bit<32> b;
    T       t;
}

header H_bit32 {
    bit<32> b;
    bit<32> t;
}

control c(out bit<32> r) {
    @name("c.s") H_bit32[3] s_0;
    apply {
        s_0[0].setInvalid();
        s_0[1].setInvalid();
        s_0[2].setInvalid();
        s_0 = (H_bit32[3]){(H_bit32){b = 32w0,t = 32w1},(H_bit32){b = 32w2,t = 32w3},(H_bit32){#}};
        r = s_0[0].b + s_0[0].t + s_0[1].b + s_0[1].t + s_0[2].b;
    }
}

control C(out bit<32> r);
package top(C _c);
top(c()) main;
