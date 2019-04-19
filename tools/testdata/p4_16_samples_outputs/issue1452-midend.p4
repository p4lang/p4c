control c() {
    bit<32> x_0;
    @name("c.a") action a() {
        x_0 = 32w1;
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

