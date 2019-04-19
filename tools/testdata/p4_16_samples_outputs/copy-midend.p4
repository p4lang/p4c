struct S {
    bit<32> x;
}

control c(inout bit<32> b) {
    @name("c.a") action a() {
        b = 32w0;
    }
    @hidden table tbl_a {
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

