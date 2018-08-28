control c() {
    @name("c.b") action b_0() {
    }
    @hidden table tbl_b {
        actions = {
            b_0();
        }
        const default_action = b_0();
    }
    apply {
        tbl_b.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;

