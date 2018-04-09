struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    @name("c.a") action a_0() {
        b = 32w0;
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

