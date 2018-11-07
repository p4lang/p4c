control c() {
    @name("c.b") action b() {
    }
    @hidden table tbl_b {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_b.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;

