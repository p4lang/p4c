header H<T> {
    bit<32> b;
    T t;
}

control c(out bit<32> r) {
    apply {
        H<bit<32>>[3] s;
        s = (H<bit<32>>[3]){ {0, 1}, {2, 3}, (H<bit<32>>){#} };
        r = s[0].b + s[0].t + s[1].b + s[1].t + s[2].b;
    }
}

control C(out bit<32> r);
package top(C _c);

top(c()) main;
