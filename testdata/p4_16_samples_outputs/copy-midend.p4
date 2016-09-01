struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    S s1_0;
    S s2_0;
    @name("a") action a() {
        s2_0 = { 32w0 };
        s1_0 = s2_0;
        s2_0 = s1_0;
        b = s2_0.x;
    }
    table tbl_a() {
        actions = {
            a();
        }
        const default_action = a();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<32> _b);
package top(proto _p);
top(c()) main;
