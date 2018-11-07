control c(inout bit<32> x) {
    @name("c.a") action a() {
        x = 32w15;
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

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

