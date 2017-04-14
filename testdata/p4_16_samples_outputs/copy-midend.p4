struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    S s1;
    S s2;
    @name("a") action a_0() {
        s2.x = 32w0;
        s1.x = s2.x;
        s2.x = s1.x;
        b = s2.x;
    }
    @hidden table tbl_a {
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
