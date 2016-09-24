struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    @name("s1") S s1;
    @name("s2") S s2;
    @name("a") action a_0() {
        s2 = { 32w0 };
        s1 = s2;
        s2 = s1;
        b = s2.x;
    }
    table tbl_a() {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<32> _b);
package top(proto _p);
top(c()) main;
