header H<T> {
    bit<32> b;
    T       t;
}

header H_0 {
    bit<32> b;
    bit<32> t;
}

control c(out bit<32> r) {
    apply {
        H_0[3] s;
        s = (H_0[3]){(H_0){b = 32w0,t = 32w1},(H_0){b = 32w2,t = 32w3},(H_0){#}};
        r = s[0].b + s[0].t + s[1].b + s[1].t + s[2].b;
    }
}

control C(out bit<32> r);
package top(C _c);
top(c()) main;
