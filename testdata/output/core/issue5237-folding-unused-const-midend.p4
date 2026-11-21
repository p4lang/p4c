struct S {
    bit<32> a;
}

control c(inout S s) {
    @name("c.a") action a_1() {
        s.a = 32w1;
    }
    @hidden table tbl_a {
        actions = {
            a_1();
        }
        const default_action = a_1();
    }
    apply {
        tbl_a.apply();
    }
}

control cc(inout S s);
package top(cc c);
top(c()) main;
