control c() {
    @name("c.a") action a() {
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

control proto();
package top(proto p);
top(c()) main;
